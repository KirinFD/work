#include "hal/rotary_encoder.h"

#include <gpiod.h>
#include <pthread.h>
#include <poll.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <stdatomic.h>
#include <limits.h>

struct re_encoder {
    re_config cfg;

    struct gpiod_chip *chip;
    struct gpiod_line_request *request;

    struct gpiod_line_settings *set_a;
    struct gpiod_line_settings *set_b;

    struct gpiod_line_config *lcfg;
    struct gpiod_request_config *rcfg;

    pthread_t thread;
    atomic_bool running;

    atomic_int position_detent;
    int steps_accum;
    uint8_t prev_state;

    re_position_cb pos_cb;
    void *pos_user;
};

static int line_read_logic(struct re_encoder *enc, unsigned offset, bool active_low) {
    int v = gpiod_line_request_get_value(enc->request, offset);
    if (v < 0) return v;
    return active_low ? !v : v;
}

static int init_prev_state(struct re_encoder *enc) {
    int av = line_read_logic(enc, enc->cfg.line_a, enc->cfg.a_active_low);
    if (av < 0) return -1;
    int bv = line_read_logic(enc, enc->cfg.line_b, enc->cfg.b_active_low);
    if (bv < 0) return -1;
    enc->prev_state = (uint8_t)((av & 1) | ((bv & 1) << 1));
    return 0;
}

static int8_t transition_delta(uint8_t prev, uint8_t curr) {
    uint8_t idx = (prev << 2) | (curr & 0x3);
    switch (idx) {
        case (0u<<2)|1u: case (1u<<2)|3u: case (3u<<2)|2u: case (2u<<2)|0u:
            return +1;
        case (1u<<2)|0u: case (3u<<2)|1u: case (2u<<2)|3u: case (0u<<2)|2u:
            return -1;
        default:
            return 0;
    }
}

static void deliver_position_cb(struct re_encoder *enc, int32_t delta_detents, int64_t ts_ns) {
    if (!enc->pos_cb || delta_detents == 0) return;
    int32_t pos = atomic_load(&enc->position_detent);
    enc->pos_cb(pos, delta_detents, ts_ns, enc->pos_user);
}

static void handle_ab_event(struct re_encoder *enc, int64_t ts_ns) {
    int av = line_read_logic(enc, enc->cfg.line_a, enc->cfg.a_active_low);
    if (av < 0) return;
    int bv = line_read_logic(enc, enc->cfg.line_b, enc->cfg.b_active_low);
    if (bv < 0) return;

    uint8_t curr = (uint8_t)((av & 1) | ((bv & 1) << 1));
    int8_t step = transition_delta(enc->prev_state, curr);
    enc->prev_state = curr;
    if (step == 0) return;

    enc->steps_accum += step;
    unsigned spd = enc->cfg.steps_per_detent ? enc->cfg.steps_per_detent : 4;
    int detent = 0;
    while (enc->steps_accum >= (int)spd) { enc->steps_accum -= (int)spd; detent++; }
    while (enc->steps_accum <= -(int)spd) { enc->steps_accum += (int)spd; detent--; }

    if (detent) {
        int32_t pos = atomic_load(&enc->position_detent);
        pos += detent;
        atomic_store(&enc->position_detent, pos);
        deliver_position_cb(enc, detent, ts_ns);
    }
}

static struct gpiod_line_settings* make_settings(bool active_low, int bias) {
    struct gpiod_line_settings *s = gpiod_line_settings_new();
    if (!s) return NULL;
    gpiod_line_settings_set_direction(s, GPIOD_LINE_DIRECTION_INPUT);
    gpiod_line_settings_set_edge_detection(s, GPIOD_LINE_EDGE_BOTH);
    gpiod_line_settings_set_active_low(s, active_low);
    if (bias >= 0) gpiod_line_settings_set_bias(s, bias);
    return s;
}

static bool is_path(const char *s) { return s && s[0] == '/'; }

static void* worker_thread(void *arg) {
    struct re_encoder *enc = (struct re_encoder*)arg;
    int fd = gpiod_line_request_get_fd(enc->request);
    struct pollfd pfd = { .fd = fd, .events = POLLIN, .revents = 0 };

    (void)init_prev_state(enc);

    // Edge event buffer
    const unsigned CAP = 16;
    struct gpiod_edge_event_buffer *buf = gpiod_edge_event_buffer_new(CAP);
    if (!buf) {
        atomic_store(&enc->running, false);
        return NULL;
    }

    while (atomic_load(&enc->running)) {
        int pr = poll(&pfd, 1, 250);
        if (pr < 0) {
            if (errno == EINTR) continue;
            break;
        }
        if (pr == 0) continue;

        if (pfd.revents & POLLIN) {
            int n = gpiod_line_request_read_edge_events(enc->request, buf, CAP);
            if (n < 0) {
                if (errno == EAGAIN) continue;
                break;
            }
            for (int i = 0; i < n; ++i) {
                struct gpiod_edge_event *ev = gpiod_edge_event_buffer_get_event(buf, i);
                unsigned offset = gpiod_edge_event_get_line_offset(ev);
                int64_t ts_ns = (int64_t)gpiod_edge_event_get_timestamp_ns(ev);
                if (offset == enc->cfg.line_a || offset == enc->cfg.line_b) {
                    handle_ab_event(enc, ts_ns);
                }
            }
        }
    }

    gpiod_edge_event_buffer_free(buf);
    return NULL;
}

