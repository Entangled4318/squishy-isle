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
 *   seek N              in the meadow: walk Pip to the nearest gift box
 *                       until the game reports a touch (at most N frames).
 *                       Reads positions from game memory; symbol addresses
 *                       come from `arm-none-eabi-nm` on the ROM's .elf.
 */
#include <mgba/core/blip_buf.h>
#include <mgba/core/core.h>
#include <mgba/core/log.h>
#include <mgba/gba/core.h>
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

static void run(unsigned keys, long n) {
    core->setKeys(core, keys);
    for (long i = 0; i < n; i++) {
        core->runFrame(core);
        frame++;
        pump_audio();
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


/* ---- seek: a test driver that walks Pip to the nearest box ---- */
#define MAP_W 480
#define MAP_H 320
#define FEET_W 5
#define FEET_H 5
#define TOUCH 6

static struct { const char *name; uint32_t addr, size; } syms[] = {
    {"pip_x"}, {"pip_y"}, {"boxes"}, {"touch_box"}, {"meadow_solid"}, {"meadow_spots"},
    {"meadow_doors"}, {"current"}, {"pending"}, {"fade_dir"}, {"scene_meadow_view"}};
enum { S_PIPX, S_PIPY, S_BOXES, S_TOUCH, S_SOLID, S_SPOTS, S_DOORS, S_CUR, S_PEND, S_FADE, S_MEADOW, S_N };
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
        char type, name[128];
        if (sscanf(line, "%x %x %c %127s", &addr, &size, &type, name) != 4) continue;
        for (int i = 0; i < S_N; i++)
            if (!strcmp(name, syms[i].name)) { syms[i].addr = addr; syms[i].size = size; }
    }
    pclose(p);
    for (int i = 0; i < S_N; i++)
        if (!syms[i].addr) { fprintf(stderr, "seek: symbol %s not found in %s\n", syms[i].name, elf); exit(1); }
    syms_loaded = 1;
}

static uint32_t rd32(uint32_t a) { return core->busRead32(core, a); }

static uint8_t solid_grid[(MAP_H / 4) * (MAP_W / 4)];   /* 4 px cells at the finest */
static int cell_shift;
static int box_x[3], box_y[3], n_box_slots;
static int dist[MAP_H][MAP_W];

static int solid_at(int x, int y) {
    if (x < 0 || y < 0 || x >= MAP_W || y >= MAP_H) return 1;
    return solid_grid[(y >> cell_shift) * (MAP_W >> cell_shift) + (x >> cell_shift)];
}

static int box_hit(int i, int x0, int y0, int x1, int y1) {
    if (box_x[i] < 0) return 0;
    return x1 >= box_x[i] - 7 && x0 <= box_x[i] + 7 && y1 >= box_y[i] - 6 && y0 <= box_y[i];
}

static int door_x, door_y, door_w, door_h;

static int walkable(int x, int y) {
    if (solid_at(x - FEET_W, y - FEET_H) || solid_at(x + FEET_W, y - FEET_H) || solid_at(x - FEET_W, y) ||
        solid_at(x + FEET_W, y))
        return 0;
    for (int i = 0; i < n_box_slots; i++)
        if (box_hit(i, x - FEET_W, y - FEET_H, x + FEET_W, y)) return 0;
    if (x >= door_x - 4 && x < door_x + door_w + 4 && y >= door_y && y < door_y + door_h + 16) return 0;   /* door opens the shelf */
    return 1;
}

static void read_boxes(void) {
    int stride = syms[S_BOXES].size / 3;
    n_box_slots = 3;
    for (int i = 0; i < 3; i++) {
        int spot = (int8_t)core->busRead8(core, syms[S_BOXES].addr + i * stride);
        box_x[i] = spot < 0 ? -1 : (int)core->busRead16(core, syms[S_SPOTS].addr + spot * 4);
        box_y[i] = spot < 0 ? -1 : (int)core->busRead16(core, syms[S_SPOTS].addr + spot * 4 + 2);
    }
}

/* Distance (in 1 px steps) from every feet position to a spot that touches a box. */
static void build_field(void) {
    static int qx[MAP_W * MAP_H], qy[MAP_W * MAP_H];
    int head = 0, tail = 0;
    for (int y = 0; y < MAP_H; y++)
        for (int x = 0; x < MAP_W; x++) {
            dist[y][x] = -1;
            if (!walkable(x, y)) continue;
            for (int i = 0; i < n_box_slots; i++)
                if (box_hit(i, x - FEET_W - TOUCH + 2, y - FEET_H - TOUCH + 2, x + FEET_W + TOUCH - 2, y + TOUCH - 2)) {
                    dist[y][x] = 0;
                    qx[tail] = x; qy[tail++] = y;
                    break;
                }
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

static void seek(long max_frames) {
    load_syms();
    unsigned cells = syms[S_SOLID].size;       /* the grid size gives the cell size */
    for (cell_shift = 1; (unsigned)((MAP_W >> cell_shift) * (MAP_H >> cell_shift)) > cells; cell_shift++) {}
    for (unsigned i = 0; i < cells && i < sizeof solid_grid; i++) solid_grid[i] = core->busRead8(core, syms[S_SOLID].addr + i);
    door_x = core->busRead16(core, syms[S_DOORS].addr);
    door_y = core->busRead16(core, syms[S_DOORS].addr + 2);
    door_w = core->busRead16(core, syms[S_DOORS].addr + 4);
    door_h = core->busRead16(core, syms[S_DOORS].addr + 6);
    long f = 0;
    while (!in_meadow() && f < max_frames) { run(0, 1); f++; }
    run(0, 2);                                  /* let the meadow update touch_box once */

    f += 2;
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
        run(keys, 1);
        f++;
    }
    printf("[seek] timeout after %ld frames\n", f);
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
