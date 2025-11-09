#include "hal/sampler.h"
#include "hal/adc_hal.h"
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define HISTORY_MAX 2000
#define SAMPLE_CHANNEL 0
#define SMOOTH_FACTOR 0.999
#define DIP_THRESHOLD 0.1
#define DIP_HYSTERESIS 0.03

static pthread_t samplerThread;
static volatile bool running = false;

static double currentAvg = 0;
static long long numSamples = 0;

static double history[HISTORY_MAX];
static int historySize = 0;

static double currentBuffer[HISTORY_MAX];
static int currentBufferSize = 0;

static int dips = 0;
static bool inDip = false;

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

static void* samplerFunc(void* arg) {
    (void)arg;
    while (running) {
        double sample = ADC_read(SAMPLE_CHANNEL);

        pthread_mutex_lock(&lock);
        if (currentBufferSize < HISTORY_MAX) currentBuffer[currentBufferSize++] = sample;

        if (numSamples == 0) currentAvg = sample;
        else currentAvg = SMOOTH_FACTOR * currentAvg + (1 - SMOOTH_FACTOR) * sample;

        if (!inDip && currentAvg - sample >= DIP_THRESHOLD) {
            dips++;
            inDip = true;
        } else if (inDip && currentAvg - sample <= DIP_THRESHOLD - DIP_HYSTERESIS) {
            inDip = false;
        }

        numSamples++;
        pthread_mutex_unlock(&lock);

        usleep(1000); // 1 ms
    }
    return NULL;
}

void Sampler_init(void) {
    ADC_init();
    running = true;
    pthread_create(&samplerThread, NULL, samplerFunc, NULL);
}

void Sampler_cleanup(void) {
    running = false;
    pthread_join(samplerThread, NULL);
}

void Sampler_moveCurrentDataToHistory(void) {
    pthread_mutex_lock(&lock);
    historySize = currentBufferSize;
    memcpy(history, currentBuffer, historySize * sizeof(double));
    currentBufferSize = 0;
    dips = 0; // reset dips for next second
    pthread_mutex_unlock(&lock);
}

int Sampler_getHistorySize(void) {
    pthread_mutex_lock(&lock);
    int sz = historySize;
    pthread_mutex_unlock(&lock);
    return sz;
}

double* Sampler_getHistory(int* size) {
    pthread_mutex_lock(&lock);
    double* buf = malloc(historySize * sizeof(double));
    memcpy(buf, history, historySize * sizeof(double));
    *size = historySize;
    pthread_mutex_unlock(&lock);
    return buf;
}

double Sampler_getAverageReading(void) {
    pthread_mutex_lock(&lock);
    double avg = currentAvg;
    pthread_mutex_unlock(&lock);
    return avg;
}

long long Sampler_getNumSamplesTaken(void) {
    pthread_mutex_lock(&lock);
    long long n = numSamples;
    pthread_mutex_unlock(&lock);
    return n;
}

int Sampler_countDips(void) {
    pthread_mutex_lock(&lock);
    int d = dips;
    pthread_mutex_unlock(&lock);
    return d;
}

