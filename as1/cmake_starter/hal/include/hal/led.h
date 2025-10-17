#ifndef _LED_H_
#define _LED_H_

#include <stdbool.h>

typedef struct {
    const char *green_trigger_path;
    const char *green_brightness_path;
    const char *red_trigger_path;
    const char *red_brightness_path;
    int green_state;
    int red_state;
} LEDHAL;

void led_init(LEDHAL *h, const char *green_trigger_path, const char *green_brightness_path,const char *red_trigger_path, const char *red_brightness_path);
void led_green_on(LEDHAL *h);
void led_green_off(LEDHAL *h);
void led_red_on(LEDHAL *h);
void led_red_off(LEDHAL *h);

/* Flash a color (green or red) 'times' times over total_duration_sec */
void led_flash_color(LEDHAL *h, const char *color, int times, double total_duration_sec);

void led_cleanup(LEDHAL *h);

#endif