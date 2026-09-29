/* Headless mGBA test harness for Squishy Isle.
 *
 * Usage: harness ROM SAVE SCRIPT OUTDIR
 *
 * Runs the ROM on the real mGBA core (HLE BIOS), feeds scripted button
 * presses, writes screenshots (PPM) and audio (WAV), prints the game's
 * mGBA debug log, and keeps battery saves in SAVE between runs.
 *
 * Script commands, one per line (# starts a comment):
 *   wait N              run N frames with no buttons
 *   hold KEYS N         hold KEYS (A,B,L,R,START,SELECT,UP,DOWN,LEFT,RIGHT
 *                       joined with +) for N frames
 *   tap KEYS            hold for 3 frames, then release for 3 frames
 *   shot NAME           save OUTDIR/NAME.ppm
 *   audio NAME          start recording OUTDIR/NAME.wav (until "audio end")
 *   audio end           stop recording
 *   solo N              hear only PSG channel N (1..4) from now on, 0 = all
 *                       channels again (music checks record each voice alone)
 *   dump NAME           save IO registers, palettes, VRAM, OAM and IWRAM
 *                       to OUTDIR/NAME.bin (checks read game state there)
 *   seek N              in the current area: walk Pip to the nearest box
 *                       until the game reports a touch (at most N frames).
 *   walkto X Y N        walk Pip's feet to (X, Y), or until an exit takes
 *                       him to another area (at most N frames).
 *                       Reads positions from game memory; symbol addresses
 *                       come from `arm-none-eabi-nm` on the ROM's .elf.
 *   goto KIND N         walk to a place in the current area: sign, mail,
 *                       basket, door (then UP into it), exit, momo (at most N)
 *   perf N              run N idle frames and print the worst and average
 *                       frame work in scanlines (228 = a whole frame)
 *   mash SEED N         a toddler plays for N frames: random taps, chords,
 *                       held directions, A mashing, long idle spells, and
 *                       now and then a walk to a box or a place. From then
 *                       on every frame is watched: the game's frame counter
 *                       must go on, the scene must be a known one and the
 *                       CPU must run from BIOS, ROM or IWRAM. The summary
 *                       line "[mash] ..." lists scene visits, the longest
 *                       stay in each scene and any fault.
 */
#include <mgba/core/blip_buf.h>
#include <mgba/core/core.h>
#include <mgba/core/log.h>
#include <mgba/gba/core.h>
#include <mgba/internal/arm/arm.h>
#include <mgba/internal/gba/gba.h>
#include <mgba-util/vfs.h>

#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SAMPLE_RATE 32768

static struct mCore *core;
static color_t *video;
static unsigned vw, vh;
static FILE *wav;
static long wav_samples;
static const char *outdir;
static long frame;

static void log_cb(struct mLogger *l, int cat, enum mLogLevel level, const char *fmt, va_list args) {
    (void)l;
    const char *name = mLogCategoryName(cat);
    if (name && strcmp(name, "GBA Debug") == 0) {
        printf("[game f%ld] ", frame);
        vprintf(fmt, args);
        printf("\n");
    } else if (level & (mLOG_FATAL | mLOG_ERROR)) {
        printf("[mgba %s] ", name ? name : "?");
        vprintf(fmt, args);
        printf("\n");
    }
}

static struct mLogger logger = {.log = log_cb};

static void wav_header(FILE *f, long samples) {
    long data = samples * 4;
    fseek(f, 0, SEEK_SET);
    fwrite("RIFF", 1, 4, f);
    unsigned int v = (unsigned int)(36 + data);
    fwrite(&v, 4, 1, f);
    fwrite("WAVEfmt ", 1, 8, f);
    v = 16; fwrite(&v, 4, 1, f);
    unsigned short s = 1; fwrite(&s, 2, 1, f);        /* PCM */
    s = 2; fwrite(&s, 2, 1, f);                       /* stereo */
    v = SAMPLE_RATE; fwrite(&v, 4, 1, f);
    v = SAMPLE_RATE * 4; fwrite(&v, 4, 1, f);
    s = 4; fwrite(&s, 2, 1, f);
    s = 16; fwrite(&s, 2, 1, f);
    fwrite("data", 1, 4, f);
    v = (unsigned int)data; fwrite(&v, 4, 1, f);
}

