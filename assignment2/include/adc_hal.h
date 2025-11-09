#ifndef _ADC_HAL_H_
#define _ADC_HAL_H_

void ADC_init(void);
double ADC_read(int channel);
void ADC_cleanup(void);

#endif

