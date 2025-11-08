#ifndef _UDP_LISTENER_H_
#define _UDP_LISTENER_H_

#include <pthread.h>

// Function that starts the UDP listening thread
void UDP_init(void);

// Function that stops the UDP listener thread
void UDP_cleanup(void);

// The function run by the UDP thread
void* udpFunc(void* arg);

#endif