static void pump_audio(void) {
    blip_t *left = core->getAudioChannel(core, 0);
    blip_t *right = core->getAudioChannel(core, 1);
    short buf[2048 * 2];
    int avail;
    while ((avail = blip_samples_avail(left)) > 0) {
        if (avail > 2048) avail = 2048;
        blip_read_samples(left, buf, avail, 1);
        blip_read_samples(right, buf + 1, avail, 1);
        if (wav) {
            fwrite(buf, 4, (size_t)avail, wav);
            wav_samples += avail;
        }
    }
}

static void monitor(void);
static int watching;              /* set by the first mash: every frame is checked */

static void run(unsigned keys, long n) {
    core->setKeys(core, keys);
    for (long i = 0; i < n; i++) {
        core->runFrame(core);
        frame++;
        pump_audio();
        if (watching) monitor();
    }
}

static void shot(const char *name) {
    char path[1024];
    snprintf(path, sizeof path, "%s/%s.ppm", outdir, name);
    FILE *f = fopen(path, "wb");
    if (!f) { perror(path); exit(1); }
    fprintf(f, "P6\n%u %u\n255\n", vw, vh);
    for (unsigned y = 0; y < vh; y++) {
        for (unsigned x = 0; x < vw; x++) {
            uint32_t c = video[y * vw + x];
            unsigned char rgb[3] = {c & 0xFF, (c >> 8) & 0xFF, (c >> 16) & 0xFF};
            fwrite(rgb, 1, 3, f);
        }
    }
    fclose(f);
}

static unsigned parse_keys(const char *s) {
    static const char *names[] = {"A", "B", "SELECT", "START", "RIGHT", "LEFT", "UP", "DOWN", "R", "L"};
    unsigned keys = 0;
    char buf[128];
    strncpy(buf, s, sizeof buf - 1);
    buf[sizeof buf - 1] = 0;
    for (char *tok = strtok(buf, "+"); tok; tok = strtok(NULL, "+")) {
        int found = 0;
        for (unsigned i = 0; i < 10; i++) {
            if (strcmp(tok, names[i]) == 0) { keys |= 1u << i; found = 1; }
        }
        if (!found) { fprintf(stderr, "unknown key %s\n", tok); exit(1); }
    }
    return keys;
}


/* ---- seek / walkto: test drivers that walk Pip around the current area ---- */
#define MAP_W 480
#define MAP_H 320
#define FEET_W 5
#define FEET_H 5
#define TOUCH 6
#define SAVE_AREA 112            /* offsetof(SaveData, area): checked by a _Static_assert in save.h */
#define SAVE_FOUND 16
#define GATE_NEED 10

static const char *area_names[4] = {"meadow", "woods", "shore", "clouds"};
static struct { const char *name; uint32_t addr, size; } syms[] = {
    {"pip_x"}, {"pip_y"}, {"boxes"}, {"touch_box"}, {"current"}, {"pending"}, {"fade_dir"}, {"scene_meadow_view"},
    {"game_save"}, {"frame_count"}, {"area_maps"}, {"meadow_doors"}};
enum { S_PIPX, S_PIPY, S_BOXES, S_TOUCH, S_CUR, S_PEND, S_FADE, S_MEADOW, S_SAVE, S_FRAMES, S_AREAS, S_MDOORS, S_N };

/* scenes the mash watch knows (the pointer in `current` must be one of them) */
static struct { const char *name; uint32_t addr; long visits, stay_max; } scenes[] = {
    {"scene_title"}, {"scene_meadow_view"}, {"scene_open"}, {"scene_reveal"}, {"scene_shelf"},
    {"scene_closeup"}, {"scene_letter"}, {"scene_house"}, {"scene_jukebox"}};
#define N_SCENES (int)(sizeof scenes / sizeof scenes[0])
static long scene_lines[N_SCENES], scene_enter_lines[N_SCENES];   /* worst frame work per scene, in scanlines */

/* optional symbols (older ROMs lack them): frame work and the stack mark */
static struct { const char *name; uint32_t addr; } opt[] = {
    {"perf_lines"}, {"perf_enter_lines"}, {"__stack_paint"}, {"__stack_paint_end"}, {"__sp_usr"}};
enum { O_PERF, O_PERF_ENTER, O_PAINT, O_PAINT_END, O_SP, O_N };
static struct { uint32_t solid, solid_size, spots, doors, gates, gates_size; } area_syms[4];
static int syms_loaded;
static const char *rom_path;

