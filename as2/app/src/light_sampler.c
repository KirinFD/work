// light_sampler.c
// Integrates sampler and dip analyzer modules.
// Build: gcc -O2 -Wall -pthread -o light_sampler light_sampler.c sampler.c dips.c
//
// Uses per-sample smoothed averages from sampler for accurate dip hysteresis.
// NOTE: Samples and averages are already in volts (ADC_read returns volts).
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdatomic.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "sampler.h"
#include "dips.h"

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x)/sizeof((x)[0]))
#endif

// Configuration
#define DEFAULT_DIP_THRESHOLD_TRIGGER 0.10   // volts
#define DEFAULT_DIP_THRESHOLD_RESET   0.07   // volts
#define DEFAULT_UDP_PORT              5555
#define DEFAULT_BLINK_HZ              5
#define MIN_BLINK_HZ                  1
#define MAX_BLINK_HZ                  50
#define DEFAULT_DUTY_PERCENT          50

static atomic_bool shutdown_flag;
static atomic_int blink_hz;
static atomic_int duty_percent;

// Store last second stats
typedef struct {
    int    sample_count;
    int    dip_count;
    double min_volts;
    double max_volts;
} prev_second_stats_t;

static prev_second_stats_t g_prev_stats;
static pthread_mutex_t stats_lock;

static DipConfig g_dip_cfg;

// Hardware stubs
static int pwm_set_stub(int freq_hz, int duty) {
    (void)freq_hz; (void)duty;
    return 0;
}

static void sleep_ns(long ns) {
    struct timespec req = {
        .tv_sec  = ns / 1000000000L,
        .tv_nsec = ns % 1000000000L
    };
    while (nanosleep(&req, &req) == -1 && errno == EINTR) {}
}

static uint64_t monotonic_seconds(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec;
}

// PWM thread
static void* pwm_thread_fn(void *arg) {
    (void)arg;
    uint64_t last_sec = 0;
    while (!atomic_load(&shutdown_flag)) {
        uint64_t s = monotonic_seconds();
        if (s != last_sec) {
            last_sec = s;
            int hz = atomic_load(&blink_hz);
            int duty = atomic_load(&duty_percent);
            if (hz < MIN_BLINK_HZ) hz = MIN_BLINK_HZ;
            if (hz > MAX_BLINK_HZ) hz = MAX_BLINK_HZ;
            pwm_set_stub(hz, duty);
        }
        sleep_ns(100000000L); // 100 ms
    }
    return NULL;
}

// Per-second rollover
static void perform_second_rollover(void) {
    Sampler_moveCurrentDataToHistory();
    double *samples = NULL;
    double *avgs    = NULL;
    int sz = 0;
    Sampler_getHistoryWithAverages(&samples, &avgs, &sz);

    prev_second_stats_t st = (prev_second_stats_t){0};
    st.sample_count = sz;

    if (sz > 0 && samples && avgs) {
        st.min_volts = samples[0];
        st.max_volts = samples[0];
        for (int i = 0; i < sz; ++i) {
            if (samples[i] < st.min_volts) st.min_volts = samples[i];
            if (samples[i] > st.max_volts) st.max_volts = samples[i];
        }
        st.dip_count = Dips_count(samples, avgs, sz, g_dip_cfg);
    }

    pthread_mutex_lock(&stats_lock);
    g_prev_stats = st;
    pthread_mutex_unlock(&stats_lock);

    free(samples);
    free(avgs);
}

// Status printing
static void print_status_line(void) {
    pthread_mutex_lock(&stats_lock);
    prev_second_stats_t st = g_prev_stats;
    pthread_mutex_unlock(&stats_lock);
    double avg_volts  = Sampler_getAverageReading();
    printf("[STATUS] total=%lld count=%d dips=%d minV=%.3f maxV=%.3f avgV=%.3f trig=%.3f reset=%.3f blink_hz=%d duty=%d\n",
           Sampler_getNumSamplesTaken(),
           st.sample_count,
           st.dip_count,
           st.min_volts,
           st.max_volts,
           avg_volts,
           g_dip_cfg.trigger_drop_volts,
           g_dip_cfg.reset_drop_volts,
           atomic_load(&blink_hz),
           atomic_load(&duty_percent));
    fflush(stdout);
}

// Signal
static void handle_signal(int sig) {
    (void)sig;
    atomic_store(&shutdown_flag, true);
}

static void usage(const char *prog) {
    fprintf(stderr,
        "Usage: %s [--udp-port <port>] [--trigger <V>] [--reset <V>]\n",
        prog);
}

int main(int argc, char **argv) {
    int udp_port = DEFAULT_UDP_PORT;
    double trigger_v = DEFAULT_DIP_THRESHOLD_TRIGGER;
    double reset_v   = DEFAULT_DIP_THRESHOLD_RESET;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--udp-port") == 0 && i+1 < argc) {
            udp_port = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--trigger") == 0 && i+1 < argc) {
            trigger_v = atof(argv[++i]);
        } else if (strcmp(argv[i], "--reset") == 0 && i+1 < argc) {
            reset_v = atof(argv[++i]);
        } else if (strcmp(argv[i], "--help") == 0) {
            usage(argv[0]);
            return 0;
        } else {
            fprintf(stderr, "Unknown arg: %s\n", argv[i]);
            usage(argv[0]);
            return 1;
        }
    }

    g_dip_cfg = DipConfig_make(trigger_v, reset_v);

    atomic_store(&blink_hz, DEFAULT_BLINK_HZ);
    atomic_store(&duty_percent, DEFAULT_DUTY_PERCENT);
    atomic_store(&shutdown_flag, false);
    pthread_mutex_init(&stats_lock, NULL);

    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    Sampler_init();

    pthread_t th_pwm, th_udp;
    if (pthread_create(&th_pwm, NULL, pwm_thread_fn, NULL) != 0) {
        perror("pthread_create(pwm)");
        atomic_store(&shutdown_flag, true);
    }
    if (pthread_create(&th_udp, NULL, udp_thread_fn, &udp_port) != 0) {
        perror("pthread_create(udp)");
        atomic_store(&shutdown_flag, true);
    }

    uint64_t last_sec = monotonic_seconds();
    while (!atomic_load(&shutdown_flag)) {
        uint64_t now = monotonic_seconds();
        if (now != last_sec) {
            last_sec = now;
            perform_second_rollover();
            print_status_line();
        }
        sleep_ns(10000000L); // 10 ms
    }

    // Final rollover & status
    perform_second_rollover();
    print_status_line();

    pthread_join(th_pwm, NULL);
    pthread_join(th_udp, NULL);
    Sampler_cleanup();
    pthread_mutex_destroy(&stats_lock);
    printf("Exiting.\n");
    return 0;
}