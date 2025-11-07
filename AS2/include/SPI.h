#ifndef SPI_H
#define SPI_H

#include <stdint.h>

int spi_init(const char *device, uint32_t speed_hz);
int readADC(int fd, int channel, uint32_t speed_hz);

#endif