static void load_syms(void) {
    if (syms_loaded) return;
    char elf[1024], cmd[1200], line[256];
    snprintf(elf, sizeof elf, "%s", rom_path);
    char *dot = strrchr(elf, '.');
    if (dot) strcpy(dot, ".elf");
    snprintf(cmd, sizeof cmd, "arm-none-eabi-nm -S %s", elf);
    FILE *p = popen(cmd, "r");
    if (!p) { perror("nm"); exit(1); }
    while (fgets(line, sizeof line, p)) {
        unsigned addr, size;
        char name[128];
        char t[4][128];                         /* "addr size type name", or "addr type name" (linker symbols) */
        int nt = sscanf(line, "%127s %127s %127s %127s", t[0], t[1], t[2], t[3]);
        if (nt < 3) continue;
        addr = (unsigned)strtoul(t[0], NULL, 16);
        size = nt == 4 ? (unsigned)strtoul(t[1], NULL, 16) : 0;
        snprintf(name, sizeof name, "%s", t[nt - 1]);
        for (int i = 0; i < S_N; i++)
            if (!strcmp(name, syms[i].name)) { syms[i].addr = addr; syms[i].size = size; }
        for (int i = 0; i < N_SCENES; i++)
            if (!strcmp(name, scenes[i].name)) scenes[i].addr = addr;
        for (int i = 0; i < O_N; i++)
            if (!strcmp(name, opt[i].name)) opt[i].addr = addr;
        for (int a = 0; a < 4; a++) {
            size_t n = strlen(area_names[a]);
            if (strncmp(name, area_names[a], n) || name[n] != '_') continue;
            const char *f = name + n + 1;
            if (!strcmp(f, "solid")) { area_syms[a].solid = addr; area_syms[a].solid_size = size; }
            if (!strcmp(f, "spots")) area_syms[a].spots = addr;
            if (!strcmp(f, "doors")) area_syms[a].doors = addr;
            if (!strcmp(f, "gates")) { area_syms[a].gates = addr; area_syms[a].gates_size = size; }
        }
    }
    pclose(p);
    for (int i = 0; i < S_N; i++)
        if (!syms[i].addr) { fprintf(stderr, "seek: symbol %s not found in %s\n", syms[i].name, elf); exit(1); }
    syms_loaded = 1;
}

static uint32_t rd32(uint32_t a) { return core->busRead32(core, a); }

static uint8_t solid_grid[(MAP_H / 4) * (MAP_W / 4)];   /* 4 px cells at the finest */
static int cell_shift, cur_area;
static int box_x[3], box_y[3], n_box_slots;
static int dist[MAP_H][MAP_W];
static int door_x, door_y, door_w, door_h;
static int gate_r[4][4], n_gates;                       /* shut gates: x, y, w, h */
static int goal_x = -1, goal_y = -1;                    /* walkto target (feet), -1 = boxes */

static int solid_at(int x, int y) {
    if (x < 0 || y < 0 || x >= MAP_W || y >= MAP_H) return 1;
    return solid_grid[(y >> cell_shift) * (MAP_W >> cell_shift) + (x >> cell_shift)];
}

static int box_hit(int i, int x0, int y0, int x1, int y1) {
    if (box_x[i] < 0) return 0;
    return x1 >= box_x[i] - 7 && x0 <= box_x[i] + 7 && y1 >= box_y[i] - 6 && y0 <= box_y[i];
}

static int walkable(int x, int y) {
    if (solid_at(x - FEET_W, y - FEET_H) || solid_at(x + FEET_W, y - FEET_H) || solid_at(x - FEET_W, y) ||
        solid_at(x + FEET_W, y))
        return 0;
    for (int i = 0; i < n_box_slots; i++)
        if (box_hit(i, x - FEET_W, y - FEET_H, x + FEET_W, y)) return 0;
    for (int i = 0; i < n_gates; i++)
        if (x + FEET_W >= gate_r[i][0] && x - FEET_W < gate_r[i][0] + gate_r[i][2] && y >= gate_r[i][1] &&
            y - FEET_H < gate_r[i][1] + gate_r[i][3])
            return 0;
    if (door_w && x >= door_x - 4 && x < door_x + door_w + 4 && y >= door_y && y < door_y + door_h + 16) return 0;   /* door opens the shelf */
    return 1;
}

static int found_in(int area) {
    int n = 0;
    for (int id = area * 20; id < area * 20 + 20; id++) n += core->busRead8(core, syms[S_SAVE].addr + SAVE_FOUND + id) != 0;
    return n;
}

