#define _POSIX_C_SOURCE 200809L

#include "hal/rotary_encoder.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdint.h>
#include <limits.h>
#include <time.h>
#include <gpiod.h>

static void pos_cb(int32_t position, int32_t delta, int64_t ts_ns, void *user) {
    (void)user;
    printf("[POS] ts=%" PRId64 " delta=%d pos=%d\n", ts_ns, delta, position);
    fflush(stdout);
}

static void usage(const char *argv0) {
    fprintf(stderr,
        "Usage: %s --chip <gpiochipX|/dev/gpiochipX> --a <offset> --b <offset>\n"
        "           [--a-active-low] [--b-active-low]\n"
        "           [--steps-per-detent N]\n"
        "           [--a-bias up|down|disable] [--b-bias up|down|disable]\n",
        argv0);
}

static int parse_bias(const char *s) {
    if (!s) return -1;
    if (!strcmp(s, "up")) return GPIOD_LINE_BIAS_PULL_UP;
    if (!strcmp(s, "down")) return GPIOD_LINE_BIAS_PULL_DOWN;
    if (!strcmp(s, "disable")) return GPIOD_LINE_BIAS_DISABLED;
    return -1;
}

int main(int argc, char **argv) {
    re_config cfg = {
        .chip = "gpiochip0",
        .line_a = UINT_MAX,
        .line_b = UINT_MAX,
        .a_active_low = false,
        .b_active_low = false,
        .steps_per_detent = 4,
        .a_bias = -1,
        .b_bias = -1,
        .consumer = "as2_demo"
    };

    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "--chip") && i+1 < argc) {
            cfg.chip = argv[++i];
        } else if (!strcmp(argv[i], "--a") && i+1 < argc) {
            cfg.line_a = (unsigned)strtoul(argv[++i], NULL, 0);
        } else if (!strcmp(argv[i], "--b") && i+1 < argc) {
            cfg.line_b = (unsigned)strtoul(argv[++i], NULL, 0);
        } else if (!strcmp(argv[i], "--a-active-low")) {
            cfg.a_active_low = true;
        } else if (!strcmp(argv[i], "--b-active-low")) {
            cfg.b_active_low = true;
        } else if (!strcmp(argv[i], "--steps-per-detent") && i+1 < argc) {
            cfg.steps_per_detent = (unsigned)strtoul(argv[++i], NULL, 0);
            if (!cfg.steps_per_detent) cfg.steps_per_detent = 4;
        } else if (!strcmp(argv[i], "--a-bias") && i+1 < argc) {
            cfg.a_bias = parse_bias(argv[++i]);
        } else if (!strcmp(argv[i], "--b-bias") && i+1 < argc) {
            cfg.b_bias = parse_bias(argv[++i]);
        } else {
            usage(argv[0]);
            return 1;
        }
    }

    if (cfg.line_a == UINT_MAX || cfg.line_b == UINT_MAX) {
        usage(argv[0]);
        return 1;
    }

    re_encoder *enc = NULL;
    if (re_create(&cfg, &enc) < 0) {
        fprintf(stderr, "Failed to create encoder (check chip path, offsets, permissions)\n");
        return 1;
    }

    re_set_position_callback(enc, pos_cb, NULL);

    if (re_start(enc) < 0) {
        fprintf(stderr, "Failed to start encoder\n");
        re_destroy(enc);
        return 1;
    }

    printf("Running. Ctrl-C to exit.\n");
    while (1) {
        struct timespec ts = { .tv_sec = 3600, .tv_nsec = 0 };
        nanosleep(&ts, NULL);
    }

    re_stop(enc);
    re_destroy(enc);
    return 0;
}