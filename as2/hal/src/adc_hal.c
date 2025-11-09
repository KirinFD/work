#include "hal/adc_hal.h"
#include "hal/SPI.h"
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

static int spi_fd = -1;  // handle for SPI device

void ADC_init(void)
{
    // Initialize the SPI interface
    spi_fd = spi_init("/dev/spidev0.0", 500000);
    if (spi_fd < 0) {
        perror("Failed to initialize SPI for ADC");
    } else {
        printf("ADC SPI initialized (fd=%d)\n", spi_fd);
    }
}

double ADC_read(int channel)
{
    if (spi_fd < 0) {
        fprintf(stderr, "ADC_read() called before ADC_init()\n");
        return 0.0;
    }

    int raw = readADC(spi_fd, channel, 500000); // read from SPI ADC
    double voltage = (raw / 4095.0) * 3.3;      // convert to volts (12-bit ADC)
    return voltage;
}

void ADC_cleanup(void)
{
    if (spi_fd >= 0) {
        close(spi_fd);  // <- just close the SPI file descriptor
        spi_fd = -1;
        printf("ADC SPI closed\n");
    }
}