/* Reads the current area's collision, door and shut gates from game memory. */
static void load_area(void) {
    load_syms();
    cur_area = core->busRead8(core, syms[S_SAVE].addr + SAVE_AREA) & 3;
    unsigned cells = area_syms[cur_area].solid_size;       /* the grid size gives the cell size */
    if (!cells) { fprintf(stderr, "seek: no map for area %d\n", cur_area); exit(1); }
    for (cell_shift = 1; (unsigned)((MAP_W >> cell_shift) * (MAP_H >> cell_shift)) > cells; cell_shift++) {}
    for (unsigned i = 0; i < cells && i < sizeof solid_grid; i++) solid_grid[i] = core->busRead8(core, area_syms[cur_area].solid + i);
    uint32_t d = area_syms[cur_area].doors;
    door_x = core->busRead16(core, d);
    door_y = core->busRead16(core, d + 2);
    door_w = core->busRead16(core, d + 4);
    door_h = core->busRead16(core, d + 6);
    n_gates = 0;
    for (unsigned g = 0; g < area_syms[cur_area].gates_size / 10 && n_gates < 4; g++) {
        uint32_t b = area_syms[cur_area].gates + g * 10;
        int to = core->busRead16(core, b + 8);
        int shut = to > 3 || !area_syms[to].solid || (to > 0 && found_in(to - 1) < GATE_NEED);
        if (!shut || !core->busRead16(core, b + 4)) continue;
        for (int k = 0; k < 4; k++) gate_r[n_gates][k] = core->busRead16(core, b + k * 2);
        n_gates++;
    }
}

static void read_boxes(void) {
    int stride = syms[S_BOXES].size / 3;
    n_box_slots = 3;
    for (int i = 0; i < 3; i++) {
        int spot = (int8_t)core->busRead8(core, syms[S_BOXES].addr + i * stride);
        box_x[i] = spot < 0 ? -1 : (int)core->busRead16(core, area_syms[cur_area].spots + spot * 4);
        box_y[i] = spot < 0 ? -1 : (int)core->busRead16(core, area_syms[cur_area].spots + spot * 4 + 2);
    }
}

static int is_goal(int x, int y) {
    if (goal_x >= 0) return abs(x - goal_x) <= 1 && abs(y - goal_y) <= 1;
    for (int i = 0; i < n_box_slots; i++)
        if (box_hit(i, x - FEET_W - TOUCH + 2, y - FEET_H - TOUCH + 2, x + FEET_W + TOUCH - 2, y + TOUCH - 2)) return 1;
    return 0;
}

/* Distance (in 1 px steps) from every feet position to the goal: a spot
 * that touches a box, or the walkto point. */
static void build_field(void) {
    static int qx[MAP_W * MAP_H], qy[MAP_W * MAP_H];
    int head = 0, tail = 0;
    for (int y = 0; y < MAP_H; y++)
        for (int x = 0; x < MAP_W; x++) {
            dist[y][x] = -1;
            if (!walkable(x, y) || !is_goal(x, y)) continue;
            dist[y][x] = 0;
            qx[tail] = x; qy[tail++] = y;
        }
    static const int dx4[4] = {1, -1, 0, 0}, dy4[4] = {0, 0, 1, -1};
    while (head < tail) {
        int x = qx[head], y = qy[head++];
        for (int k = 0; k < 4; k++) {
            int nx = x + dx4[k], ny = y + dy4[k];
            if (nx < 0 || ny < 0 || nx >= MAP_W || ny >= MAP_H || dist[ny][nx] >= 0 || !walkable(nx, ny)) continue;
            dist[ny][nx] = dist[y][x] + 1;
            qx[tail] = nx; qy[tail++] = ny;
        }
    }
}

static int field_at(int x, int y) {
    if (x < 0 || y < 0 || x >= MAP_W || y >= MAP_H || dist[y][x] < 0) return 1 << 30;
    return dist[y][x];
}

static int in_meadow(void) {
    return rd32(syms[S_CUR].addr) == syms[S_MEADOW].addr && rd32(syms[S_PEND].addr) == syms[S_MEADOW].addr &&
           rd32(syms[S_FADE].addr) == 0;
}

/* One step downhill on the distance field (8 directions). */
static void step_downhill(void) {
    int px = (int32_t)rd32(syms[S_PIPX].addr) >> 8, py = (int32_t)rd32(syms[S_PIPY].addr) >> 8;
    static const int ddx[8] = {1, -1, 0, 0, 1, 1, -1, -1}, ddy[8] = {0, 0, 1, -1, 1, -1, 1, -1};
    static const unsigned kx[3] = {1u << 5, 0, 1u << 4}, ky[3] = {1u << 6, 0, 1u << 7};   /* LEFT, -, RIGHT / UP, -, DOWN */
    int best = field_at(px, py), bk = -1;
    for (int k = 0; k < 8; k++) {
        int d = field_at(px + ddx[k] * 2, py + ddy[k] * 2);
        if (d < best) { best = d; bk = k; }
    }
    unsigned keys = 0;
    if (bk >= 0) keys = kx[ddx[bk] + 1] | ky[ddy[bk] + 1];
    else if (goal_x >= 0) {                                 /* near the point: nudge straight at it */
        if (goal_x > px) keys |= kx[2]; else if (goal_x < px) keys |= kx[0];
        if (goal_y > py) keys |= ky[2]; else if (goal_y < py) keys |= ky[0];
    }
    run(keys, 1);
}

