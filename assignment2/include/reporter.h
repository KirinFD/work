#ifndef _REPORTER_H_
#define _REPORTER_H_

#include <stdbool.h>

// Initialize reporter thread
void Reporter_init(void);

// Stop reporter thread
void Reporter_cleanup(void);

// Set the current PWM/Flash frequency (so the output shows correct Hz)
void Reporter_setPWMFrequency(int freq);

#endif
