/* The walkable areas (Blossom Meadow first): Pip walks around a full-size
 * map (2x2 screens) described by area_maps[game_save.area].
 * Ground layer under Pip, overlay (tree tops, roof) over Pip, collision
 * from the exported solid grid (4x4 px cells). The cottage door opens the shelf.
 * Gift boxes wait at the map's container spots; touching one shows a
 * bouncing A button and A or B opens it. After a while without finding
 * one, a guide arrow points the way. */
#include "collection.h"
#include "game.h"
#include "game_assets.h"
#include "music_data.h"
#include "squishy.h"
#include "sound.h"
#include "system.h"
#include "text.h"

#define T_PIP     0        /* 8 frames x 8 tiles */
#define T_SHADOW  64
#define T_BOX     66       /* the area's container, 16x16: one tile set, a palette per color */
#define T_SPARK   70       /* twinkle tiny/small/big, heart */
#define T_ABTN    74       /* 16x32 */
#define T_ARROW   82       /* right, up, upright: 16x16 each */
#define T_SQ16    94       /* the area's 4 species x (idle, squish) x 4 tiles */
#define P_PIP     0
#define P_SHADOW  1
#define P_BOX     2        /* 2..6 */
#define P_SPARK   7
#define P_ABTN    8
#define P_ARROW   9
#define T_COUNT   128      /* found counter: pill with icon (two 32x16), then text 32x16 */
#define T_COUNT_TXT (T_COUNT + 16)
#define T_NPC     160      /* Momo: asleep, awake (32x32 each), two z's; palette P_SHADOW */
#define T_NPC_TXT 232      /* "7/10" in Momo's bubble, 32x16 (after Momo's 66 tiles) */
#define T_EXTRA   240      /* snacks, envelope, flag up / down (16x16 each), palette P_SPARK */
#define P_SQ      10       /* 10..14: the area's 5 flavor palettes */
#define P_COUNT   15

#define MAX_BOXES   3
#define ARROW_DELAY (5 * 60)    /* frames without an open before the arrow shows */
#define ARROW_R     26          /* px from Pip's middle to the arrow's middle */
#define ARROW_BLINK 60          /* frames on, then the same off */
#define RESPAWN     90          /* frames before a new box replaces an opened one */
#define TOUCH       6           /* px around the box that count as touching */
#define MAX_PEN     20          /* every friend of an area fits in its pen */
#define PEN_TRIES   10           /* random spots tried for each new pen target */
#define PEN_ROOM    13          /* px: pen friends do not walk closer than this */
#define TRAIL       64          /* Pip's recent steps, for followers */
#define TRAIL_GAP   14          /* steps between followers in the line */

#define SPEED     320      /* 8.8 fixed: 1.25 px per frame */
#define FEET_W    5        /* half width of Pip's feet box */
#define FEET_H    5

enum { DIR_DOWN, DIR_UP, DIR_RIGHT, DIR_LEFT };

static const AreaMap *A = &area_maps[0];   /* the area Pip is in */
static const u8 area_song[AREA_COUNT] = {SONG_MEADOW, SONG_WOODS, SONG_SHORE, SONG_CLOUD};
static s32 pip_x = MEADOW_SPAWN_X << 8, pip_y = MEADOW_SPAWN_Y << 8;
static int travel_to = -1, travel_x, travel_y;   /* set by an exit: the next enter() arrives there */
static int dir = DIR_DOWN, walk_t, cam_x, cam_y, door_cool;

/* ---- gift boxes (kept while other scenes run) ---- */
typedef struct {
    s8 spot;          /* index into the area's spots, -1 = empty */
    u8 color;
    s16 wait;         /* frames until an empty slot gets a new box */
    u16 phase;        /* animation offset */
} Box;

static Box boxes[MAX_BOXES];
static bool boxes_ready;           /* false: place new boxes (new game, other area) */
static int seek_t;                 /* frames since the last open */
static int arrow_box = -1;         /* box the arrow points at, -1 = none */
static int arrow_t;                /* frames since the arrow showed, for the blink */
static int touch_box = -1;
EWRAM_BSS static TextStrip st_count, st_npc;   /* rendered into OBJ tiles: the meadow has no free BG palette */
static int count_last = -1, count_hop;   /* the counter hops when a new friend is counted */
static bool at_sign;               /* Pip stands by the pen sign: A opens the picker */
static bool at_mail, at_basket;    /* by the mailbox (A reads the letter) or the picnic basket (A: snack time) */
static bool pos_restored;          /* the saved position is used once, on the first visit after boot */
static int still_t;                /* frames Pip stood still since the last step */
static bool pos_dirty;             /* Pip moved since the position was last saved */
#define POS_SAVE_WAIT 60           /* save the position after Pip stands still this long */

void meadow_reset(void) {
    A = &area_maps[0];
    pip_x = A->spawn_x << 8;
    pip_y = A->spawn_y << 8;
    dir = DIR_DOWN;
    boxes_ready = false;
    seek_t = 0;
    arrow_box = touch_box = -1;
    pos_restored = true;               /* a new game starts at the house */
}

/* ---- friends: a line behind Pip, the rest roam in the pen ---- */
typedef struct {
    s16 x, y, tx, ty, wait;
    u8 id, step, stuck;
} Roamer;

static Roamer pen[MAX_PEN];
static int n_pen, n_follow, follow_id[MAX_FOLLOWERS];
static s16 trail_x[TRAIL], trail_y[TRAIL];
static int trail_i;
static u32 pen_seed = 12345;

static int pen_rand(int n) {          /* local: the pen must not use up the saved random numbers */
    pen_seed = pen_seed * 1103515245u + 12345u;
    return (int)((pen_seed >> 16) % (u32)n);
}

/* Picks the roomiest of a few random spots: the one farthest from where the
 * other friends stand or are going, so 17 friends spread out instead of piling up. */
static void pen_target(Roamer *r) {
    int best = -1;
    for (int k = 0; k < PEN_TRIES; k++) {
        int x = A->pen_x0 + pen_rand(A->pen_x1 - A->pen_x0);
        int y = A->pen_y0 + pen_rand(A->pen_y1 - A->pen_y0);
        int room = 1 << 30;
        for (int i = 0; i < n_pen; i++) {
            const Roamer *o = &pen[i];
            if (o == r) continue;
            int dx = x - o->x, dy = y - o->y, d = dx * dx + dy * dy;
            if (d < room) room = d;
            dx = x - o->tx, dy = y - o->ty, d = dx * dx + dy * dy;
            if (d < room) room = d;
        }
        if (room > best) {
            best = room;
            r->tx = (s16)x;
            r->ty = (s16)y;
        }
    }
}