static long wait_meadow(long f, long max_frames) {
    load_syms();                                /* in_meadow() needs the addresses */
    while (!in_meadow() && f < max_frames) { run(0, 1); f++; }
    run(0, 2);                                  /* let the map update once */
    return f + 2;
}

static void seek(long max_frames) {
    long f = wait_meadow(0, max_frames);
    load_area();
    goal_x = goal_y = -1;
    int built_for[3] = {-2, -2, -2};
    while (f < max_frames) {
        if ((int32_t)rd32(syms[S_TOUCH].addr) >= 0) {
            printf("[seek] touch after %ld frames\n", f);
            return;
        }
        read_boxes();
        if (box_x[0] != built_for[0] || box_x[1] != built_for[1] || box_x[2] != built_for[2]) {
            build_field();
            for (int i = 0; i < 3; i++) built_for[i] = box_x[i];
        }
        step_downhill();
        f++;
    }
    printf("[seek] timeout after %ld frames\n", f);
}

/* walkto X Y N: walk Pip's feet to (X, Y) around boxes and shut gates.
 * Stops there, when the scene changes (an exit), or after N frames. */
static void walkto(int x, int y, long max_frames) {
    long f = wait_meadow(0, max_frames);
    load_area();
    read_boxes();
    goal_x = x;
    goal_y = y;
    build_field();
    int built_for[3] = {box_x[0], box_x[1], box_x[2]};
    while (f < max_frames) {
        int px = (int32_t)rd32(syms[S_PIPX].addr) >> 8, py = (int32_t)rd32(syms[S_PIPY].addr) >> 8;
        if (!in_meadow()) { printf("[walkto] left the area at %d,%d after %ld frames\n", px, py, f); break; }
        read_boxes();                          /* a new box on the way: plan around it */
        if (box_x[0] != built_for[0] || box_x[1] != built_for[1] || box_x[2] != built_for[2]) {
            build_field();
            for (int i = 0; i < 3; i++) built_for[i] = box_x[i];
        }
        if (abs(px - x) <= 1 && abs(py - y) <= 1) { printf("[walkto] at %d,%d after %ld frames\n", px, py, f); break; }
        step_downhill();
        f++;
    }
    if (f >= max_frames)
        printf("[walkto] stopped at %d,%d (goal %d,%d)\n", (int32_t)rd32(syms[S_PIPX].addr) >> 8,
               (int32_t)rd32(syms[S_PIPY].addr) >> 8, x, y);
    goal_x = goal_y = -1;
}

/* ---- goto: walk to a place of the current area (AreaMap read from ROM) ---- */
#define AM_SIZE 112                     /* sizeof(AreaMap); checked against known values below */
enum { AM_W = 34, AM_H = 36, AM_SPAWN_X = 38, AM_SIGN_X = 58, AM_SIGN_Y = 60, AM_DOORS = 76, AM_NDOORS = 80,
       AM_EXITS = 84, AM_NEXITS = 88, AM_GATES = 92, AM_NGATES = 96, AM_MOMO_X = 98, AM_MOMO_Y = 100,
       AM_MAIL_X = 102, AM_MAIL_Y = 104, AM_BASKET_X = 106, AM_BASKET_Y = 108 };
static const char *place_names[] = {"sign", "mail", "basket", "door", "exit", "momo"};
#define N_PLACES 6

static uint16_t am16(int area, int off) { return core->busRead16(core, syms[S_AREAS].addr + area * AM_SIZE + off); }

static void check_area_layout(void) {
    static int checked;
    if (checked) return;
    load_syms();
    int ok = syms[S_AREAS].size == 4 * AM_SIZE && am16(0, AM_W) == MAP_W && am16(0, AM_H) == MAP_H &&
             rd32(syms[S_AREAS].addr + AM_DOORS) == syms[S_MDOORS].addr && am16(0, AM_SPAWN_X) == 232 &&
             am16(0, AM_MAIL_X) == 276 && am16(0, AM_BASKET_Y) == 243;
    for (int a = 0; a < 4; a++) ok &= core->busRead8(core, syms[S_AREAS].addr + a * AM_SIZE) == a;
    if (!ok) { fprintf(stderr, "goto: AreaMap layout changed; update AM_* in harness.c\n"); exit(1); }
    checked = 1;
}

