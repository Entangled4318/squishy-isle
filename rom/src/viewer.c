/* Blossom Meadow: Pip walks around the full-size map (2x2 screens).
 * Ground layer under Pip, overlay (tree tops, roof) over Pip, collision
 * from the exported 8x8 solid grid. The cottage door opens the shelf. */
#include "game.h"
#include "game_assets.h"
#include "sound.h"
#include "system.h"

#define T_PIP     0        /* 8 frames x 8 tiles */
#define T_SHADOW  64
#define P_PIP     0
#define P_SHADOW  1

#define SPEED     320      /* 8.8 fixed: 1.25 px per frame */
#define FEET_W    5        /* half width of Pip's feet box */
#define FEET_H    5

enum { DIR_DOWN, DIR_UP, DIR_RIGHT, DIR_LEFT };

static s32 pip_x = MEADOW_SPAWN_X << 8, pip_y = MEADOW_SPAWN_Y << 8;
static int dir = DIR_DOWN, walk_t, cam_x, cam_y, door_cool;

static bool solid_at(int x, int y) {
    if (x < 0 || y < 0 || x >= MEADOW_W || y >= MEADOW_H) return true;
    return meadow_solid[(y >> 3) * (MEADOW_W >> 3) + (x >> 3)];
}

static bool blocked(int fx, int fy) {
    return solid_at(fx - FEET_W, fy - FEET_H) || solid_at(fx + FEET_W, fy - FEET_H) ||
           solid_at(fx - FEET_W, fy) || solid_at(fx + FEET_W, fy);
}

static void update_camera(void) {
    int px = pip_x >> 8, py = pip_y >> 8;
    int tx = px - SCREEN_W / 2, ty = py - 12 - SCREEN_H / 2;
    if (tx < 0) tx = 0;
    if (ty < 0) ty = 0;
    if (tx > MEADOW_W - SCREEN_W) tx = MEADOW_W - SCREEN_W;
    if (ty > MEADOW_H - SCREEN_H) ty = MEADOW_H - SCREEN_H;
    cam_x = tx;
    cam_y = ty;
    for (int i = 1; i <= 2; i++) {
        bg_scroll_x[i] = (s16)cam_x;
        bg_scroll_y[i] = (s16)cam_y;
    }
}

static void enter(void) {
    dma3_copy32(CHARBLOCK(0), meadow_tiles, sizeof meadow_tiles);
    dma3_copy16(PAL_BG, meadow_pal, sizeof meadow_pal);
    dma3_copy32(SCREENBLOCK(24), meadow_ground, sizeof meadow_ground);
    dma3_copy32(SCREENBLOCK(28), meadow_overlay, sizeof meadow_overlay);
    REG_BGCNT(2) = BG_PRIO(2) | BG_CBB(0) | BG_SBB(24) | 0xC000;     /* 64x64 */
    REG_BGCNT(1) = BG_PRIO(0) | BG_CBB(0) | BG_SBB(28) | 0xC000;
    PAL_BG[0] = meadow_pal[1];

    dma3_copy32(OBJ_TILES + T_PIP * 16, pip_tiles, sizeof pip_tiles);
    dma3_copy16(PAL_OBJ + P_PIP * 16, pip_pal, sizeof pip_pal);
    dma3_copy32(OBJ_TILES + T_SHADOW * 16, shadow16_tiles, sizeof shadow16_tiles);
    dma3_copy16(PAL_OBJ + P_SHADOW * 16, shadow16_pal, sizeof shadow16_pal);

    door_cool = 30;          /* do not walk straight back in */
    dbg("scene meadow pip=%d,%d", (int)(pip_x >> 8), (int)(pip_y >> 8));
    update_camera();
    scene_blend(BLD_BG2 << 8, 5 | (11 << 8));
    REG_DISPCNT = DCNT_MODE0 | DCNT_BG1 | DCNT_BG2 | DCNT_OBJ | DCNT_OBJ_1D;
}