static bool blocked(int fx, int fy);

/* Lays the follower line out behind Pip (away from where Pip faces), so the
 * friends stand in a row instead of on Pip's feet. Tries the other sides
 * when the way back is blocked; stacks them on Pip only when all are. */
static void trail_line(void) {
    static const s8 back[4][2] = {{0, -1}, {0, 1}, {-1, 0}, {1, 0}};   /* by dir: down, up, right, left */
    static const u8 order[4][4] = {{0, 2, 3, 1}, {1, 2, 3, 0}, {2, 0, 1, 3}, {3, 0, 1, 2}};
    int px = pip_x >> 8, py = pip_y >> 8, bx = 0, by = 0;
    for (int o = 0; o < 4; o++) {
        int d = order[dir][o];
        bool clear = true;
        for (int k = 1; k <= MAX_FOLLOWERS && clear; k++) {
            int s = k * TRAIL_GAP * SPEED >> 8;
            for (int t = 4; t <= s && clear; t += 4) clear = !blocked(px + back[d][0] * t, py + back[d][1] * t);
        }
        if (clear) {
            bx = back[d][0];
            by = back[d][1];
            break;
        }
    }
    trail_i = 0;
    for (int i = 0; i < TRAIL; i++) {            /* entry i steps back sits i steps along the line */
        int t = (trail_i - i) & (TRAIL - 1), s = i * SPEED >> 8;
        trail_x[t] = (s16)(px + bx * s);
        trail_y[t] = (s16)(py + by * s);
    }
}

static void friends_setup(void) {
    int first = A->area * 20;
    for (int sp = 0; sp < 4; sp++) sq_load_frames(A->area * 4 + sp, 16, 0, 2, T_SQ16 + sp * 8);
    dma3_copy16(PAL_OBJ + P_SQ * 16, sq_area_pals[A->area], 5 * 32);
    pen_seed ^= frame_count;
    n_follow = 0;
    for (int i = 0; i < MAX_FOLLOWERS; i++) {
        int id = follower_get(A->area, i);
        if (id >= 0 && friend_found(id)) follow_id[n_follow++] = id;
    }
    n_pen = 0;
    for (int id = first; id < first + 20; id++) {
        bool following = false;
        for (int k = 0; k < n_follow; k++) following |= follow_id[k] == id;
        if (!friend_found(id) || following) continue;
        Roamer *r = &pen[n_pen];
        r->id = (u8)id;
        pen_target(r);                           /* compared only with the friends placed so far */
        r->x = r->tx;
        r->y = r->ty;
        r->wait = (s16)pen_rand(120);
        r->step = 0;
        r->stuck = 0;
        n_pen++;
        pen_target(r);
    }
    dbg("friends pen=%d follow=%d", n_pen, n_follow);
    trail_line();
}

/* True when a step to (nx,ny) takes r closer to a friend that is already near. */
static bool pen_crowded(const Roamer *r, int nx, int ny) {
    for (int i = 0; i < n_pen; i++) {
        const Roamer *o = &pen[i];
        if (o == r) continue;
        int dx = nx - o->x, dy = ny - o->y, d = dx * dx + dy * dy;
        int cx = r->x - o->x, cy = r->y - o->y;
        if (d < PEN_ROOM * PEN_ROOM && d < cx * cx + cy * cy) return true;
    }
    return false;
}

static void friends_update(bool pip_moved) {
    if (pip_moved) {
        trail_i = (trail_i + 1) & (TRAIL - 1);
        trail_x[trail_i] = (s16)(pip_x >> 8);
        trail_y[trail_i] = (s16)(pip_y >> 8);
    }
    for (int i = 0; i < n_pen; i++) {
        Roamer *r = &pen[i];
        if (r->wait > 0) {
            r->wait--;
            continue;
        }
        if ((frame_count & 1) == 0) {            /* half a pixel per frame: a calm waddle */
            int nx = r->x + (r->tx > r->x) - (r->tx < r->x);
            int ny = r->y + (r->ty > r->y) - (r->ty < r->y);
            if (pen_crowded(r, nx, ny) && nx != r->x && !pen_crowded(r, nx, r->y)) ny = r->y;   /* sidestep */
            else if (pen_crowded(r, nx, ny) && ny != r->y && !pen_crowded(r, r->x, ny)) nx = r->x;
            if (pen_crowded(r, nx, ny)) {        /* give way; find another spot if stuck */
                if (++r->stuck > 30) {
                    r->stuck = 0;
                    pen_target(r);
                }
            } else {
                r->x = (s16)nx;
                r->y = (s16)ny;
                r->step++;
                r->stuck = 0;
            }
        }
        if (r->x == r->tx && r->y == r->ty) {
            r->wait = (s16)(60 + pen_rand(180));
            r->step = 0;
            pen_target(r);
        }
    }
}

static inline int spot_x(int s) { return A->spots[s * 2]; }
static inline int spot_y(int s) { return A->spots[s * 2 + 1]; }