/* where to stand for a place; returns 0 when the area has none */
static int place_goal(int kind, int pick, int *x, int *y) {
    int a = core->busRead8(core, syms[S_SAVE].addr + SAVE_AREA) & 3;
    uint32_t base = syms[S_AREAS].addr + a * AM_SIZE;
    switch (kind) {
    case 0: *x = am16(a, AM_SIGN_X); *y = am16(a, AM_SIGN_Y) + 14; return *x != 0;
    case 1: *x = am16(a, AM_MAIL_X); *y = am16(a, AM_MAIL_Y) + 14; return *x != 0;
    case 2: *x = am16(a, AM_BASKET_X); *y = am16(a, AM_BASKET_Y) + 14; return *x != 0;
    case 3: {
        if (!core->busRead8(core, base + AM_NDOORS)) return 0;
        uint32_t d = rd32(base + AM_DOORS);
        *x = core->busRead16(core, d) + core->busRead16(core, d + 4) / 2;
        *y = core->busRead16(core, d + 2) + core->busRead16(core, d + 6) + 18;
        return 1;
    }
    case 4: {
        int n = core->busRead8(core, base + AM_NEXITS);
        if (!n) return 0;
        uint32_t e = rd32(base + AM_EXITS) + (pick % n) * 14;
        *x = core->busRead16(core, e) + core->busRead16(core, e + 4) / 2;
        *y = core->busRead16(core, e + 2) + core->busRead16(core, e + 6) / 2;
        return 1;
    }
    default:
        if (am16(a, AM_MOMO_X)) { *x = am16(a, AM_MOMO_X); *y = am16(a, AM_MOMO_Y) + 20; return 1; }
        if (!core->busRead8(core, base + AM_NGATES)) return 0;
        uint32_t g = rd32(base + AM_GATES);
        *x = core->busRead16(core, g) + core->busRead16(core, g + 4) / 2;
        *y = core->busRead16(core, g + 2) + core->busRead16(core, g + 6) + 12;
        return 1;
    }
}

static void goto_place(int kind, int pick, long max_frames) {
    check_area_layout();
    int x, y;
    if (!place_goal(kind, pick, &x, &y)) { printf("[goto] no %s here\n", place_names[kind]); return; }
    printf("[goto] %s at %d,%d\n", place_names[kind], x, y);
    walkto(x, y, max_frames);
    if (kind == 3) run(1u << 6, 24);            /* UP into the door */
}

/* ---- mash: a toddler at the buttons, with every frame watched ---- */
#define K_A 1u
#define K_B 2u
#define K_START 8u
#define K_DPAD 0xF0u
static uint32_t mrng = 1;
static uint32_t mr(void) { mrng ^= mrng << 13; mrng ^= mrng >> 17; mrng ^= mrng << 5; return mrng; }

static uint32_t last_fc;
static long fc_still, fc_still_max, faults, pc_bad;
static int cur_scene = -1;
static long stay;

static void fault(const char *what) {
    faults++;
    if (faults <= 20) printf("[mash] FAULT %s at f%ld\n", what, frame);
}

static void monitor(void) {
    uint32_t fc = rd32(syms[S_FRAMES].addr);
    if (fc == last_fc) {
        if (++fc_still > fc_still_max) fc_still_max = fc_still;
        if (fc_still == 60) fault("hang: the game's frame counter stopped for 60 frames");
    } else {
        fc_still = 0;
    }
    last_fc = fc;
    uint32_t cur = rd32(syms[S_CUR].addr);
    if (!cur && cur_scene < 0) return;                  /* still booting: no scene yet */
    int k = -1;
    for (int i = 0; i < N_SCENES; i++)
        if (scenes[i].addr && scenes[i].addr == cur) k = i;
    if (k < 0) {
        fault("unknown scene pointer");
    } else if (k != cur_scene) {
        scenes[k].visits++;
        cur_scene = k;
        stay = 0;
    } else if (++stay > scenes[k].stay_max) {
        scenes[k].stay_max = stay;
    }
    if (k >= 0 && opt[O_PERF].addr) {
        long l = core->busRead16(core, opt[O_PERF].addr), e = core->busRead16(core, opt[O_PERF_ENTER].addr);
        if (stay > 1 && l > scene_lines[k]) scene_lines[k] = l;     /* the first frame of a scene may still be the load */
        if (e > scene_enter_lines[k]) scene_enter_lines[k] = e;
    }
    uint32_t pc = (uint32_t)((struct ARMCore *)core->cpu)->gprs[ARM_PC];
    if (!(pc < 0x4000 || (pc >= 0x08000000 && pc < 0x0A000000) || (pc >= 0x03000000 && pc < 0x03008000))) {
        if (!pc_bad++) fault("CPU runs outside BIOS, ROM and IWRAM");
    }
}

