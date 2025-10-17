#define _GNU_SOURCE
#include "led.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>

static void write_trigger_and_delay(const char *trigger_path, const char *value)
{
    if (!trigger_path) return;
    FILE *f = fopen(trigger_path, "w");
    if (!f) {
        perror("write_trigger_and_delay: fopen trigger");
        return;
    }
    int n = fprintf(f, "%s\n", value);
    if (n <= 0) {
        perror("write_trigger_and_delay: fprintf trigger");
        /* still continue to close */
    }
    fclose(f);
    /* Sleep 100ms to allow kernel to settle per your guidance */
    struct timespec req = {0, 100 * 1000 * 1000};
    nanosleep(&req, NULL);
}

static void write_brightness(const char *brightness_path, int value)
{
    if (!brightness_path) return;
    FILE *f = fopen(brightness_path, "w");
    if (!f) {
        perror("write_brightness: fopen brightness");
        return;
    }
    int n = fprintf(f, "%d\n", value ? 1 : 0);
    if (n <= 0) {
        perror("write_brightness: fprintf brightness");
    }
    fclose(f);
}

void led_init(LEDHAL *h, const char *green_trigger_path, const char *green_brightness_path, const char *red_trigger_path, const char *red_brightness_path)
{
    if (!h) return;
    h->green_trigger_path = green_trigger_path;
    h->green_brightness_path = green_brightness_path;
    h->red_trigger_path = red_trigger_path;
    h->red_brightness_path = red_brightness_path;
    h->green_state = 0;
    h->red_state = 0;

    if (green_brightness_path && green_trigger_path && red_brightness_path && red_trigger_path) {
        /* Set triggers to "none" so we can control brightness directly */
        write_trigger_and_delay(h->green_trigger_path, "none");
        write_trigger_and_delay(h->red_trigger_path, "none");
        /* start with LEDs off */
        write_brightness(h->green_brightness_path, 0);
        write_brightness(h->red_brightness_path, 0);
    }
}

void led_green_on(LEDHAL *h)
{
    if (!h) return;
    write_brightness(h->green_brightness_path, 1);
    h->green_state = 1;
}

void led_green_off(LEDHAL *h)
{
    if (!h) return;
    write_brightness(h->green_brightness_path, 0);
    h->green_state = 0;
}

void led_red_on(LEDHAL *h)
{
    if (!h) return;
    write_brightness(h->red_brightness_path, 1);
    h->red_state = 1;
}

void led_red_off(LEDHAL *h)
{
    if (!h) return;
    write_brightness(h->red_brightness_path, 0);
    h->red_state = 0;
}

void led_flash_color(LEDHAL *h, const char *color, int times, double total_duration_sec)
{
    if (!h || times <= 0 || total_duration_sec <= 0.0) return;
    double cycle = total_duration_sec / (double)times;
    double on_time = cycle / 2.0;
    long on_ns = (long)(on_time * 1e9);
    long off_ns = (long)((cycle - on_time) * 1e9);
    struct timespec ton = { on_ns / 1000000000L, on_ns % 1000000000L };
    struct timespec toff = { off_ns / 1000000000L, off_ns % 1000000000L };

    for (int i = 0; i < times; ++i) {
        if (strcasecmp(color, "green") == 0) {
            led_green_on(h);
            nanosleep(&ton, NULL);
            led_green_off(h);
            nanosleep(&toff, NULL);
        } else if (strcasecmp(color, "red") == 0) {
            led_red_on(h);
            nanosleep(&ton, NULL);
            led_red_off(h);
            nanosleep(&toff, NULL);
        } else {
            nanosleep(&toff, NULL);
        }
    }
}

void led_cleanup(LEDHAL *h)
{
    if (!h) return;
    write_brightness(h->green_brightness_path, 0);
    write_brightness(h->red_brightness_path, 0);
}