#ifndef REPORTER_H
#define REPORTER_H

// Start the reporting thread
void Reporter_start(void);

// Stop the reporting thread
void Reporter_stop(void);

// Set the current PWM frequency (for printing)
void Reporter_setPWMFrequency(int freq);

#endif // REPORTER_H