static void mash(uint32_t seed, long n) {
    static const unsigned taps[] = {K_A, K_A, K_A, K_A, K_B, K_B, K_START, 4u, 1u << 8, 1u << 9, 1u << 4, 1u << 5, 1u << 6, 1u << 7};
    static const unsigned dirs[] = {1u << 4, 1u << 5, 1u << 6, 1u << 7};
    check_area_layout();
    mrng = seed * 2654435761u | 1;
    if (!watching) { watching = 1; last_fc = rd32(syms[S_FRAMES].addr); }
    long end = frame + n;
    while (frame < end) {
        uint32_t r = mr() % 100;
        if (r < 25) {                                   /* taps one button */
            run(taps[mr() % (sizeof taps / sizeof taps[0])], 2 + mr() % 8);
            run(0, mr() % 12);
        } else if (r < 45) {                            /* walks, sometimes on a slant, tapping A on the way */
            unsigned d = dirs[mr() % 4];
            if (mr() % 4 == 0) d |= dirs[mr() % 4];
            long len = 10 + mr() % 90;
            for (long i = 0; i < len; i++) run(d | (mr() % 12 == 0 ? K_A : 0), 1);
        } else if (r < 55) {                            /* a whole hand on the buttons */
            run(mr() & 0x3FF, 1 + mr() % 30);
            run(0, mr() % 10);
        } else if (r < 67) {                            /* bashes A (or A and B) as fast as it can */
            unsigned k = mr() % 3 ? K_A : K_A | K_B;
            long len = 30 + mr() % 120;
            for (long i = 0; i < len;) {
                long on = 1 + mr() % 3, off = 1 + mr() % 3;
                run(k, on);
                run(0, off);
                i += on + off;
            }
        } else if (r < 75) {                            /* looks away */
            run(0, 30 + mr() % 400);
        } else if (r < 80) {                            /* leans on one button */
            unsigned k = mr() % 3 ? K_A : taps[mr() % (sizeof taps / sizeof taps[0])];
            run(k, 120 + mr() % 300);
        } else {                                        /* a grown-up helps: walk to a box or a place */
            if (!in_meadow()) { run(0, 1 + mr() % 5); continue; }
            int what = mr() % 12;
            if (what < 6) seek(900);
            else goto_place(what - 6, (int)mr(), 900);
        }
    }
    printf("[mash] seed %u frames %ld faults %ld hang_max %ld\n", seed, n, faults, fc_still_max);
    for (int i = 0; i < N_SCENES; i++)
        printf("[mash] scene %s visits %ld stay_max %ld lines_max %ld enter_lines %ld\n", scenes[i].name + 6,
               scenes[i].visits, scenes[i].stay_max, scene_lines[i], scene_enter_lines[i]);
    if (opt[O_PAINT].addr && opt[O_SP].addr) {           /* the deepest the stack went: the lowest word not still marked */
        uint32_t a = opt[O_PAINT].addr;
        while (a < opt[O_PAINT_END].addr && rd32(a) == 0xA5A5A5A5u) a += 4;
        printf("[mash] stack deepest %u bytes of %u marked%s\n", opt[O_SP].addr - a, opt[O_SP].addr - opt[O_PAINT].addr,
               a == opt[O_PAINT].addr ? " (OVER the mark)" : "");
    }
}

