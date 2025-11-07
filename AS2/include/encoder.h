#ifndef ENCODER_H
#define ENCODER_H

#ifdef __cplusplus
extern "C" {
#endif

// Initialize the encoder hardware and start the polling thread
void Encoder_init(void);

// Stop polling and clean up the encoder hardware
void Encoder_cleanup(void);

// Set a callback function that will be called with each encoder delta
void Encoder_setPositionCallback(void (*callback)(int delta));

#ifdef __cplusplus
}
#endif

#endif // ENCODER_H

