#ifndef _JOYSTICK_H_
#define _JOYSTICK_H_

#include "mcp3208.h"

/* Joystick states */
typedef enum {
    JSTATE_NONE = 0,
    JSTATE_UP,
    JSTATE_DOWN,
    JSTATE_LEFT,
    JSTATE_RIGHT
} JOY_STATE;

/* Opaque joystick handle */
typedef struct Joystick Joystick;

int joystick_init(Joystick **j_out, MCP3208 *adc, int x_channel, int y_channel, int threshold);

JOY_STATE joystick_get_state(Joystick *j);

void joystick_cleanup(Joystick *j);

#endif