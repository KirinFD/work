#include "reporter.h"
#include "sampler.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

static pthread_t reporterThread;
static volatile bool running = false;
static volatile int pwmFrequency = 0;  // Current PWM frequency in Hz

static void* reporterFunc(void* arg) {
    (void)arg;
    while (running) {
        int size = 0;
        double* samples = Sampler_getHistory(&size);

        if (samples && size > 0) {
            // Compute statistics
            double sum = 0;
            int dips = Sampler_countDips();
            for (int i = 0; i < size; i++) sum += samples[i];
            double avg = sum / size;

            // Compute approximate start/end time in ms
            double msPerSample = 1000.0 / size;
            double startMs = 0;
            double endMs = msPerSample * (size - 1);

            // Header line
            printf("#Smpl/s = %d Flash @ %dHz avg = %.3fV dips = %d Smpl ms[ %.3f, %.3f] avg %.3f/%d\n",
                   size, pwmFrequency, Sampler_getAverageReading(), dips,
                   startMs / 1000.0, endMs / 1000.0, avg, size);

            // Sample values: print every N-th sample to reduce output
            int step = size / 10;
            if (step == 0) step = 1;
            for (int i = 0; i < size; i += step) {
                printf("%d:%.3f ", i, samples[i]);
            }
            printf("\n");
        }

        free(samples);
        sleep(1);
    }
    return NULL;
}

void Reporter_init(void) {
    running = true;
    pthread_create(&reporterThread, NULL, reporterFunc, NULL);
}

void Reporter_cleanup(void) {
    running = false;
    pthread_join(reporterThread, NULL);
}

void Reporter_setPWMFrequency(int freq) {
    pwmFrequency = freq;
}
