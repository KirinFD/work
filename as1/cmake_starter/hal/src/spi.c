#include "hal/spi.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <linux/spi/spidev.h>
#include <string.h>
#include <errno.h>

int spi_open_device(const char *device, uint8_t mode, uint32_t speed_hz)
{
    int fd = open(device, O_RDWR);
    if (fd < 0) {
        perror("spi_open_device: open");
        return -1;
    }
    if (ioctl(fd, SPI_IOC_WR_MODE, &mode) < 0) {
        perror("spi_open_device: SPI_IOC_WR_MODE");
        close(fd);
        return -1;
    }
    if (ioctl(fd, SPI_IOC_RD_MODE, &mode) < 0) {
        perror("spi_open_device: SPI_IOC_RD_MODE");
        close(fd);
        return -1;
    }
    if (ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed_hz) < 0) {
        perror("spi_open_device: SPI_IOC_WR_MAX_SPEED_HZ");
        close(fd);
        return -1;
    }
    uint32_t read_speed = 0;
    if (ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &read_speed) < 0) {
        perror("spi_open_device: SPI_IOC_RD_MAX_SPEED_HZ");
        close(fd);
        return -1;
    }
    /* Optionally check that read_speed == speed_hz */
    (void)read_speed;
    return fd;
}

int spi_transfer(int fd, const uint8_t *tx, uint8_t *rx, size_t len, uint32_t speed_hz)
{
    if (fd < 0) return -1;
    struct spi_ioc_transfer tr;
    memset(&tr, 0, sizeof(tr));
    tr.tx_buf = (unsigned long)tx;
    tr.rx_buf = (unsigned long)rx;
    tr.len = len;
    tr.delay_usecs = 0;
    tr.speed_hz = speed_hz;
    tr.bits_per_word = 8;

    int ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
    if (ret < 1) {
        perror("spi_transfer: SPI_IOC_MESSAGE");
        return -1;
    }
    return 0;
}

void spi_close_device(int fd)
{
    if (fd >= 0) close(fd);
}