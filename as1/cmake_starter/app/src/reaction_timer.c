#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>

#include "hal/led.h"
#include "hal/joystick.h"
#include "hal/spi.h"
#include "hal/mcp3208.h"

static volatile int keep_running = 1;
static void sigint_handler(int s) { (void)s; keep_running = 0; }

/* ---------------- Utility time helpers ---------------- */

static double now_seconds(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec / 1e9;
}
static long elapsed_ms(double start, double end) {
    return (long)((end - start) * 1000.0);
}

int main()
{
    signal(SIGINT, sigint_handler);
    const char *spi_dev = "/dev/spidev0.0";
    uint32_t spi_speed = 1000000U;
    int xchan = 0, ychan = 1;

    const char *g_trigger = "/sys/class/leds/ACT/trigger";
    const char *g_bright  = "/sys/class/leds/ACT/brightness";
    const char *r_trigger = "/sys/class/leds/PWR/trigger";
    const char *r_bright  = "/sys/class/leds/PWR/brightness";

    srand((unsigned)time(NULL));

    LED led;
    Joystick joy;
    led_init(&led, g_trigger, g_bright, r_trigger, r_bright);

    if (joystick_init(&joy, spi_dev, spi_speed, xchan, ychan) != 0) {
        fprintf(stderr, "Failed to initialize joystick (SPI).\n");
    }

    printf("Hello embedded world, from Colby!\n\n");

    long best_ms = -1;
    int round_number = 0;

    while (keep_running) {
        round_number++;
        printf("=== Round %d ===\n", round_number);

        /* Step 1: get ready + flash LEDs 4 times (green then red) */
        printf("Get ready...\n");
        for (int i = 0; i < 4; ++i) {
            led_green_on(&led);
            usleep(250000);
            led_green_off(&led);
            led_red_on(&led);
            usleep(250000);
            led_red_off(&led);
        }

        /* Step 2: if joystick pressing up or down, ask user to let go and wait until released */
        JOY_STATE state = joystick_get_state(&joy);
        if (state == JSTATE_UP || state == JSTATE_DOWN) {
            printf("Please let go of joystick\n");
            fflush(stdout);
            while (keep_running) {
                usleep(50000);
                state = joystick_get_state(&joy);
                if (!(state == JSTATE_UP || state == JSTATE_DOWN)) break;
            }
        }

        /* Step 3: wait random delay between 0.5 and 3.0s */
        double delay = 0.5 + ((double)rand() / RAND_MAX) * (3.0 - 0.5);
        usleep((useconds_t)(delay * 1e6));

        /* Step 4: If after the delay the user is pressing up/down, too soon */
        state = joystick_get_state(&joy);
        if (state == JSTATE_UP || state == JSTATE_DOWN) {
            printf("too soon\n\n");
            led_green_off(&led);
            led_red_off(&led);
            continue;
        }

        /* Step 5: pick random direction */
        JOY_STATE chosen = (rand() % 2) ? JSTATE_UP : JSTATE_DOWN;
        if (chosen == JSTATE_UP) {
            printf("GO: UP\n");
            led_green_on(&led);
            led_red_off(&led);
        } else {
            printf("GO: DOWN\n");
            led_red_on(&led);
            led_green_off(&led);
        }
        fflush(stdout);

        /* Step 6: time how long it takes for user to press joystick any direction (max 5s) */
        double start = now_seconds();
        JOY_STATE pressed = JSTATE_NONE;
        while (keep_running) {
            state = joystick_get_state(&joy);
            if (state != JSTATE_NONE) {
                pressed = state;
                break;
            }
            double elapsed = now_seconds() - start;
            if (elapsed > 5.0) {
                printf("No response within 5 seconds. Exiting.\n");
                led_green_off(&led);
                led_red_off(&led);
                joystick_cleanup(&joy);
                led_cleanup(&led);
                return 0;
            }
            usleep(10000);
        }
        double end = now_seconds();
        long response_ms = elapsed_ms(start, end);

        /* Ensure indicator LEDs off */
        led_green_off(&led);
        led_red_off(&led);

        /* Step 7: process joystick press */
        if (pressed == JSTATE_UP || pressed == JSTATE_DOWN) {
            if (pressed == chosen) {
                printf("Correct! You pressed %s\n", pressed == JSTATE_UP ? "UP" : "DOWN");
                if (best_ms < 0 || response_ms < best_ms) {
                    best_ms = response_ms;
                    printf("New fastest correct response!\n");
                }
                printf("This attempt: %ld ms    Best: %ld ms\n", response_ms, best_ms);
                led_flash_color(&led, "green", 5, 1.0);
            } else {
                printf("Incorrect. You pressed %s but expected %s\n",
                       pressed == JSTATE_UP ? "UP" : "DOWN",
                       chosen == JSTATE_UP ? "UP" : "DOWN");
                led_flash_color(&led, "red", 5, 1.0);
            }
        } else if (pressed == JSTATE_LEFT || pressed == JSTATE_RIGHT) {
            printf("Left or Right pressed (%s). Quitting.\n", pressed == JSTATE_LEFT ? "LEFT" : "RIGHT");
            led_green_off(&led);
            led_red_off(&led);
            break;
        }

        printf("\n");
        usleep(500000);
    }

    joystick_cleanup(&joy);
    led_cleanup(&led);
    printf("Exiting.\n");
    return 0;
}