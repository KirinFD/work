#include "joystick.h"
#include "spi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int mcp3208_read_channel(int spi_fd, int channel, uint32_t speed_hz, int *out_value)
{
    if (channel < 0 || channel > 7) return -1;
    uint8_t tx[3], rx[3];
    memset(tx, 0, sizeof(tx));
    memset(rx, 0, sizeof(rx));
    tx[0] = 0x06 | ((channel & 0x04) >> 2);
    tx[1] = (uint8_t)((channel & 0x03) << 6);
    tx[2] = 0x00;
    if (spi_transfer(spi_fd, tx, rx, 3, speed_hz) != 0) {
        return -1;
    }
    int value = ((rx[1] & 0x0F) << 8) | rx[2];
    *out_value = value;
    return 0;
}

int joystick_init(Joystick *j, const char *spidev_path, uint32_t spi_speed_hz,
                  int x_channel, int y_channel, int threshold)
{
    if (!j) return -1;
    memset(j, 0, sizeof(*j));
    if (!spidev_path) spidev_path = "/dev/spidev0.0";
    strncpy(j->spidev_path, spidev_path, sizeof(j->spidev_path) - 1);
    j->spi_speed_hz = spi_speed_hz > 0 ? spi_speed_hz : 1000000;
    j->x_channel = x_channel;
    j->y_channel = y_channel;
    j->threshold = threshold > 0 ? threshold : 600; /* default threshold */

    j->spi_fd = spi_open_device(j->spidev_path, 0, (uint32_t)j->spi_speed_hz);
    if (j->spi_fd < 0) {
        fprintf(stderr, "joystick_init: failed to open spidev %s\n", j->spidev_path);
        return -1;
    }

    /* Calibrate center by averaging several samples */
    const int samples = 64;
    long sumx = 0, sumy = 0;
    for (int i = 0; i < samples; ++i) {
        int vx = 0, vy = 0;
        if (mcp3208_read_channel(j->spi_fd, j->x_channel, j->spi_speed_hz, &vx) != 0) vx = 0;
        if (mcp3208_read_channel(j->spi_fd, j->y_channel, j->spi_speed_hz, &vy) != 0) vy = 0;
        sumx += vx;
        sumy += vy;
        usleep(5000); /* 5ms between samples */
    }
    j->center_x = (int)(sumx / samples);
    j->center_y = (int)(sumy / samples);
    j->calibrated = 1;

    fprintf(stderr, "Joystick HAL: spi=%s speed=%u chX=%d chY=%d centerX=%d centerY=%d threshold=%d\n",
            j->spidev_path, (unsigned)j->spi_speed_hz, j->x_channel, j->y_channel,
            j->center_x, j->center_y, j->threshold);

    return 0;
}

JOY_STATE joystick_get_state(Joystick *j)
{
    if (!j || !j->calibrated) return JSTATE_NONE;
    int vx = 0, vy = 0;
    if (mcp3208_read_channel(j->spi_fd, j->x_channel, j->spi_speed_hz, &vx) != 0) return JSTATE_NONE;
    if (mcp3208_read_channel(j->spi_fd, j->y_channel, j->spi_speed_hz, &vy) != 0) return JSTATE_NONE;

    int dx = vx - j->center_x;
    int dy = vy - j->center_y;

    if (abs(dx) > abs(dy)) {
        /* horizontal movement dominates */
        if (dx < -j->threshold) return JSTATE_LEFT;
        if (dx > j->threshold) return JSTATE_RIGHT;
    } else {
        /* vertical movement dominates */
        if (dy < -j->threshold) return JSTATE_UP;
        if (dy > j->threshold) return JSTATE_DOWN;
    }
    return JSTATE_NONE;
}

void joystick_cleanup(Joystick *j)
{
    if (!j) return;
    if (j->spi_fd >= 0) spi_close_device(j->spi_fd);
    j->spi_fd = -1;
    j->calibrated = 0;
}