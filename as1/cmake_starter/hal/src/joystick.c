#include "hal/joystick.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>

struct Joystick {
    MCP3208 *adc;           /* may be NULL -> simulation */
    int x_channel;
    int y_channel;
    int threshold;
    int center_x;
    int center_y;
    int calibrated;
};

int joystick_init(Joystick **j_out, MCP3208 *adc, int x_channel, int y_channel, int threshold)
{
    if (!j_out) return -1;
    Joystick *j = (Joystick *)calloc(1, sizeof(Joystick));
    if (!j) return -1;
    j->adc = adc;
    j->x_channel = x_channel;
    j->y_channel = y_channel;
    j->threshold = threshold > 0 ? threshold : 600;
    j->center_x = 0;
    j->center_y = 0;
    j->calibrated = 0;

    if (adc && adc->fd >= 0) {
        /* Calibrate center by sampling */
        const int samples = 64;
        long sumx = 0, sumy = 0;
        for (int i = 0; i < samples; ++i) {
            int vx = 0, vy = 0;
            if (mcp3208_read_channel(adc, j->x_channel, &vx) != 0) vx = 0;
            if (mcp3208_read_channel(adc, j->y_channel, &vy) != 0) vy = 0;
            sumx += vx;
            sumy += vy;
            usleep(5000);
        }
        j->center_x = (int)(sumx / samples);
        j->center_y = (int)(sumy / samples);
        j->calibrated = 1;
        fprintf(stderr, "joystick_init: calibrated center_x=%d center_y=%d threshold=%d\n",
                j->center_x, j->center_y, j->threshold);
    } else {
        /* Simulation mode: mark calibrated so joystick_get_state() will use keyboard */
        j->calibrated = 1;
    }

    *j_out = j;
    return 0;
}

JOY_STATE joystick_get_state(Joystick *j)
{
    if (!j || !j->calibrated) return JSTATE_NONE;
    int vx = 0, vy = 0;
    if (mcp3208_read_channel(j->adc, j->x_channel, &vx) != 0) return JSTATE_NONE;
    if (mcp3208_read_channel(j->adc, j->y_channel, &vy) != 0) return JSTATE_NONE;
    int dx = vx - j->center_x;
    int dy = vy - j->center_y;
    if (abs(dx) > abs(dy)) {
        if (dx < -j->threshold) return JSTATE_LEFT;
        if (dx > j->threshold) return JSTATE_RIGHT;
    } else {
        if (dy < -j->threshold) return JSTATE_UP;
        if (dy > j->threshold) return JSTATE_DOWN;
    }
    return JSTATE_NONE;
}

void joystick_cleanup(Joystick *j)
{
    if (!j) return;
    /* We do not close the MCP3208 here — caller owns it */
    free(j);
}