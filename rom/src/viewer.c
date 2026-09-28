/* Blossom Meadow: Pip walks around the full-size map (2x2 screens).
 * Ground layer under Pip, overlay (tree tops, roof) over Pip, collision
 * from the exported 8x8 solid grid. The cottage door opens the shelf.
 * Gift boxes wait at the map's container spots; touching one shows a
 * bouncing A button and A or B opens it. After a while without finding
 * one, a guide arrow points the way. */
#include "collection.h"
#include "game.h"
#include "game_assets.h"
#include "sound.h"
#include "system.h"

#define T_PIP     0        /* 8 frames x 8 tiles */
#define T_SHADOW  64
#define T_BOX     66       /* 16x16, one tile set, a palette per color */
#define T_SPARK   70       /* twinkle tiny/small/big, heart */
#define T_ABTN    74       /* 16x32 */
#define T_ARROW   82       /* right, up, upright: 16x16 each */
#define P_PIP     0
#define P_SHADOW  1
#define P_BOX     2        /* 2..6 */
#define P_SPARK   7
#define P_ABTN    8
#define P_ARROW   9

#define MAX_BOXES   3
#define ARROW_DELAY (15 * 60)   /* frames without an open before the arrow shows */
#define RESPAWN     90          /* frames before a new box replaces an opened one */
#define TOUCH       6           /* px around the box that count as touching */

#define SPEED     320      /* 8.8 fixed: 1.25 px per frame */
#define FEET_W    5        /* half width of Pip's feet box */
#define FEET_H    5

enum { DIR_DOWN, DIR_UP, DIR_RIGHT, DIR_LEFT };

static s32 pip_x = MEADOW_SPAWN_X << 8, pip_y = MEADOW_SPAWN_Y << 8;
static int dir = DIR_DOWN, walk_t, cam_x, cam_y, door_cool;

/* ---- gift boxes (kept while other scenes run) ---- */
typedef struct {
    s8 spot;          /* index into meadow_spots, -1 = empty */
    u8 color;
    u8 near;          /* chimed for this approach */
    s16 wait;         /* frames until an empty slot gets a new box */
    u16 phase;        /* animation offset */
} Box;

static Box boxes[MAX_BOXES];
static bool boxes_ready;
static int seek_t;                 /* frames since the last open */
static int arrow_box = -1;         /* box the arrow points at, -1 = none */
static int touch_box = -1;

void meadow_reset(void) {
    pip_x = MEADOW_SPAWN_X << 8;
    pip_y = MEADOW_SPAWN_Y << 8;
    dir = DIR_DOWN;
    boxes_ready = false;
    seek_t = 0;
    arrow_box = touch_box = -1;
}

static inline int spot_x(int s) { return meadow_spots[s * 2]; }
static inline int spot_y(int s) { return meadow_spots[s * 2 + 1]; }

static int boxes_wanted(void) {
    int left = 20 - found_in_area(0);
    return left < MAX_BOXES ? left : MAX_BOXES;
}

static int boxes_out(void) {
    int n = 0;
    for (int i = 0; i < MAX_BOXES; i++) n += boxes[i].spot >= 0;
    return n;
}

static bool spot_used(int s) {
    for (int i = 0; i < MAX_BOXES; i++)
        if (boxes[i].spot == s) return true;
    return false;
}

static bool color_used(int c) {
    for (int i = 0; i < MAX_BOXES; i++)
        if (boxes[i].spot >= 0 && boxes[i].color == c) return true;
    return false;
}

/* Picks a free spot away from Pip, preferring one off screen so boxes do not
 * pop up under the child's nose. The very first box of a game sits in view
 * near the cottage. */
static int choose_spot(void) {
    int px = pip_x >> 8, py = pip_y >> 8;
    if (game_save.opens == 0 && boxes_out() == 0 && !spot_used(MEADOW_FIRST_SPOT)) return MEADOW_FIRST_SPOT;
    int best = -1, best_score = -1;
    for (int s = 0; s < MEADOW_NSPOTS; s++) {
        if (spot_used(s)) continue;
        int dx = spot_x(s) - px, dy = spot_y(s) - py;
        if (dx * dx + dy * dy < 48 * 48) continue;
        bool on_screen = spot_x(s) >= cam_x - 8 && spot_x(s) < cam_x + SCREEN_W + 8 &&
                         spot_y(s) >= cam_y && spot_y(s) < cam_y + SCREEN_H + 16;
        int score = (on_screen ? 0 : 100) + (int)(game_rand() % 64);
        if (score > best_score) { best_score = score; best = s; }
    }
    return best;
}

