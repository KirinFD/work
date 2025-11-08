#ifndef ROTARY_ENCODER_H
#define ROTARY_ENCODER_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct re_encoder re_encoder;

typedef void (*re_position_cb)(int32_t position, int32_t delta, int64_t timestamp_ns, void *user);

typedef struct re_config {
    const char *chip;   
    unsigned line_a;
    unsigned line_b;

    bool a_active_low;
    bool b_active_low;

    unsigned steps_per_detent;

    int a_bias;               
    int b_bias;

    const char *consumer;   
} re_config;

int  re_create(const re_config *cfg, re_encoder **out);
void re_set_position_callback(re_encoder *enc, re_position_cb cb, void *user);
int  re_start(re_encoder *enc);
void re_stop(re_encoder *enc);
void re_destroy(re_encoder *enc);
int32_t re_get_position(re_encoder *enc);

#ifdef __cplusplus
}
#endif

#endif 