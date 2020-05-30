#include <Arduino.h>

extern const PROGMEM uint8_t display_ctl_addr[4];
extern const PROGMEM uint8_t display_data_addr[4];

extern const PROGMEM uint8_t gauge_addr[4];

extern uint8_t display_buffers[4][41];
extern uint8_t brightness;

struct gauge_t {
  uint8_t cur_pos;
  uint8_t tgt_pos;
  uint32_t last_update;
};
extern struct gauge_t gauge_status[4];

void init_displays();
void gauge_state_machine();
void display_state_machine();
