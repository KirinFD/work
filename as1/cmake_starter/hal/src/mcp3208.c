#include "hal/mcp3208.h"
#include "hal/spi.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>

int mcp3208_init(MCP3208 *h, const char *spidev_path, uint32_t spi_speed_hz)
{
    if (!h) return -1;
    h->fd = -1;
    h->device[0] = '\0';
    h->speed_hz = spi_speed_hz > 0 ? spi_speed_hz : 1000000U;
    if (!spidev_path) spidev_path = "/dev/spidev0.0";
    strncpy(h->device, spidev_path, sizeof(h->device) - 1);
    h->fd = spi_open_device(h->device, 0, h->speed_hz);
    if (h->fd < 0) {
        fprintf(stderr, "mcp3208_init: failed to open %s: %s\n", h->device, strerror(errno));
        h->fd = -1;
        return -1;
    }
    return 0;
}

/* MCP3208 read: 3-byte transaction, return 12-bit result */
int mcp3208_read_channel(MCP3208 *h, int channel, int *out_value)
{
    if (!h || !out_value) return -1;
    if (channel < 0 || channel > 7) return -1;
    if (h->fd < 0) return -1;

    uint8_t tx[3] = {0}, rx[3] = {0};
    tx[0] = 0x06 | ((channel & 0x04) >> 2); /* start + single/diff + msb */
    tx[1] = (uint8_t)((channel & 0x03) << 6);
    tx[2] = 0x00;

    if (spi_transfer(h->fd, tx, rx, 3, h->speed_hz) != 0) {
        return -1;
    }
    int value = ((rx[1] & 0x0F) << 8) | rx[2];
    *out_value = value;
    return 0;
}

void mcp3208_close(MCP3208 *h)
{
    if (!h) return;
    if (h->fd >= 0) spi_close_device(h->fd);
    h->fd = -1;
    h->device[0] = '\0';
    h->speed_hz = 0;
}