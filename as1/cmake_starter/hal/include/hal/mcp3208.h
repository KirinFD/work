#ifndef _MCP3208_H_
#define _MCP3208_H_

#include <stdint.h>

/* Simple MCP3208 handle */
typedef struct {
    int fd;                     /* spidev fd, <0 if not opened */
    char device[256];           /* device path */
    uint32_t speed_hz;          /* SPI speed */
} MCP3208;

/* Initialize the MCP3208 handle. Returns 0 on success, -1 on error.*/
int mcp3208_init(MCP3208 *h, const char *spidev_path, uint32_t spi_speed_hz);

/* Read a channel (0..7). Returns 0 on success and writes 0..4095 to out_value. */
int mcp3208_read_channel(MCP3208 *h, int channel, int *out_value);

void mcp3208_close(MCP3208 *h);

#endif