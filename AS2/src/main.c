#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include "sampler.h"
#include "pwm_hal.h"
#include "udp_listener.h"
#include "periodTimer.h"
#include "reporter.h"
#include "encoder.h"

volatile int keepRunning = 1;

// Current PWM frequency index
static size_t currentIndex = 0;
static int pwmFrequencies[] = {12, 16, 22, 29, 32, 37, 45};
static size_t numFrequencies = sizeof(pwmFrequencies) / sizeof(pwmFrequencies[0]);

// Handle Ctrl+C
void sigintHandler(int sig) {
    (void)sig;
    keepRunning = 0;
}

// Callback for encoder changes
void encoderPositionChanged(int delta) {
    if (delta > 0) {
        currentIndex = (currentIndex + 1) % numFrequencies;
    } else if (delta < 0) {
        currentIndex = (currentIndex + numFrequencies - 1) % numFrequencies;
    }

    int freq = pwmFrequencies[currentIndex];
    PWM_setFrequency(freq);
    Reporter_setPWMFrequency(freq);

    printf("Encoder changed: delta=%d, PWM frequency=%d Hz\n", delta, freq);
}

int main(void) {
    // Register Ctrl+C handler
    signal(SIGINT, sigintHandler);

    // Initialize modules
    Sampler_init();
    PWM_init();
    UDP_init();
    Period_init();
    Reporter_start();

    // Initialize encoder
    Encoder_init();
    Encoder_setPositionCallback(encoderPositionChanged);

    // Start with first PWM frequency
    PWM_setFrequency(pwmFrequencies[currentIndex]);
    Reporter_setPWMFrequency(pwmFrequencies[currentIndex]);

    printf("Starting main loop. Ctrl+C to exit...\n");

    while (keepRunning) {
        sleep(1); // main loop sleeps, encoder thread handles changes
    }

    // Cleanup modules
    Encoder_cleanup();
    Reporter_stop();
    PWM_cleanup();
    UDP_cleanup();
    Sampler_cleanup();
    Period_cleanup();

    printf("Exiting cleanly...\n");
    return 0;
}

