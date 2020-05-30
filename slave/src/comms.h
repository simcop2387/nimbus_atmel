#include <Arduino.h>
#include <RingBuf.h>

// All comm commands with the highbit set should only be sent by the slave device

#define SER_COM_GAUGE 0x01
#define SER_COM_DISP  0x02
#define SER_COM_BRIGHT 0x03
#define SER_COM_AUDIO  0x04
// Stop all audio playback
#define SER_COM_AUDIO_STOP 0x05

// This should in theory get sent to the esp when a button is hit
#define SER_COM_BUTTON_A 0x80
#define SER_COM_BUTTON_B 0x81
#define SER_COM_AUDIO_DONE 0xA0
// For when you send audio and we're currently playing something
#define SER_COM_AUDIO_BUSY 0xA1
#define SER_COM_AUDIO_BAD  0xA2

#define SER_COM_RESET_ACK 0xFE
// A command to flush the buffer and pretend like we never got any data
#define SER_COM_RESET 0xFF

void check_decode_serial();