int main(int argc, char **argv) {
    if (argc < 5) {
        fprintf(stderr, "usage: %s ROM SAVE SCRIPT OUTDIR\n", argv[0]);
        return 2;
    }
    outdir = argv[4];
    rom_path = argv[1];
    mLogSetDefaultLogger(&logger);

    core = GBACoreCreate();
    if (!core || !core->init(core)) { fprintf(stderr, "core init failed\n"); return 1; }
    mCoreInitConfig(core, NULL);
    core->desiredVideoDimensions(core, &vw, &vh);
    video = calloc(vw * vh, sizeof(color_t));
    core->setVideoBuffer(core, video, vw);
    core->setAudioBufferSize(core, 2048);
    blip_set_rates(core->getAudioChannel(core, 0), core->frequency(core), SAMPLE_RATE);
    blip_set_rates(core->getAudioChannel(core, 1), core->frequency(core), SAMPLE_RATE);

    if (!mCoreLoadFile(core, argv[1])) { fprintf(stderr, "cannot load %s\n", argv[1]); return 1; }
    struct VFile *save = VFileOpen(argv[2], O_CREAT | O_RDWR);
    if (!save || !core->loadSave(core, save)) { fprintf(stderr, "cannot open save %s\n", argv[2]); return 1; }
    core->reset(core);

    FILE *script = fopen(argv[3], "r");
    if (!script) { perror(argv[3]); return 1; }
    char line[512];
    while (fgets(line, sizeof line, script)) {
        char cmd[32] = {0}, a[256] = {0};
        long n = 0;
        char *hash = strchr(line, '#');
        if (hash) *hash = 0;
        int got = sscanf(line, "%31s %255s %ld", cmd, a, &n);
        if (got <= 0) continue;
        if (!strcmp(cmd, "wait")) {
            run(0, atol(a));
        } else if (!strcmp(cmd, "hold")) {
            run(parse_keys(a), n);
        } else if (!strcmp(cmd, "tap")) {
            run(parse_keys(a), 3);
            run(0, 3);
        } else if (!strcmp(cmd, "shot")) {
            shot(a);
        } else if (!strcmp(cmd, "dump")) {
            char path[1024];
            snprintf(path, sizeof path, "%s/%s.bin", outdir, a);
            FILE *f = fopen(path, "wb");
            static const struct { uint32_t base, size; } regions[] = {
                {0x04000000, 0x400}, {0x05000000, 0x400}, {0x06000000, 0x18000}, {0x07000000, 0x400},
                {0x03000000, 0x8000}};
            for (unsigned r = 0; r < 5; r++)
                for (uint32_t i = 0; i < regions[r].size; i++)
                    fputc(core->busRead8(core, regions[r].base + i), f);
            fclose(f);
        } else if (!strcmp(cmd, "solo")) {
            long ch = atol(a);
            /* core->enableAudioChannel crashes in this libmgba build: set the flags directly */
            struct GBA *gba = core->board;
            for (int i = 0; i < 4; i++) gba->audio.psg.forceDisableCh[i] = !(ch == 0 || i == ch - 1);
            gba->audio.forceDisableChA = gba->audio.forceDisableChB = ch != 0;
        } else if (!strcmp(cmd, "seek")) {
            seek(atol(a));
        } else if (!strcmp(cmd, "walkto")) {
            long y = 0, m = 0;
            sscanf(line, "%*s %*s %ld %ld", &y, &m);
            walkto(atoi(a), (int)y, m);
        } else if (!strcmp(cmd, "goto")) {
            int kind = -1;
            for (int i = 0; i < N_PLACES; i++)
                if (!strcmp(a, place_names[i])) kind = i;
            if (kind < 0) { fprintf(stderr, "goto: unknown place %s\n", a); return 1; }
            goto_place(kind, 0, n);
        } else if (!strcmp(cmd, "perf")) {             /* perf N: N idle frames; the frame work, in scanlines */
            load_syms();
            long nf = atol(a), mx = 0, sum = 0, over = 0;
            for (long i = 0; i < nf; i++) {
                run(0, 1);
                long l = opt[O_PERF].addr ? core->busRead16(core, opt[O_PERF].addr) : 0;
                if (l > mx) mx = l;
                sum += l;
                over += l > 180;
            }
            printf("[perf] %ld frames: max %ld avg %ld lines, %ld over 180\n", nf, mx, nf ? sum / nf : 0, over);
        } else if (!strcmp(cmd, "mash")) {
            mash((uint32_t)strtoul(a, NULL, 0), n);
        } else if (!strcmp(cmd, "audio")) {
            if (!strcmp(a, "end")) {
                if (wav) { wav_header(wav, wav_samples); fclose(wav); wav = NULL; }
            } else {
                char path[1024];
                snprintf(path, sizeof path, "%s/%s.wav", outdir, a);
                wav = fopen(path, "wb");
                wav_samples = 0;
                wav_header(wav, 0);
            }
        } else {
            fprintf(stderr, "unknown command %s\n", cmd);
            return 1;
        }
    }
    if (wav) { wav_header(wav, wav_samples); fclose(wav); }
    core->unloadROM(core);
    core->deinit(core);
    printf("harness: %ld frames\n", frame);
    return 0;
}
