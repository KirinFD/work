#ifndef ROTARY_ENCODER_H
#define ROTARY_ENCODER_H

#include <stdint.h>
#include <stdbool.h>

typedef struct re_encoder re_encoder;

typedef void (*re_position_cb)(int32_t position, int32_t delta, int64_t timestamp_ns, void *user);
typedef void (*re_button_cb)(bool pressed, int64_t timestamp_ns, void *user);

typedef void (*re_position_cb)(int32_t position, int32_t delta, int64_t timestamp_ns, void *user);

typedef struct re_config {
    const char *chip;          // "/dev/gpiochip0" or "gpiochip0" (will be prefixed internally)
    unsigned line_a;
    unsigned line_b;

    bool a_active_low;
    bool b_active_low;

    unsigned steps_per_detent; // transitions per detent (typ 4)

    // Bias hints: -1 means "do not set (leave default)"
    int a_bias;                // GPIOD_LINE_BIAS_* or -1
    int b_bias;

    const char *consumer;      // label shown to kernel
} re_config;

int  re_create(const re_config *cfg, re_encoder **out);
void re_set_position_callback(re_encoder *enc, re_position_cb cb, void *user);
int  re_start(re_encoder *enc);
void re_stop(re_encoder *enc);
void re_destroy(re_encoder *enc);
int32_t re_get_position(re_encoder *enc);
#endif