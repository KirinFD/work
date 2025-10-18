#ifndef _SPI_H_
#define _SPI_H_

#include <stdint.h>
#include <stddef.h>

/* Open spidev device. Returns fd or -1 on error. */
int spi_open_device(const char *device, uint8_t mode, uint32_t speed_hz);

/* Perform a full-duplex SPI transfer (tx -> rx). Returns 0 on success, -1 on error. */
int spi_transfer(int fd, const uint8_t *tx, uint8_t *rx, size_t len, uint32_t speed_hz);

/* Close device */
void spi_close_device(int fd);

#endif