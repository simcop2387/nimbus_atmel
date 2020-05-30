#include <Volume.h>

extern Volume vol; // TODO move this to a more appropriate place

union audiobuf_t {
  struct {
    unsigned int is_command:1;
    unsigned int length:3; // note length
    unsigned int attack:1; // Should we attack the note
    unsigned int note:6;   // which note to play
    unsigned int volume:5; // Volume to play, scaled to 8bit (volume << 3 | volume >> 2)
  } note;
  struct {
    unsigned int is_command:1;
    unsigned int cmd_value:3;
    unsigned int cmd_param_a:6;
    unsigned int cmd_param_b:6;
  } command;
  struct {uint8_t high, low;} fill;
};

extern audiobuf_t audiobuff[64];
extern uint8_t audiobuff_length;
extern uint8_t audiobuff_loop_size;
extern uint8_t audiobuff_loop_count;

void sound_state_machine();