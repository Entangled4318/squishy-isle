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
 *   dump NAME           save VRAM, palettes, OAM and IO registers to
 *                       OUTDIR/NAME.bin (for debugging)
 */
#include <mgba/core/blip_buf.h>
#include <mgba/core/core.h>
#include <mgba/core/log.h>
#include <mgba/gba/core.h>
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

int main(int argc, char **argv) {
    if (argc < 5) {
        fprintf(stderr, "usage: %s ROM SAVE SCRIPT OUTDIR\n", argv[0]);
        return 2;
    }
    outdir = argv[4];
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
                {0x04000000, 0x400}, {0x05000000, 0x400}, {0x06000000, 0x18000}, {0x07000000, 0x400}};
            for (unsigned r = 0; r < 4; r++)
                for (uint32_t i = 0; i < regions[r].size; i++)
                    fputc(core->busRead8(core, regions[r].base + i), f);
            fclose(f);
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