static void place_box(int i) {
    int s = choose_spot();
    if (s < 0) return;
    int c = (int)(game_rand() % BOX_COLORS);
    for (int k = 0; k < BOX_COLORS && color_used(c); k++) c = (c + 1) % BOX_COLORS;
    boxes[i] = (Box){(s8)s, (u8)c, 0, 0, (u16)(game_rand() & 255)};
    dbg("box %d at %d,%d color %d", i, spot_x(s), spot_y(s), c);
}

static void boxes_fill(void) {
    for (int i = 0; i < MAX_BOXES && boxes_out() < boxes_wanted(); i++)
        if (boxes[i].spot < 0 && boxes[i].wait == 0) place_box(i);
}

/* box footprint: 14 px wide, 6 px deep at its base */
static bool box_hit(int i, int x0, int y0, int x1, int y1) {
    if (boxes[i].spot < 0) return false;
    int bx = spot_x(boxes[i].spot), by = spot_y(boxes[i].spot);
    return x1 >= bx - 7 && x0 <= bx + 7 && y1 >= by - 6 && y0 <= by;
}

static bool solid_at(int x, int y) {
    if (x < 0 || y < 0 || x >= MEADOW_W || y >= MEADOW_H) return true;
    return meadow_solid[(y >> 3) * (MEADOW_W >> 3) + (x >> 3)];
}

