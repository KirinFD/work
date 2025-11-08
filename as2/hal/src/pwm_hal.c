#include "pwm_hal.h"
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>

#define PWM_PATH "/dev/hat/pwm/GPIO12"

static volatile bool pwmRunning = false;
static volatile int currentFreq = 0;

// Helper: write string to PWM file
static void write_pwm(const char* file, const char* value) {
    char path[128];
    snprintf(path, sizeof(path), "%s/%s", PWM_PATH, file);

    int fd = open(path, O_WRONLY);
    if (fd < 0) return;
    write(fd, value, strlen(value));
    close(fd);
}

// Helper: write integer value to PWM file
static void write_pwm_int(const char* file, long value) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%ld", value);
    write_pwm(file, buf);
}

// Set PWM frequency in Hz (50% duty cycle)
void PWM_setFrequency(int freq) {
    if (freq <= 0) {
        write_pwm("enable", "0");
        currentFreq = 0;
        return;
    }

    // Only update if frequency changed
    if (freq == currentFreq) return;

    long period_ns = 1000000000L / freq;   // period in ns
    long duty_ns = period_ns / 2;          // 50% duty cycle

    write_pwm_int("duty_cycle", 0);        // Reset first
    write_pwm_int("period", period_ns);
    write_pwm_int("duty_cycle", duty_ns);
    write_pwm("enable", "1");

    currentFreq = freq;
}

// Get current frequency
int PWM_getFrequency(void) {
    return currentFreq;
}

// Initialize PWM
void PWM_init(void) {
    // Export PWM if not already exported
    write_pwm("export", "0");  // Export pwm0; may fail if already exported
    usleep(500000);            // wait 500ms

    pwmRunning = true;
    PWM_setFrequency(10);      // default 10Hz
}

// Cleanup PWM
void PWM_cleanup(void) {
    pwmRunning = false;
    write_pwm("enable", "0");  // Turn off
}

