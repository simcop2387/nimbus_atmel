#include "sound.h"
#include <Volume.h>

Volume vol; // TODO move this to a more appropriate place

// each note is 2 bytes, 0bT_LLD_A_NNNNNN_VVVVV

// T - Type, 1 bit (when set, the rest is a command not a note)
// L - Note length 2 bits (Whole, Half, Quarter, Eighth) + dotted bit
// A - Attack/Fade, if set will fade the note out/in
// N - Note, 6 bits, cover many octaves see table at top for order
// V - Volume, starting volume, 5 bits scaled to 8bit (volume << 3 | volume >> 2), use the high bits as the low bits when scaling to get more even steps

// Commands are formatted like this
// 0b1_CCC_AAAAAA_BBBBBB
// C - Command number
// A - Parameter A
// B - Parameter B
//
// C = 0  ? Create Chorus? A = length, B = loops?
// C = 1  Set tempo, A = notes per minute, B = fractional notes?
// C = 2  Set attack aggressiveness, A = attack, B = fade?
// C = 3  ?
// C = 4  Play sound effect, A = sound effect, B = parameter passed to effect
// C = 5  Set buffer loop count, A = how many times to play buffer, B = 0, all buffer, else next B notes?
// C = 6  ?
// C = 7  ? Play chorus
audiobuf_t audiobuff[64]; // 64 commands/notes
uint8_t audiobuff_length = 0;
uint8_t audiobuff_loop_size = 0;
uint8_t audiobuff_loop_count = 0;


void sound_state_machine() {

}