static int boxes_wanted(void) {
    int left = 20 - found_in_area(A->area);
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
    if (found_in_area(A->area) == 0 && boxes_out() == 0 && !spot_used(A->first_spot)) return A->first_spot;
    int best = -1, best_score = -1;
    for (int s = 0; s < A->nspots; s++) {
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
    boxes[i] = (Box){(s8)s, (u8)c, 0, (u16)(game_rand() & 255)};
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

/* The way into an area stays shut until the area before it has enough
 * friends (and until its map is built). */
static bool gate_shut(int to) { return to >= AREA_MAPS || !area_open(to); }
static bool gate_built(int to) { return to <= 0 || (game_save.gates & (1 << (to - 1))); }   /* the meadow has no gate */
static bool gate_there(int to) { return gate_shut(to) || !gate_built(to); }   /* the log still lies there */

static bool solid_at(int x, int y) {
    if (x < 0 || y < 0 || x >= A->w || y >= A->h) return true;
    return A->solid[(y >> A->cell_shift) * (A->w >> A->cell_shift) + (x >> A->cell_shift)];
}

static bool blocked(int fx, int fy) {
    if (solid_at(fx - FEET_W, fy - FEET_H) || solid_at(fx + FEET_W, fy - FEET_H) ||
        solid_at(fx - FEET_W, fy) || solid_at(fx + FEET_W, fy))
        return true;
    for (int i = 0; i < MAX_BOXES; i++)
        if (box_hit(i, fx - FEET_W, fy - FEET_H, fx + FEET_W, fy)) return true;
    for (int i = 0; i < A->ngates; i++) {
        const u16 *g = &A->gates[i * 5];
        if (gate_there(g[4]) && fx + FEET_W >= g[0] && fx - FEET_W < g[0] + g[2] && fy >= g[1] && fy - FEET_H < g[1] + g[3])
            return true;
    }
    return false;
}

/* ---- gate scene: the first time a way opens, the area's friends wake Momo, who moves on ---- */
#define GS_PAN     50       /* frames: camera glides to Momo */
#define GS_IN      95       /* helpers have hopped in */
#define GS_PUSH    165      /* three tickles; then Momo wakes and hops for joy */
#define GS_ROLL    215      /* Momo waddles off toward the next area */
#define GS_CHEER   255      /* sparkle, jingle, happy hops */
#define GS_BACK    305      /* camera glides back to Pip; the scene ends */
static struct {
    int t;                  /* frames since the start, -1 = none */
    int gate;               /* index in A->gates */
    int n, ids[3];          /* helpers: friends of this area */
} gs = {-1, 0, 0, {0}};

static inline int lerp(int a, int b, int t, int n) { return a + (b - a) * t / n; }

static void gate_center(int *x, int *y) {   /* where Momo's feet are */
    const u16 *g = &A->gates[gs.gate * 5];
    *x = g[0] + g[2] / 2;
    *y = g[1] + g[3];
}

static void gate_scene_start(int i) {
    gs.t = 0;
    gs.gate = i;
    gs.n = 0;
    for (int k = 0; k < n_pen && gs.n < 3; k++) gs.ids[gs.n++] = pen[k].id;       /* pen friends first */
    for (int k = 0; k < n_follow && gs.n < 3; k++) gs.ids[gs.n++] = follow_id[k];
    dbg("gate scene %d start helpers=%d", A->gates[i * 5 + 4], gs.n);
}

static void update_camera(void) {
    int px = pip_x >> 8, py = pip_y >> 8;
    if (gs.t >= 0) {                             /* the scene moves the camera between Pip and the log */
        int gx, gy, t = gs.t;
        gate_center(&gx, &gy);
        gy -= 16;
        if (t < GS_PAN) { px = lerp(px, gx, t, GS_PAN); py = lerp(py, gy + 12, t, GS_PAN); }
        else if (t < GS_CHEER) { px = gx; py = gy + 12; }
        else { px = lerp(gx, px, t - GS_CHEER, GS_BACK - GS_CHEER); py = lerp(gy + 12, py, t - GS_CHEER, GS_BACK - GS_CHEER); }
    }
    int tx = px - SCREEN_W / 2, ty = py - 12 - SCREEN_H / 2;
    if (tx < 0) tx = 0;
    if (ty < 0) ty = 0;
    if (tx > A->w - SCREEN_W) tx = A->w - SCREEN_W;
    if (ty > A->h - SCREEN_H) ty = A->h - SCREEN_H;
    cam_x = tx;
    cam_y = ty;
    for (int i = 1; i <= 2; i++) {
        bg_scroll_x[i] = (s16)cam_x;
        bg_scroll_y[i] = (s16)cam_y;
    }
}

static bool blocked(int fx, int fy);

/* Continue: put Pip back where the save says, once per boot, if it is walkable. */
static void restore_pos(void) {
    if (pos_restored) return;
    pos_restored = true;
    int x = game_save.pip_x, y = game_save.pip_y;
    if (!x && !y) {                            /* no spot saved: the area's start */
        x = A->spawn_x;
        y = A->spawn_y;
    }
    if (x < A->w && y < A->h && !blocked(x, y)) {
        pip_x = x << 8;
        pip_y = y << 8;
        dbg("restore pip=%d,%d", x, y);
    }
}

/* Keeps the save's position up to date without writing SRAM on every step. */
static void save_pos(bool now) {
    if (!pos_dirty || (!now && still_t < POS_SAVE_WAIT)) return;
    pos_dirty = false;
    game_save.pip_x = (u16)(pip_x >> 8);
    game_save.pip_y = (u16)(pip_y >> 8);
    collection_save();
    dbg("save pip=%d,%d", game_save.pip_x, game_save.pip_y);
}

static void enter(void) {
    if (travel_to >= 0) {                      /* walked in from another area */
        game_save.area = (u8)travel_to;
        pip_x = travel_x << 8;
        pip_y = travel_y << 8;
        travel_to = -1;
        pos_restored = true;
        pos_dirty = true;
    }
    const AreaMap *was = A;
    A = &area_maps[game_save.area < AREA_MAPS ? game_save.area : 0];
    if (A != was) boxes_ready = false;         /* boxes belong to the area they were placed in */
    restore_pos();
    music_play(area_song[A->area]);  /* after the shelf or a box it goes on where it was */
    dma3_copy32(CHARBLOCK(0), A->tiles, A->tiles_bytes);
    dma3_copy16(PAL_BG, A->pal, A->pal_bytes);
    dma3_copy32(SCREENBLOCK(24), A->ground, 4096 * 2);
    dma3_copy32(SCREENBLOCK(28), A->overlay, 4096 * 2);
    REG_BGCNT(2) = BG_PRIO(2) | BG_CBB(0) | BG_SBB(24) | 0xC000;     /* 64x64 */
    REG_BGCNT(1) = BG_PRIO(0) | BG_CBB(0) | BG_SBB(28) | 0xC000;
    PAL_BG[0] = A->pal[1];

    dma3_copy32(OBJ_TILES + T_PIP * 16, pip_tiles, sizeof pip_tiles);
    dma3_copy16(PAL_OBJ + P_PIP * 16, pip_pal, sizeof pip_pal);
    dma3_copy32(OBJ_TILES + T_SHADOW * 16, shadow16_tiles, sizeof shadow16_tiles);
    dma3_copy16(PAL_OBJ + P_SHADOW * 16, shadow16_pal, sizeof shadow16_pal);
    dma3_copy32(OBJ_TILES + T_BOX * 16, cont16_tiles + A->area * 32, 4 * 32);
    dma3_copy16(PAL_OBJ + P_BOX * 16, cont16_pal + A->area * 5 * 16, 5 * 32);
    dma3_copy32(OBJ_TILES + T_NPC * 16, npc_tiles, sizeof npc_tiles);
    dma3_copy32(OBJ_TILES + T_SPARK * 16, ui_small_tiles, sizeof ui_small_tiles);
    dma3_copy32(OBJ_TILES + T_EXTRA * 16, ui_extra_tiles, sizeof ui_extra_tiles);
    dma3_copy16(PAL_OBJ + P_SPARK * 16, ui_small_pal, sizeof ui_small_pal);
    dma3_copy32(OBJ_TILES + T_ABTN * 16, abubble_tiles, sizeof abubble_tiles);
    dma3_copy16(PAL_OBJ + P_ABTN * 16, abubble_pal, sizeof abubble_pal);
    dma3_copy32(OBJ_TILES + T_ARROW * 16, arrow_tiles, sizeof arrow_tiles);
    dma3_copy16(PAL_OBJ + P_ARROW * 16, arrow_pal, sizeof arrow_pal);

    char buf[8];             /* found counter, top right: area icon and "7/20" */
    int n = found_in_area(A->area), k = 0;
    if (count_last >= 0 && n > count_last) count_hop = 40;
    count_last = n;
    if (n >= 10) buf[k++] = (char)('0' + n / 10);
    buf[k++] = (char)('0' + n % 10);
    buf[k++] = '/'; buf[k++] = '2'; buf[k++] = '0'; buf[k] = 0;
    dma3_copy32(OBJ_TILES + T_COUNT * 16, count_pill_tiles + A->area * 16 * 8, 16 * 32);   /* the area's container icon */
    dma3_copy16(PAL_OBJ + P_COUNT * 16, count_pill_pal + A->area * 16, 32);
    st_count.tw = 4; st_count.th = 2; st_count.cbb = 4; st_count.first_tile = T_COUNT_TXT;
    strip_print(&st_count, buf, 1, 4, 1, 0);        /* centred after the icon */
    k = 0;                   /* Momo's bubble: friends found here out of the GATE_NEED that wake Momo */
    int need = n < GATE_NEED ? n : GATE_NEED;
    if (need >= 10) buf[k++] = (char)('0' + need / 10);
    buf[k++] = (char)('0' + need % 10);
    buf[k++] = '/'; buf[k++] = (char)('0' + GATE_NEED / 10); buf[k++] = (char)('0' + GATE_NEED % 10); buf[k] = 0;
    st_npc.tw = 4; st_npc.th = 2; st_npc.cbb = 4; st_npc.first_tile = T_NPC_TXT;
    strip_print(&st_npc, buf, 1, 4, 1, 0);

    door_cool = 30;          /* do not walk straight back in */
    dbg("scene meadow pip=%d,%d area=%d", (int)(pip_x >> 8), (int)(pip_y >> 8), A->area);
    update_camera();
    if (!boxes_ready) {
        for (int i = 0; i < MAX_BOXES; i++) boxes[i] = (Box){-1, 0, 0, 0};
        boxes_ready = true;
    }
    boxes_fill();
    friends_setup();
    if (pos_dirty) save_pos(true);             /* arrived from another area: remember it */
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
static Spr world[64];
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

/* Screen overlays (counter, A bubble, arrow) sit over every BG layer, trees too. */
static void hud_spr(int sx, int sy, u16 shape, int size, u16 flip, int tile, int pal) {
    if (n_oam >= 128) return;
    ui_spr(sx, sy, shape, size, flip, tile, pal);
    oam[n_oam - 1].attr2 = (u16)((oam[n_oam - 1].attr2 & ~A2_PRIO(3)) | A2_PRIO(0));
}

static void world_spr(int depth, int sx, int sy, u16 shape, int size, u16 flip, int tile, int pal) {
    if (n_world >= 64) return;
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

static int hop_of(int step);

/* ---- snack time at the picnic basket: a treat pops out onto the blanket,
 * the followers hop over, share it with a squeak each, then go back ---- */
#define SN_LAND    24       /* the treat has landed on the blanket */
#define SN_THERE   44       /* the followers stand round it */
#define SN_GONE    104      /* eaten: a twinkle where it was */
#define SN_BACK    134      /* the followers are back in line; the snack ends */
#define SN_COOL    60       /* frames before the next snack */
static struct {
    int t;                  /* frames since A, -1 = none */
    int kind, cool;
} snack = {-1, 0, 0};
static int snack_next = -1;         /* the next treat: a different one each time */
static int pip_hop;                 /* px Pip is lifted (hops for joy, no followers) */

static void snack_spot(int *x, int *y) {   /* where the treat sits: on the blanket, left of the basket */
    *x = A->basket_x - 22;
    *y = A->basket_y + 4;
}

/* follower k's place round the treat (left, behind, in front): clear of
 * the basket and of Pip, who stands by the basket */
static void snack_seat(int k, int *x, int *y) {
    static const s8 seat[3][2] = {{-17, 1}, {1, -11}, {-7, 12}};
    snack_spot(x, y);
    *x += seat[k][0];
    *y += seat[k][1];
}

static void snack_start(void) {
    if (snack_next < 0) snack_next = pen_rand(SNACK_KINDS);
    snack.kind = snack_next;
    snack_next = (snack_next + 1) % SNACK_KINDS;
    snack.t = 0;
    sfx_boing();
    sfx_chime(4);
    dbg("snack %s followers=%d", snack_names[snack.kind], n_follow);
}

static void snack_update(void) {
    int t = snack.t;
    if (t == SN_LAND) sfx_chime(2);
    if (n_follow) {
        for (int k = 0; k < n_follow; k++)                         /* one bite each, in turn */
            if (t == SN_THERE + 6 + k * 16) sfx_squeak(friend_flavor(follow_id[k]));
    } else if (t == SN_THERE || t == SN_THERE + 20) {
        sfx_boing();                                               /* Pip hops and eats it */
    }
    if (t == SN_GONE) sfx_chime(5);
    pip_hop = 0;
    if (!n_follow && t >= SN_THERE && t < SN_THERE + 40) {
        int h = (t - SN_THERE) % 20;
        pip_hop = h < 14 ? h * (14 - h) / 8 : 0;
    }
    int end = n_follow ? SN_BACK : SN_GONE + 20;
    if (++snack.t >= end) {
        snack.t = -1;
        snack.cool = SN_COOL;
        pip_hop = 0;
        dbg("snack done");
    }
}

/* where follower k stands this frame: its place in line, or on its way to / at the treat */
static void follower_pos(int k, int *x, int *y, int *hop) {
    int t = (trail_i - (k + 1) * TRAIL_GAP) & (TRAIL - 1);
    *x = trail_x[t];
    *y = trail_y[t];
    *hop = walk_t > 0 ? hop_of(walk_t + k * 5) : 0;
    int st = snack.t, f = 0;
    if (st < 0 || !n_follow) return;
    if (st >= 10 && st < SN_THERE) f = (st - 10) * 256 / (SN_THERE - 10);
    else if (st >= SN_THERE && st < SN_GONE + 6) f = 256;
    else if (st >= SN_GONE + 6) f = 256 - (st - SN_GONE - 6) * 256 / (SN_BACK - SN_GONE - 6);
    if (f < 0) f = 0;
    int sx, sy;
    snack_seat(k, &sx, &sy);
    *x += (sx - *x) * f / 256;
    *y += (sy - *y) * f / 256;
    if ((st >= 10 && st < SN_THERE) || st >= SN_GONE + 6) *hop = hop_of(st * 2 + k * 5);   /* hopping over and back */
    int bite = st - (SN_THERE + 6 + k * 16);
    if (bite >= 0 && bite < 10) *hop = bite < 5 ? bite : 10 - bite;                     /* a happy hop at its bite */
}

static void draw_snack(void) {
    if (snack.t < 0) return;
    int st = snack.t, tx, ty;
    snack_spot(&tx, &ty);
    if (st < SN_GONE) {
        int x = tx, y = ty;
        if (st < SN_LAND) {                                          /* arcs out of the basket */
            x = A->basket_x + (tx - A->basket_x) * st / SN_LAND;
            y = A->basket_y - 8 + (ty - A->basket_y + 8) * st / SN_LAND - isin(st * 32 / SN_LAND) * 26 / 256;
        }
        world_spr(y, x - 8 - cam_x, y - 15 - cam_y, A0_SQUARE, 1, 0, T_EXTRA + snack.kind * 4, P_SPARK);
    } else if (st < SN_GONE + 16) {                                  /* all gone: a twinkle */
        int f = (st - SN_GONE) / 4;
        ui_spr(tx - 4 - cam_x, ty - 10 - cam_y, A0_SQUARE, 0, 0, T_SPARK + (f < 3 ? 2 - f % 3 : 0), P_SPARK);
    }
    /* a heart rises over each friend after its bite (over Pip without friends) */
    int n = n_follow ? n_follow : 1;
    for (int k = 0; k < n; k++) {
        int ht = st - (n_follow ? SN_THERE + 10 + k * 16 : SN_THERE + 8);
        if (ht < 0 || ht > 36) continue;
        int hx, hy, hop;
        if (n_follow) follower_pos(k, &hx, &hy, &hop);
        else { hx = pip_x >> 8; hy = (pip_y >> 8) - 12; }
        ui_spr(hx - 4 - cam_x + isin(ht * 3) * 3 / 256, hy - 26 - ht / 2 - cam_y, A0_SQUARE, 0, 0, T_SPARK + 3, P_SPARK);
    }
}

/* the mailbox flag (a sprite: up while a letter waits) and the envelope that bobs over it */
static void draw_mailbox(void) {
    if (!A->mail_x) return;
    int sx = A->mail_x - cam_x, sy = A->mail_y - cam_y;
    if (!on_screen(sx - 8, sy - 40, 24, 44)) return;
    bool waiting = game_save.mail_new;
    int wave = waiting && ((frame_count / 16) & 1);
    world_spr(A->mail_y, sx + 5, sy - 24 - wave, A0_SQUARE, 1, 0, T_EXTRA + (waiting ? EXTRA_FLAG_UP : EXTRA_FLAG_DOWN), P_SPARK);
    if (waiting && !at_mail) {
        int bob = isin((int)(frame_count * 2)) * 3 / 256;
        hud_spr(sx - 8, sy - 44 + bob, A0_SQUARE, 1, 0, T_EXTRA + EXTRA_ENVELOPE, P_SPARK);
    }
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
    int bob = ((walking && (step & 1)) ? 1 : 0) + pip_hop;
    int py = pip_y >> 8;
    int sx = (pip_x >> 8) - cam_x, sy = py - cam_y;
    world_spr(py, sx - 8, sy - PIP_FEET_ROW - 1 - bob, A0_TALL, 2, hflip ? A1_HFLIP : 0, T_PIP + frame * 8, P_PIP);
    world_spr(py - 1, sx - 8, sy - 5, A0_WIDE | A0_BLEND, 0, 0, T_SHADOW, P_SHADOW);
}

/* one friend: 16 px sprite standing on (x, y), with a small shadow */
static void draw_friend(int id, int x, int y, int hop) {
    int sx = x - cam_x, sy = y - cam_y;
    if (!on_screen(sx - 8, sy - 16, 16, 18)) return;
    int sp = friend_species(id) - A->area * 4;          /* the area's 4 species are loaded */
    world_spr(y, sx - 8, sy - 15 - hop, A0_SQUARE, 1, 0, T_SQ16 + sp * 8, P_SQ + friend_flavor(id));
    world_spr(y - 1, sx - 8, sy - 5, A0_WIDE | A0_BLEND, 0, 0, T_SHADOW, P_SHADOW);
}

static int hop_of(int step) {             /* little hops while walking */
    int ph = (step / 5) % 4;
    return ph == 1 ? 2 : ph == 2 ? 3 : 0;
}

static void draw_friends(void) {
    for (int i = 0; i < n_pen; i++) {
        Roamer *r = &pen[i];
        draw_friend(r->id, r->x, r->y, r->wait > 0 ? 0 : hop_of(r->step));
    }
    for (int k = 0; k < n_follow; k++) {
        int x, y, hop;
        follower_pos(k, &x, &y, &hop);
        draw_friend(follow_id[k], x, y, hop);
    }
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
        hud_spr(bx - cam_x - 8, by - cam_y - 36 + bob, A0_TALL, 2, 0, T_ABTN, P_ABTN);
    } else if (at_sign || at_mail || at_basket) {
        int bob = isin((int)(frame_count * 2)) * 2 / 256;
        int x = at_sign ? A->sign_x : at_mail ? A->mail_x : A->basket_x;
        int y = at_sign ? A->sign_y : at_mail ? A->mail_y : A->basket_y + 10;
        int top = (pip_y >> 8) < y ? (pip_y >> 8) : y;   /* over Pip's head when he stands above the prop */
        hud_spr(x - cam_x - 8, top - cam_y - 46 + bob, A0_TALL, 2, 0, T_ABTN, P_ABTN);
    }
}

static int isqrt(int v) {
    int r = 0;
    while ((r + 1) * (r + 1) <= v) r++;
    return r;
}

/* Guide arrow: floats ARROW_R px from Pip toward the nearest box and blinks
 * (ARROW_BLINK frames on, the same off). 8 directions from 3 drawings and flips. */
static void draw_arrow(void) {
    if (arrow_box < 0 || boxes[arrow_box].spot < 0 || touch_box >= 0 || snack.t >= 0) return;
    int t = arrow_t++;
    if ((t / ARROW_BLINK) & 1) return;
    int pcx = (pip_x >> 8) - cam_x, pcy = (pip_y >> 8) - cam_y - 12;
    int dx = spot_x(boxes[arrow_box].spot) - cam_x - pcx, dy = spot_y(boxes[arrow_box].spot) - cam_y - 8 - pcy;
    int len = isqrt(dx * dx + dy * dy);
    if (len == 0) return;
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
    int r = ARROW_R + (isin((int)(frame_count * 2)) + 256) * 3 / 512;   /* a small nudge toward the box */
    int x = pcx + dx * r / len, y = pcy + dy * r / len;
    hud_spr(x - 8, y - 8, A0_SQUARE, 1, flip, T_ARROW + tile * 4, P_ARROW);
}

static void draw_counter(void) {
    if (!found_in_area(A->area)) return;
    int hop = 0;
    if (count_hop > 0) {                /* two small hops, starting after the fade-in */
        count_hop--;
        int t = count_hop % 20;
        if (count_hop < 30) hop = t * (20 - t) / 25;
    }
    int x = SCREEN_W - 4 - COUNT_PILL_W, y = 3 - hop;
    hud_spr(x + COUNT_TEXT_X, y + 1, A0_WIDE, 2, 0, T_COUNT_TXT, P_COUNT);   /* text over the pill */
    hud_spr(x, y, A0_WIDE, 2, 0, T_COUNT, P_COUNT);
    hud_spr(x + 32, y, A0_WIDE, 2, 0, T_COUNT + 8, P_COUNT);
}

/* Momo lies at the gate's bottom centre: 32x32, asleep with z's rising,
 * awake for a peek while Pip stands close. */
static void draw_momo(int x, int y, bool awake, int hop, u16 flip) {
    int sx = x - cam_x, sy = y - cam_y;
    if (!on_screen(sx - 16, sy - 32, 32, 34)) return;
    int breathe = awake ? 0 : ((frame_count / 40) & 1);                  /* slow sleepy breathing */
    world_spr(y, sx - 16, sy - 32 - hop + breathe, A0_SQUARE, 2, flip, T_NPC + (awake ? NPC_AWAKE : 0), P_SHADOW);
    world_spr(y - 1, sx - 8, sy - 5, A0_WIDE | A0_BLEND, 0, 0, T_SHADOW, P_SHADOW);
    if (awake) return;
    for (int k = 0; k < 2; k++) {                                        /* z's drift up and away */
        int t = (int)((frame_count + k * 45) % 90);
        if (t > 70) continue;
        ui_spr(sx + 8 + t / 10, sy - 36 - t / 3, A0_SQUARE, 0, 0, T_NPC + NPC_ZZ + k, P_SHADOW);
    }
}

static int npc_near = -1;          /* gate whose Momo Pip stands by (shows the count bubble), -1 = none */

static void draw_gates(void) {
    for (int i = 0; i < A->ngates; i++) {
        const u16 *g = &A->gates[i * 5];
        if (!gate_there(g[4]) || (gs.t >= 0 && gs.gate == i)) continue;
        int gx = g[0] + g[2] / 2, gy = g[1] + g[3];
        draw_momo(gx, gy, npc_near == i, 0, 0);
        if (npc_near == i && gate_shut(g[4])) {                          /* bubble: the area's icon and "7/10" */
            int bob = isin((int)(frame_count * 2)) * 2 / 256;
            int x = gx - COUNT_PILL_W / 2 - cam_x, y = gy - 58 - cam_y + bob;
            if (x > SCREEN_W - 2 - COUNT_PILL_W) x = SCREEN_W - 2 - COUNT_PILL_W;   /* Momo sleeps near map edges */
            if (x < 2) x = 2;
            hud_spr(x + COUNT_TEXT_X, y + 1, A0_WIDE, 2, 0, T_NPC_TXT, P_COUNT);
            hud_spr(x, y, A0_WIDE, 2, 0, T_COUNT, P_COUNT);
            hud_spr(x + 32, y, A0_WIDE, 2, 0, T_COUNT + 8, P_COUNT);
        }
    }
}

/* Once every way is open Momo sleeps happily on its cloud bed (Cloud Hill).
 * Pip close by: Momo wakes, hops for joy and hearts float up. */
static bool momo_home_near;
static int momo_home_t;             /* frames since Pip came close, for the hops */

static void draw_momo_home(void) {
    if (!A->momo_x || !gate_built(AREA_COUNT - 1)) return;
    int x = A->momo_x, y = A->momo_y, hop = 0;
    if (momo_home_near) {
        int t = momo_home_t % 48;                                       /* two quick hops, then a rest */
        if (t < 24) hop = (t % 12) * (12 - t % 12) / 6;
    }
    int sx = x - cam_x, sy = y - cam_y;
    if (on_screen(sx - 32, sy - 24, 64, 32))                            /* the blanket tucks Momo in (64x32, */
        world_spr(y + 1, sx - 32, sy - 24, A0_WIDE, 3, 0, T_NPC + NPC_BLANKET, P_SHADOW);   /* quilt in rows 17..30) */
    draw_momo(x, y, momo_home_near, hop, 0);
    if (!momo_home_near) return;
    for (int k = 0; k < 3; k++) {                                        /* hearts drift up and fade out */
        int t = (momo_home_t + k * 30) % 90;
        if (t > 72) continue;
        int wob = isin(t * 2 + k * 20) * 3 / 256;
        ui_spr(x - cam_x - 4 + (k - 1) * 10 + wob, y - cam_y - 38 - t / 3, A0_SQUARE, 0, 0, T_SPARK + 3, P_SPARK);
    }
}

static void update_npc(void) {
    int px = pip_x >> 8, py = pip_y >> 8, was = npc_near;
    if (A->momo_x && gate_built(AREA_COUNT - 1)) {                      /* Momo at home on Cloud Hill */
        int dx = px - A->momo_x, dy = py - A->momo_y;
        bool near = dx * dx + dy * dy < 40 * 40;
        if (near && !momo_home_near) {
            momo_home_t = 0;
            sfx_chime(4);
            dbg("momo home near");
        }
        momo_home_near = near;
        momo_home_t++;
    }
    npc_near = -1;
    for (int i = 0; i < A->ngates; i++) {
        const u16 *g = &A->gates[i * 5];
        int dx = px - (g[0] + g[2] / 2), dy = py - (g[1] + g[3] - 8);
        if (gate_there(g[4]) && dx * dx + dy * dy < 34 * 34) npc_near = i;
    }
    if (npc_near >= 0 && npc_near != was) {
        sfx_blip();                                                      /* "hm?": Momo opens one eye */
        dbg("npc near %d found %d", npc_near, found_in_area(A->area));
    }
}

static void draw_friend(int id, int x, int y, int hop);

/* The helpers hop in from the side Pip came from and tickle Momo (squished
 * frame, squeaks) until Momo wakes up, hops for joy and waddles off toward
 * the next area; sparkles, the ta-da, and the friends cheer. */
static void draw_gate_scene(void) {
    int t = gs.t, gx, gy;
    gate_center(&gx, &gy);
    int mx = gx, my = gy, hop = 0;
    bool awake = t >= GS_PUSH;
    if (t >= GS_IN && t < GS_PUSH) mx += ((t - GS_IN) % 24 < 8) ? 1 : 0;           /* wriggles when tickled */
    if (t >= GS_PUSH && t < GS_ROLL) hop = ((t - GS_PUSH) % 16) < 8 ? ((t - GS_PUSH) % 16) * (8 - (t - GS_PUSH) % 16) / 4 : 0;
    if (t >= GS_ROLL) {                                                          /* waddles off toward the exit */
        int r = t - GS_ROLL, ex = gx, ey = gy;
        for (int i = 0; i < A->nexits; i++)
            if (A->exits[i * 7 + 4] == A->gates[gs.gate * 5 + 4]) {
                ex = A->exits[i * 7] + A->exits[i * 7 + 2] / 2;
                ey = A->exits[i * 7 + 1] + A->exits[i * 7 + 3];
            }
        int sx = ex > gx ? 1 : ex < gx ? -1 : 0, sy = ey > gy + 20 ? 1 : ey < gy - 20 ? -1 : 0;
        mx += sx * r * 3 / 2;
        my += sy * r * 3 / 2;
        hop = (r % 10) < 5 ? 2 : 0;
    }
    if (t < GS_CHEER) draw_momo(mx, my, awake, hop, (t >= GS_ROLL && mx < gx) ? A1_HFLIP : 0);
    if (t >= GS_ROLL && t < GS_CHEER) {                                         /* sparkles where Momo slept */
        static const s8 at[4][2] = {{-10, -24}, {9, -14}, {-6, -6}, {8, -30}};
        int f = ((t - GS_ROLL) / 5) % 4;
        for (int k = 0; k < 4; k++)
            ui_spr(gx + at[k][0] - cam_x - 4, gy + at[k][1] - cam_y - 4, A0_SQUARE, 0, 0, T_SPARK + ((f + k) % 3), P_SPARK);
    }
    for (int k = 0; k < gs.n; k++) {
        int hx = gx - 22 - (k == 1 ? 6 : 0), hy = gy - 16 + k * 9;                 /* beside Momo, feet on the way */
        int in = t < GS_PAN ? 0 : t < GS_IN ? t - GS_PAN : GS_IN - GS_PAN;
        int x = lerp(hx - 64, hx, in, GS_IN - GS_PAN);
        int h = 0;
        if (t < GS_IN) h = ((t + k * 7) / 5) % 4 == 1 ? 3 : 0;
        else if (t >= GS_PUSH) h = ((t + k * 5) / 6) % 3 == 0 ? 4 : 0;
        if (t >= GS_BACK - 20) break;                                            /* they run off with Pip's camera */
        int before = n_world;
        draw_friend(gs.ids[k], x, hy, h);
        if (n_world == before + 2 && t >= GS_IN && t < GS_PUSH && (t - GS_IN) % 24 < 8) world[before].a2 += 4;   /* squished frame */
    }
}

static void gate_scene_update(void) {
    int t = gs.t;
    if (t >= GS_IN && t < GS_PUSH && (t - GS_IN) % 24 == 0)
        for (int k = 0; k < gs.n; k++) sfx_squeak(friend_flavor(gs.ids[k]));
    if (t == GS_PUSH) {                                                          /* Momo wakes up */
        sfx_chime(2);
        sfx_boing();
    }
    if (t == GS_ROLL) {
        sfx_chime(4);
        song_play(tune_pop, tune_pop_len);
    }
    if (++gs.t >= GS_BACK) {
        int to = A->gates[gs.gate * 5 + 4];
        game_save.gates |= (u8)(1 << (to - 1));
        collection_save();
        dbg("gate scene %d done", to);
        gs.t = -1;
    }
}

static void draw(void) {
    n_world = n_oam = 0;
    draw_gates();
    draw_momo_home();
    draw_mailbox();
    draw_snack();
    if (gs.t >= 0) draw_gate_scene();
    draw_counter();
    draw_arrow();
    draw_boxes();
    draw_friends();
    draw_pip();
    flush_world();
}

/* Opens box i: rolls its friend and shows the open screen. */
static void open_box(int i) {
    open_friend = collection_roll(A->area, frame_count);
    open_color = boxes[i].color;
    boxes[i].spot = -1;
    boxes[i].wait = RESPAWN;
    touch_box = -1;
    arrow_box = -1;
    seek_t = 0;
    sfx_chime(3);
    dbg("box %d open friend %d pip=%d,%d", i, open_friend, (int)(pip_x >> 8), (int)(pip_y >> 8));
    save_pos(true);
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
            if (arrow_box < 0 && near >= 0) arrow_t = 0;   /* start the blink with the arrow on; no sound (owner) */
            arrow_box = near;
            dbg("arrow on box %d", arrow_box);
        }
    }

    if (touch_box >= 0 && (hit & (KEY_A | KEY_B))) open_box(touch_box);
}

/* Pip close enough to a prop to use it (the pen sign's zone: above it too, it blocks at y - 8) */
static bool near_prop(int x, int y, int px, int py) {
    return px > x - 20 && px < x + 20 && py > y - 18 && py < y + 22;
}

/* water highlights glint softly: cycle the shimmer color every 12 frames */
static void shimmer(void) {
    if ((frame_count % 12) != 0) return;
    u16 c = A->shimmer_cycle[(frame_count / 12) & 3];
    for (int i = 0; i < A->nshimmer; i++) PAL_BG[A->shimmer[i]] = c;
}

static void update(void) {
    bool fading = scene_fading();      /* a press in a fade would be half done: scene_go ignores it */
    if (gs.t < 0 && !fading)
        for (int i = 0; i < A->ngates; i++)
            if (!gate_shut(A->gates[i * 5 + 4]) && !gate_built(A->gates[i * 5 + 4])) {
                gate_scene_start(i);
                break;
            }
    if (gs.t >= 0) {                   /* the friends are busy: Pip waits, buttons rest */
        shimmer();
        friends_update(false);
        gate_scene_update();
        update_camera();
        draw();
        return;
    }
    if (snack.cool > 0) snack.cool--;
    if (snack.t >= 0) {                /* snack time: Pip and the friends are busy eating */
        shimmer();
        friends_update(false);
        snack_update();
        update_camera();
        draw();
        return;
    }
    u16 held = fading ? 0 : key_held();
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
    s32 was_x = pip_x, was_y = pip_y;
    if (dx || dy) {
        move(dx, dy);
        walk_t++;
    } else {
        walk_t = 0;
    }
    friends_update(pip_x != was_x || pip_y != was_y);   /* the line follows real steps only */
    if (pip_x != was_x || pip_y != was_y) {
        still_t = 0;
        pos_dirty = true;
    } else if (still_t < POS_SAVE_WAIT) {
        still_t++;
    }
    save_pos(false);
    update_camera();

    if (door_cool > 0) door_cool--;
    int px = pip_x >> 8, py = pip_y >> 8;
    for (int i = 0; i < A->ndoors; i++) {
        const u16 *d = &A->doors[i * 4];
        if (!door_cool && px >= d[0] && px < d[0] + d[2] && py >= d[1] && py < d[1] + d[3] + 12 &&
            (held & KEY_UP)) {
            sfx_chime(3);
            dbg("door %d", i);
            pip_y = (d[1] + d[3] + 16) << 8;        /* step back out when we return */
            dir = DIR_DOWN;
            pos_dirty = true;                       /* save the spot outside the door */
            save_pos(true);
            scene_go(&scene_house);                 /* the room with this area's gifts (owner: not the shelf) */
        }
    }
    for (int i = 0; i < A->nexits && !fading; i++) {          /* walking off the map into the next area */
        const u16 *e = &A->exits[i * 7];
        if (px >= e[0] && px < e[0] + e[2] && py >= e[1] && py < e[1] + e[3] && !gate_there(e[4])) {
            travel_to = e[4];
            travel_x = e[5];
            travel_y = e[6];
            dbg("exit to area %d", travel_to);
            save_pos(true);
            scene_reload();
            fading = true;
        }
    }
    if (!fading && (key_hit() & KEY_START)) {
        sfx_chime(2);
        dbg("meadow start pip=%d,%d", px, py);
        save_pos(true);
        scene_go(&scene_shelf);
    }
    u16 hit = fading ? 0 : key_hit();
    update_npc();
    update_boxes(hit);
    if (scene_fading()) {              /* a box just opened: this A press is used up */
        draw();
        return;
    }
    bool was_at = at_sign;             /* by the pen sign: A picks who follows Pip */
    at_sign = touch_box < 0 && found_in_area(A->area) > 0 && A->sign_x && px > A->sign_x - 20 && px < A->sign_x + 20 &&
              py > A->sign_y - 18 && py < A->sign_y + 22;   /* above it too: the sign blocks Pip at y-8 */
    if (at_sign && !was_at) dbg("at sign pip=%d,%d", px, py);
    if (at_sign && (hit & KEY_A)) {
        sfx_chime(3);
        dir = DIR_DOWN;
        shelf_pick = true;
        save_pos(true);
        scene_go(&scene_shelf);
        return;
    }
    bool was_mail = at_mail, was_basket = at_basket;   /* the mailbox and the picnic basket (meadow) */
    at_mail = touch_box < 0 && !at_sign && game_save.mail && A->mail_x && near_prop(A->mail_x, A->mail_y, px, py);
    at_basket = touch_box < 0 && !at_sign && !at_mail && snack.cool == 0 && A->basket_x &&
                near_prop(A->basket_x, A->basket_y, px, py);
    if (at_mail && !was_mail) dbg("at mailbox pip=%d,%d new=%d", px, py, game_save.mail_new);
    if (at_basket && !was_basket) dbg("at basket pip=%d,%d", px, py);
    if (at_mail && (hit & KEY_A)) {
        sfx_chime(3);
        save_pos(true);
        scene_go(&scene_letter);
        return;
    }
    if (at_basket && (hit & KEY_A)) {
        at_basket = false;
        snack_start();
        draw();
        return;
    }
    if ((hit & (KEY_A | KEY_B)) && touch_box < 0) sfx_tick();
    draw();
}

const Scene scene_meadow_view = {enter, update};
