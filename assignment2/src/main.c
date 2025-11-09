#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <stdint.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include "sampler.h"
#include "udp_listener.h"
#include "pwm_hal.h"
#include "reporter.h"   // <-- Added reporter include

// Encoder pins
#define ENC_A "/sys/class/gpio/gpio4/value"
#define ENC_B "/sys/class/gpio/gpio16/value"

// LED PWM limits
#define MIN_FREQ 1
#define MAX_FREQ 1000

static volatile int keepRunning = 1;

void intHandler(int dummy) {
    (void)dummy;
    keepRunning = 0;
}

// Helper: read GPIO value
int readGPIO(const char* path) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) return 0;
    char buf[2] = {0};
    read(fd, buf, 1);
    close(fd);
    return buf[0] == '1';
}

// Rotary encoder tracking
int updateEncoder(int* lastA, int* lastB) {
    int A = readGPIO(ENC_A);
    int B = readGPIO(ENC_B);
    int delta = 0;

    if (*lastA != A || *lastB != B) {
        if (*lastA == 0 && A == 1) {
            delta = (B == 0) ? 1 : -1;
        }
        *lastA = A;
        *lastB = B;
    }
    return delta;
}

int main(void) {
    signal(SIGINT, intHandler);

    printf("Initializing sampler...\n");
    Sampler_init();

    printf("Initializing UDP listener...\n");
    UDP_init();

    printf("Initializing PWM for LED...\n");
    PWM_init();
    int pwmFreq = 10;
    PWM_setFrequency(pwmFreq);

    printf("Initializing reporter...\n");
    Reporter_init();           // <-- Start reporter thread
    Reporter_setPWMFrequency(pwmFreq);

    int lastA = readGPIO(ENC_A);
    int lastB = readGPIO(ENC_B);

    while (keepRunning) {
        // 1. Move current buffer to history every second
        Sampler_moveCurrentDataToHistory();

        // 2. Update encoder and adjust LED frequency
        int delta = updateEncoder(&lastA, &lastB);
        pwmFreq += delta * 10;  // 10 Hz per tick
        if (pwmFreq < MIN_FREQ) pwmFreq = MIN_FREQ;
        if (pwmFreq > MAX_FREQ) pwmFreq = MAX_FREQ;
        PWM_setFrequency(pwmFreq);
        Reporter_setPWMFrequency(pwmFreq); // <-- update reporter

        sleep(1);
    }

    printf("Cleaning up...\n");
    PWM_cleanup();
    Sampler_cleanup();
    UDP_cleanup();
    Reporter_cleanup();          // <-- stop reporter thread

    printf("Exit.\n");
    return 0;
}