int re_create(const re_config *cfg_in, re_encoder **out) {
    if (!cfg_in || !out) return -1;
    if (!cfg_in->chip) return -1;

    re_encoder *enc = (re_encoder*)calloc(1, sizeof(*enc));
    if (!enc) return -1;

    enc->cfg = *cfg_in;
    if (!enc->cfg.consumer) enc->cfg.consumer = "rotary-encoder";
    if (!enc->cfg.steps_per_detent) enc->cfg.steps_per_detent = 4;

    atomic_store(&enc->position_detent, 0);
    enc->steps_accum = 0;

    char path_buf[64];
    const char *chip_path = NULL;
    if (is_path(enc->cfg.chip)) {
        chip_path = enc->cfg.chip;
    } else {
        snprintf(path_buf, sizeof(path_buf), "/dev/%s", enc->cfg.chip);
        chip_path = path_buf;
    }

    enc->chip = gpiod_chip_open(chip_path);
    if (!enc->chip) { free(enc); return -1; }

    enc->lcfg = gpiod_line_config_new();
    enc->rcfg = gpiod_request_config_new();
    if (!enc->lcfg || !enc->rcfg) { re_destroy(enc); return -1; }
    gpiod_request_config_set_consumer(enc->rcfg, enc->cfg.consumer);

    // Settings A/B (reuse if identical)
    if (enc->cfg.a_active_low == enc->cfg.b_active_low &&
        enc->cfg.a_bias == enc->cfg.b_bias) {
        enc->set_a = make_settings(enc->cfg.a_active_low, enc->cfg.a_bias);
        enc->set_b = enc->set_a;
    } else {
        enc->set_a = make_settings(enc->cfg.a_active_low, enc->cfg.a_bias);
        enc->set_b = make_settings(enc->cfg.b_active_low, enc->cfg.b_bias);
    }
    if (!enc->set_a || !enc->set_b) { re_destroy(enc); return -1; }

    unsigned offs_ab[2] = { enc->cfg.line_a, enc->cfg.line_b };
    if (gpiod_line_config_add_line_settings(enc->lcfg, offs_ab, 2, enc->set_a) < 0) {
        re_destroy(enc); return -1;
    }
    if (enc->set_b != enc->set_a) {
        if (gpiod_line_config_add_line_settings(enc->lcfg, &offs_ab[1], 1, enc->set_b) < 0) {
            re_destroy(enc); return -1;
        }
    }

    enc->request = gpiod_chip_request_lines(enc->chip, enc->rcfg, enc->lcfg);
    if (!enc->request) { re_destroy(enc); return -1; }

    atomic_store(&enc->running, false);
    *out = enc;
    return 0;
}

void re_set_position_callback(re_encoder *enc, re_position_cb cb, void *user) {
    if (!enc) return;
    enc->pos_cb = cb;
    enc->pos_user = user;
}

int re_start(re_encoder *enc) {
    if (!enc) return -1;
    bool expected = false;
    if (!atomic_compare_exchange_strong(&enc->running, &expected, true))
        return 0;
    int rc = pthread_create(&enc->thread, NULL, worker_thread, enc);
    if (rc != 0) {
        atomic_store(&enc->running, false);
        return -1;
    }
    return 0;
}

void re_stop(re_encoder *enc) {
    if (!enc) return;
    bool expected = true;
    if (!atomic_compare_exchange_strong(&enc->running, &expected, false))
        return;
    pthread_join(enc->thread, NULL);
}

void re_destroy(re_encoder *enc) {
    if (!enc) return;
    re_stop(enc);

    if (enc->request) gpiod_line_request_release(enc->request);

    if (enc->set_b && enc->set_b != enc->set_a) gpiod_line_settings_free(enc->set_b);
    if (enc->set_a) gpiod_line_settings_free(enc->set_a);

    if (enc->lcfg) gpiod_line_config_free(enc->lcfg);
    if (enc->rcfg) gpiod_request_config_free(enc->rcfg);

    if (enc->chip) gpiod_chip_close(enc->chip);
    free(enc);
}

int32_t re_get_position(re_encoder *enc) {
    if (!enc) return 0;
    return atomic_load(&enc->position_detent);
}