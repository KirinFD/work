#ifndef _JOYSTICK_H_
#define _JOYSTICK_H_

#include "joystick.h"
#include <stdint.h>

typedef enum {
    JSTATE_NONE = 0,
    JSTATE_UP,
    JSTATE_DOWN,
    JSTATE_LEFT,
    JSTATE_RIGHT
} JOY_STATE;

typedef struct {
    int spi_fd;
    char spidev_path[256];
    uint32_t spi_speed_hz;
    int x_channel;
    int y_channel;
    int calibrated;
    int center_x;
    int center_y;
    int threshold;
} Joystick;

/* Initialize joystick HAL:
 *  - spidev_path: e.g. "/dev/spidev0.0"
 *  - spi_speed_hz: e.g. 1000000
 *  - x_channel: MCP3208 channel for X (0..7)
 *  - y_channel: MCP3208 channel for Y (0..7)
 *  - threshold: detection threshold (if <=0 default used)
 *
 * Returns 0 on success, -1 on error.
 */
int joystick_init(Joystick *j, const char *spidev_path, uint32_t spi_speed_hz, int x_channel, int y_channel, int threshold);

/* Non-blocking read of joystick state (based on threshold around calibrated center) */
JOY_STATE joystick_get_state(Joystick *j);

void joystick_cleanup(Joystick *j);

#endif