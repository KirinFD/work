#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "sampler.h"
#include "periodTimer.h"
#include "reporter.h"

static pthread_t reporterThread;
static volatile int running = 0;
static volatile int pwmFrequency = 0; // current LED frequency in Hz

// Update the PWM frequency (so printed output includes it)
void Reporter_setPWMFrequency(int freq) {
    pwmFrequency = freq;
}

static void* reporterFunc(void* arg) {
    (void)arg;

    while (running) {
        sleep(1); // wait 1 second

        // Move current second samples into history
        Sampler_moveCurrentDataToHistory();

        // Get samples
        int histSize = 0;
        double* history = Sampler_getHistory(&histSize);

        // Get number of dips and average reading
        int dips = Sampler_countDips();
        double avgLight = Sampler_getAverageReading();

        // Get timing statistics from periodTimer
        Period_statistics_t stats;
        Period_getStatisticsAndClear(0, &stats);

        // Line 1: summary, include PWM frequency
        printf("#Smpl/s = %d Flash @ %dHz avg = %.3fV dips = %d Smpl ms[ %.3f, %.3f] avg %.3f/%d\n",
               histSize,
               pwmFrequency,
               avgLight,
               dips,
               stats.minPeriodInMs,
               stats.maxPeriodInMs,
               stats.avgPeriodInMs,
               histSize);

        // Line 2: 10 evenly spaced samples
        int step = (histSize < 10) ? 1 : histSize / 10;
        for (int i = 0; i < histSize; i += step) {
            printf(" %d:%.3f", i, history[i]);
        }
        printf("\n");

        free(history);
    }

    return NULL;
}

void Reporter_start(void) {
    running = 1;
    pthread_create(&reporterThread, NULL, reporterFunc, NULL);
}

void Reporter_stop(void) {
    running = 0;
    pthread_join(reporterThread, NULL);
}