static bool blocked(int fx, int fy) {
    if (solid_at(fx - FEET_W, fy - FEET_H) || solid_at(fx + FEET_W, fy - FEET_H) ||
        solid_at(fx - FEET_W, fy) || solid_at(fx + FEET_W, fy))
        return true;
    for (int i = 0; i < MAX_BOXES; i++)
        if (box_hit(i, fx - FEET_W, fy - FEET_H, fx + FEET_W, fy)) return true;
    return false;
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
    dma3_copy32(OBJ_TILES + T_BOX * 16, box16_tiles, sizeof box16_tiles);
    dma3_copy16(PAL_OBJ + P_BOX * 16, box16_pal, sizeof box16_pal);
    dma3_copy32(OBJ_TILES + T_SPARK * 16, ui_small_tiles, sizeof ui_small_tiles);
    dma3_copy16(PAL_OBJ + P_SPARK * 16, ui_small_pal, sizeof ui_small_pal);
    dma3_copy32(OBJ_TILES + T_ABTN * 16, abubble_tiles, sizeof abubble_tiles);
    dma3_copy16(PAL_OBJ + P_ABTN * 16, abubble_pal, sizeof abubble_pal);
    dma3_copy32(OBJ_TILES + T_ARROW * 16, arrow_tiles, sizeof arrow_tiles);
    dma3_copy16(PAL_OBJ + P_ARROW * 16, arrow_pal, sizeof arrow_pal);

    door_cool = 30;          /* do not walk straight back in */
    dbg("scene meadow pip=%d,%d", (int)(pip_x >> 8), (int)(pip_y >> 8));
    update_camera();
    if (!boxes_ready) {
        for (int i = 0; i < MAX_BOXES; i++) boxes[i] = (Box){-1, 0, 0, 0, 0};
        boxes_ready = true;
    }
    boxes_fill();
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

/* ---- sprites: UI first (on top), then world sprites sorted front to back ---- */
typedef struct { s16 y; u16 a0, a1, a2; } Spr;
static Spr world[16];
static int n_world, n_oam;

static bool on_screen(int sx, int sy, int w, int h) {
    return sx > -w && sx < SCREEN_W && sy > -h && sy < SCREEN_H;
}

static void ui_spr(int sx, int sy, u16 shape, int size, u16 flip, int tile, int pal) {
    if (n_oam >= 128) return;
    oam[n_oam].attr0 = A0_Y(sy) | shape;
    oam[n_oam].attr1 = A1_X(sx) | A1_SIZE(size) | flip;
    oam[n_oam].attr2 = A2_TILE(tile) | A2_PRIO(1) | A2_PAL(pal);
    n_oam++;
}

static void world_spr(int depth, int sx, int sy, u16 shape, int size, u16 flip, int tile, int pal) {
    if (n_world >= 16) return;
    Spr *w = &world[n_world++];
    w->y = (s16)depth;
    w->a0 = A0_Y(sy) | shape;
    w->a1 = A1_X(sx) | A1_SIZE(size) | flip;
    w->a2 = A2_TILE(tile) | A2_PRIO(1) | A2_PAL(pal);
}

static void flush_world(void) {
    for (int i = 1; i < n_world; i++) {          /* insertion sort, larger y (nearer) first */
        Spr t = world[i];
        int j = i;
        while (j > 0 && world[j - 1].y < t.y) { world[j] = world[j - 1]; j--; }
        world[j] = t;
    }
    for (int i = 0; i < n_world && n_oam < 128; i++, n_oam++) {
        oam[n_oam].attr0 = world[i].a0;
        oam[n_oam].attr1 = world[i].a1;
        oam[n_oam].attr2 = world[i].a2;
    }
    for (int i = n_oam; i < 128; i++) oam[i].attr0 = A0_HIDE;
}

static void draw_pip(void) {
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
    int py = pip_y >> 8;
    int sx = (pip_x >> 8) - cam_x, sy = py - cam_y;
    world_spr(py, sx - 8, sy - PIP_FEET_ROW - 1 - bob, A0_TALL, 2, hflip ? A1_HFLIP : 0, T_PIP + frame * 8, P_PIP);
    world_spr(py - 1, sx - 8, sy - 5, A0_WIDE | A0_BLEND, 0, 0, T_SHADOW, P_SHADOW);
}

static void draw_boxes(void) {
    for (int i = 0; i < MAX_BOXES; i++) {
        Box *b = &boxes[i];
        if (b->spot < 0) continue;
        int bx = spot_x(b->spot), by = spot_y(b->spot);
        int sx = bx - cam_x, sy = by - cam_y;
        if (!on_screen(sx - 8, sy - 16, 16, 24)) continue;
        /* a small hop every few seconds, staggered per box */
        int t = (int)((frame_count + b->phase) % 160);
        int hop = t < 16 ? isin(t * 2) * 3 / 256 : 0;
        world_spr(by, sx - 8, sy - 15 - hop, A0_SQUARE, 1, 0, T_BOX, P_BOX + b->color);
        world_spr(by - 1, sx - 8, sy - 5, A0_WIDE | A0_BLEND, 0, 0, T_SHADOW, P_SHADOW);
        /* one twinkle walks around the box: tiny, small, big, small */
        static const s8 around[6][2] = {{-9, -14}, {7, -17}, {9, -6}, {-3, -20}, {-11, -4}, {4, -12}};
        int k = (int)((frame_count + b->phase) / 24) % 6;
        int f = (int)((frame_count + b->phase) / 6) % 4;
        ui_spr(sx + around[k][0] - 4, sy + around[k][1] - 4, A0_SQUARE, 0, 0, T_SPARK + (f == 3 ? 1 : f), P_SPARK);
    }
    if (touch_box >= 0) {
        int bx = spot_x(boxes[touch_box].spot), by = spot_y(boxes[touch_box].spot);
        int bob = isin((int)(frame_count * 2)) * 2 / 256;
        ui_spr(bx - cam_x - 8, by - cam_y - 36 + bob, A0_TALL, 2, 0, T_ABTN, P_ABTN);
    }
}

/* Guide arrow: over the box when it is on screen, else at the screen edge
 * pointing from Pip toward it. 8 directions from 3 drawings and flips. */
static void draw_arrow(void) {
    if (arrow_box < 0 || boxes[arrow_box].spot < 0 || touch_box >= 0) return;
    int bx = spot_x(boxes[arrow_box].spot) - cam_x, by = spot_y(boxes[arrow_box].spot) - cam_y;
    int bob = (isin((int)(frame_count * 2)) + 256) * 3 / 512;
    if (bx >= 8 && bx < SCREEN_W - 8 && by >= 28 && by < SCREEN_H + 8) {
        ui_spr(bx - 8, by - 38 + bob, A0_SQUARE, 1, A1_VFLIP, T_ARROW + 4, P_ARROW);   /* points down */
        return;
    }
    int dx = bx - ((pip_x >> 8) - cam_x), dy = by - ((pip_y >> 8) - cam_y - 12);
    int ax = dx < 0 ? -dx : dx, ay = dy < 0 ? -dy : dy;
    int tile;
    u16 flip = 0;
    if (ax * 5 > ay * 12) {
        tile = 0;
        if (dx < 0) flip = A1_HFLIP;
    } else if (ay * 5 > ax * 12) {
        tile = 1;
        if (dy > 0) flip = A1_VFLIP;
    } else {
        tile = 2;
        if (dx < 0) flip |= A1_HFLIP;
        if (dy > 0) flip |= A1_VFLIP;
    }
    int x = bx < 14 ? 14 : bx > SCREEN_W - 14 ? SCREEN_W - 14 : bx;
    int y = by < 14 ? 14 : by > SCREEN_H - 14 ? SCREEN_H - 14 : by;
    int nx = dx > 0 ? 1 : dx < 0 ? -1 : 0, ny = dy > 0 ? 1 : dy < 0 ? -1 : 0;
    if (tile == 0) ny = 0;
    if (tile == 1) nx = 0;
    ui_spr(x - 8 + nx * bob, y - 8 + ny * bob, A0_SQUARE, 1, flip, T_ARROW + tile * 4, P_ARROW);
}

static void draw(void) {
    n_world = n_oam = 0;
    draw_arrow();
    draw_boxes();
    draw_pip();
    flush_world();
}

/* Opens box i: rolls its friend and shows the open screen. */
static void open_box(int i) {
    open_friend = collection_roll(0, frame_count);
    open_color = boxes[i].color;
    boxes[i].spot = -1;
    boxes[i].wait = RESPAWN;
    touch_box = -1;
    arrow_box = -1;
    seek_t = 0;
    sfx_chime(3);
    dbg("box %d open friend %d pip=%d,%d", i, open_friend, (int)(pip_x >> 8), (int)(pip_y >> 8));
    scene_go(&scene_open);
}

static void update_boxes(u16 hit) {
    int px = pip_x >> 8, py = pip_y >> 8;
    for (int i = 0; i < MAX_BOXES; i++)
        if (boxes[i].spot < 0 && boxes[i].wait > 0) boxes[i].wait--;
    boxes_fill();

    int was_touching = touch_box;
    touch_box = -1;
    for (int i = 0; i < MAX_BOXES; i++) {
        Box *b = &boxes[i];
        if (b->spot < 0) continue;
        if (box_hit(i, px - FEET_W - TOUCH, py - FEET_H - TOUCH, px + FEET_W + TOUCH, py + TOUCH)) touch_box = i;
        int dx = spot_x(b->spot) - px, dy = spot_y(b->spot) - py;
        int d2 = dx * dx + dy * dy;
        if (!b->near && d2 < 56 * 56) {       /* soft chime as Pip comes close */
            b->near = 1;
            sfx_chime(4);
        } else if (b->near && d2 > 96 * 96) {
            b->near = 0;
        }
    }

    if (touch_box >= 0 && touch_box != was_touching) dbg("touch box %d pip=%d,%d", touch_box, px, py);

    if (seek_t < ARROW_DELAY) {
        seek_t++;
    } else {                       /* the arrow always points at the nearest box */
        int best = 0x7FFFFFFF, near = -1;
        for (int i = 0; i < MAX_BOXES; i++) {
            if (boxes[i].spot < 0) continue;
            int dx = spot_x(boxes[i].spot) - px, dy = spot_y(boxes[i].spot) - py;
            if (dx * dx + dy * dy < best) { best = dx * dx + dy * dy; near = i; }
        }
        if (near != arrow_box) {
            if (arrow_box < 0 && near >= 0) sfx_chime(2);
            arrow_box = near;
            dbg("arrow on box %d", arrow_box);
        }
    }

    if (touch_box >= 0 && (hit & (KEY_A | KEY_B))) open_box(touch_box);
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
    u16 hit = key_hit();
    update_boxes(hit);
    if ((hit & (KEY_A | KEY_B)) && touch_box < 0) sfx_tick();
    draw();
}

const Scene scene_meadow_view = {enter, update};
