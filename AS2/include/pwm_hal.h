#ifndef PWM_HAL_H
#define PWM_HAL_H

void PWM_init(void);
void PWM_setFrequency(int freq_hz);
int  PWM_getFrequency(void);
void PWM_cleanup(void);

#endif

