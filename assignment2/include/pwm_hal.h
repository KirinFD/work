#ifndef PWM_HAL_H
#define PWM_HAL_H

void PWM_init(void);             // Initialize PWM (export pin, enable)
void PWM_cleanup(void);          // Stop PWM and cleanup
void PWM_setFrequency(int freq); // Set PWM frequency in Hz (0 = off)

#endif