static void move(int dx, int dy) {
    int fx = pip_x >> 8, fy = pip_y >> 8;
    if (dx) {
        s32 nx = pip_x + dx;
        if (!blocked(nx >> 8, fy)) pip_x = nx;
    }
    fx = pip_x >> 8;
    if (dy) {
        s32 ny = pip_y + dy;
        if (!blocked(fx, ny >> 8)) pip_y = ny;
    }
}

static void draw(void) {
    static const u8 walk_down[4] = {1, 0, 2, 0};
    int frame, hflip = 0;
    bool walking = walk_t > 0;
    int step = (walk_t / 8) & 3;
    switch (dir) {
    case DIR_UP:
        frame = 3 + (walking ? walk_down[step] : 0);
        break;
    case DIR_RIGHT:
    case DIR_LEFT:
        frame = 6 + (walking ? (step & 1) : 0);
        hflip = dir == DIR_LEFT;
        break;
    default:
        frame = walking ? walk_down[step] : 0;
    }
    int bob = (walking && (step & 1)) ? 1 : 0;
    int sx = (pip_x >> 8) - cam_x, sy = (pip_y >> 8) - cam_y;
    oam[0].attr0 = A0_Y(sy - PIP_FEET_ROW - 1 - bob) | A0_TALL;
    oam[0].attr1 = A1_X(sx - 8) | A1_SIZE(2) | (hflip ? A1_HFLIP : 0);
    oam[0].attr2 = A2_TILE(T_PIP + frame * 8) | A2_PRIO(1) | A2_PAL(P_PIP);
    oam[1].attr0 = A0_Y(sy - 5) | A0_WIDE | A0_BLEND;
    oam[1].attr1 = A1_X(sx - 8) | A1_SIZE(0);
    oam[1].attr2 = A2_TILE(T_SHADOW) | A2_PRIO(1) | A2_PAL(P_SHADOW);
}

/* water highlights glint softly: cycle the shimmer color every 12 frames */
static void shimmer(void) {
    if ((frame_count % 12) != 0) return;
    u16 c = water_shimmer_cycle[(frame_count / 12) & 3];
    for (int i = 0; i < MEADOW_NSHIMMER; i++) PAL_BG[meadow_shimmer[i]] = c;
}

static void update(void) {
    u16 held = key_held();
    shimmer();
    int dx = 0, dy = 0;
    if (held & KEY_LEFT) { dx = -SPEED; dir = DIR_LEFT; }
    if (held & KEY_RIGHT) { dx = SPEED; dir = DIR_RIGHT; }
    if (held & KEY_UP) { dy = -SPEED; dir = DIR_UP; }
    if (held & KEY_DOWN) { dy = SPEED; dir = DIR_DOWN; }
    if (dx && dy) {           /* keep diagonal speed the same */
        dx = dx * 181 / 256;
        dy = dy * 181 / 256;
    }
    if (dx || dy) {
        move(dx, dy);
        walk_t++;
    } else {
        walk_t = 0;
    }
    update_camera();

    if (door_cool > 0) door_cool--;
    int px = pip_x >> 8, py = pip_y >> 8;
    for (int i = 0; i < MEADOW_NDOORS; i++) {
        const u16 *d = &meadow_doors[i * 4];
        if (!door_cool && px >= d[0] && px < d[0] + d[2] && py >= d[1] && py < d[1] + d[3] + 12 &&
            (held & KEY_UP)) {
            sfx_chime(3);
            dbg("door %d", i);
            pip_y = (d[1] + d[3] + 16) << 8;        /* step back out when we return */
            dir = DIR_DOWN;
            scene_go(&scene_shelf);
        }
    }
    if (key_hit() & KEY_START) {
        sfx_chime(2);
        dbg("meadow start pip=%d,%d", px, py);
        scene_go(&scene_shelf);
    }
    if (key_hit() & (KEY_A | KEY_B)) sfx_tick();
    draw();
}

const Scene scene_meadow_view = {enter, update};
