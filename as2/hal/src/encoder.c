#include "hal/encoder.h"
#include <gpiod.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define CHIP_NAME "gpiochip0"
#define LINE_A 4   // GPIO12
#define LINE_B 16  // GPIO13

static struct gpiod_chip *chip = NULL;
static struct gpiod_line *lineA = NULL;
static struct gpiod_line *lineB = NULL;
static struct gpiod_line_request *reqA = NULL;
static struct gpiod_line_request *reqB = NULL;

static pthread_t encoderThread;
static volatile int running = 0;
static void (*positionCallback)(int) = NULL;

// Set callback
void Encoder_setPositionCallback(void (*callback)(int)) {
    positionCallback = callback;
}

// Thread to poll encoder
static void *encoderThreadFunc(void *arg) {
    (void)arg;

    int lastA = gpiod_line_request_get_value(reqA, 0);
    int lastB = gpiod_line_request_get_value(reqB, 0);

    while (running) {
        int currA = gpiod_line_request_get_value(reqA, 0);
        int currB = gpiod_line_request_get_value(reqB, 0);

        if (currA != lastA) {
            int delta = (currA == currB) ? 1 : -1;
            if (positionCallback)
                positionCallback(delta);
        }

        lastA = currA;
        lastB = currB;
        usleep(1000); // 1 ms
    }

    return NULL;
}

// Initialize encoder
void Encoder_init(void) {
    chip = gpiod_chip_open_by_name(CHIP_NAME);
    if (!chip) {
        perror("gpiod_chip_open_by_name");
        exit(EXIT_FAILURE);
    }

    lineA = gpiod_chip_get_line(chip, LINE_A);
    lineB = gpiod_chip_get_line(chip, LINE_B);
    if (!lineA || !lineB) {
        perror("gpiod_chip_get_line");
        gpiod_chip_close(chip);
        exit(EXIT_FAILURE);
    }

    struct gpiod_line_request_config config;
    config.consumer = "encoder";
    config.request_type = GPIOD_LINE_REQUEST_DIRECTION_INPUT;
    config.flags = 0;

    reqA = gpiod_line_request(lineA, &config, 0);
    reqB = gpiod_line_request(lineB, &config, 0);
    if (!reqA || !reqB) {
        perror("gpiod_line_request");
        gpiod_chip_close(chip);
        exit(EXIT_FAILURE);
    }

    running = 1;
    if (pthread_create(&encoderThread, NULL, encoderThreadFunc, NULL) != 0) {
        perror("pthread_create");
        gpiod_line_request_release(reqA);
        gpiod_line_request_release(reqB);
        gpiod_chip_close(chip);
        exit(EXIT_FAILURE);
    }
}

// Cleanup encoder
void Encoder_cleanup(void) {
    running = 0;
    pthread_join(encoderThread, NULL);

    if (reqA) gpiod_line_request_release(reqA);
    if (reqB) gpiod_line_request_release(reqB);

    if (chip) gpiod_chip_close(chip);
}

