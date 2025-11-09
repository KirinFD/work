#include "hal/udp_listener.h"
#include "hal/sampler.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define BUF_SIZE 1500
#define UDP_PORT 12345

static int udp_sock = -1;
static pthread_t udp_thread;
static volatile bool stop_requested = false;

// Keep track of last command for empty line handling
static char last_command[BUF_SIZE] = "";

// Handle a single command
static void handleCommand(const char* cmd, struct sockaddr_in* cli, int sock) {
    char buf[BUF_SIZE];

    if (strcmp(cmd, "help") == 0 || strcmp(cmd, "?") == 0) {
        snprintf(buf, sizeof(buf),
                 "Accepted command examples:\n"
                 "count -- get the total number of samples taken.\n"
                 "length -- get the number of samples taken in the previously completed second.\n"
                 "dips -- get the number of dips in the previously completed second.\n"
                 "history -- get all the samples in the previously completed second.\n"
                 "stop -- cause the server program to end.\n"
                 "<enter> -- repeat last command.\n");
    } else if (strcmp(cmd, "count") == 0) {
        snprintf(buf, sizeof(buf), "# samples taken total: %lld\n", Sampler_getNumSamplesTaken());
    } else if (strcmp(cmd, "length") == 0) {
        snprintf(buf, sizeof(buf), "# samples taken last second: %d\n", Sampler_getHistorySize());
    } else if (strcmp(cmd, "dips") == 0) {
        snprintf(buf, sizeof(buf), "# Dips: %d\n", Sampler_countDips());
    } else if (strcmp(cmd, "history") == 0) {
        int sz;
        double* hist = Sampler_getHistory(&sz);
        int max_per_line = 10;
        for (int i = 0; i < sz; i++) {
            snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), "%.3f%s", hist[i], (i < sz - 1) ? ", " : "\n");
            if ((i + 1) % max_per_line == 0) {
                strncat(buf, "\n", sizeof(buf) - strlen(buf) - 1);
            }
        }
        free(hist);
    } else if (strcmp(cmd, "stop") == 0) {
        snprintf(buf, sizeof(buf), "Program terminating.\n");
        stop_requested = true;
    } else {
        snprintf(buf, sizeof(buf), "Unknown command: %.1480s\n", cmd);
    }

    sendto(sock, buf, strlen(buf), 0, (struct sockaddr*)cli, sizeof(*cli));
}

void* udpFunc(void* arg) {
    (void)arg;
    struct sockaddr_in cli;
    socklen_t cli_len = sizeof(cli);
    char buf[BUF_SIZE];

    while (!stop_requested) {
        int n = recvfrom(udp_sock, buf, sizeof(buf) - 1, 0, (struct sockaddr*)&cli, &cli_len);
        if (n > 0) {
            buf[n] = '\0';

            // Remove trailing newline/carriage return
            buf[strcspn(buf, "\r\n")] = '\0';

            // Use last command if user pressed enter
            char* cmd_to_process = (strlen(buf) == 0) ? last_command : buf;
            strncpy(last_command, cmd_to_process, sizeof(last_command) - 1);
            last_command[sizeof(last_command) - 1] = '\0';

            handleCommand(cmd_to_process, &cli, udp_sock);
        }
    }

    return NULL;
}

void UDP_init(void) {
    udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_sock < 0) {
        perror("socket");
        exit(1);
    }

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(UDP_PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(udp_sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(udp_sock);
        exit(1);
    }

    stop_requested = false;
    pthread_create(&udp_thread, NULL, udpFunc, NULL);
}

void UDP_cleanup(void) {
    stop_requested = true;
    pthread_join(udp_thread, NULL);
    close(udp_sock);
}
