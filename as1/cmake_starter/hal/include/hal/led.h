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
} LED;

void led_init(LED *h, const char *green_trigger_path, const char *green_brightness_path,const char *red_trigger_path, const char *red_brightness_path);
void led_green_on(LED *h);
void led_green_off(LED *h);
void led_red_on(LED *h);
void led_red_off(LED *h);

/* Flash a color (green or red) 'times' times over total_duration_sec */
void led_flash_color(LED *h, const char *color, int times, double total_duration_sec);

void led_cleanup(LED *h);

#endif