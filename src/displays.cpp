#include "displays.h"
#include "sound.h"
#include <Wire.h>

const PROGMEM uint8_t display_ctl_addr[4]  = {0x3A, 0x3E, 0x3C, 0x38};
const PROGMEM uint8_t display_data_addr[4] = {0x3B, 0x3F, 0x3D, 0x39};

const PROGMEM uint8_t gauge_addr[4] = {0xC8, 0xE8, 0xA8, 0x88};

uint8_t display_buffers[4][41];
uint8_t brightness = 255;

struct gauge_t gauge_status[4];

#define I2C_LSTR(ADDR, STR) {const size_t l = sizeof(STR); write_addr(ADDR, STR, l);};

void write_addr(uint8_t addr, const char *bytes, size_t length) {
  for (size_t pos = 0; pos < length; pos += 32) {
    size_t real_len = length > 32 ? 32 : length; // Send in 32 byte chunks
  
    Wire.beginTransmission(addr);
    Wire.write(&bytes[pos], real_len);
    Wire.endTransmission();
  }
}

void init_displays() {
  for (uint8_t d=0; d < 4; d++) {
    const uint8_t CTL=display_ctl_addr[d];
    I2C_LSTR(CTL, "\xE2");
    I2C_LSTR(CTL, "\x20");
    I2C_LSTR(CTL, "\xC0");
    I2C_LSTR(CTL, "\x8D");
    I2C_LSTR(CTL, "\xEB");
    I2C_LSTR(CTL, "\x81\x30");
    I2C_LSTR(CTL, "\xB5");
    I2C_LSTR(CTL, "\xA1");
    I2C_LSTR(CTL, "\x31");
    I2C_LSTR(CTL, "\x46");
    I2C_LSTR(CTL, "\x2D");
    I2C_LSTR(CTL, "\x85");
    I2C_LSTR(CTL, "\xF2\x00");
    I2C_LSTR(CTL, "\xF3\x07");
    I2C_LSTR(CTL, "\x90");
    I2C_LSTR(CTL, "\xAF");
    I2C_LSTR(CTL, "\x40");
  }
}

void display_set_xpos(uint8_t disp, char x) {
  if (disp >= 4)
    return;

  char buf[] = {'\xB0', '\x10', x};

  write_addr(display_ctl_addr[disp], buf, 3);
}
uint8_t disp_ctr = 0;

void display_send(uint8_t disp) {
  if (disp >= 4)
    return;

  display_set_xpos(disp, 0);

  write_addr(display_data_addr[disp], (const char *) display_buffers[4], 41);
}

void set_write_gauge(uint8_t dial, uint8_t direction, uint16_t value) {
  if (dial >= 4)
    return;

  const char data[] = {(char)gauge_addr[dial], (char) (value >> 8), (char) (value & 0xFF)};

  write_addr(0x25, data, 3);
}

void display_state_machine() {
  static long last_update=0;
  static uint8_t updating=0;

  // Start updating once we hit 100ms since our last update.  TODO add dirty detection for display_send()
  if (updating == 0 && (vol.millis() - last_update > 100)) {
    last_update = vol.millis();
    updating = 1;
  }

  // Only update one screen at a time, so we can better handle latency for sound generation
  if (updating) {
      display_send(updating-1);
      updating++;
      if (updating > 4)
        updating = 0;
  }
}

void gauge_state_machine() {
  static long last_update=0;
  static uint8_t updating=0;

  // Start updating once we hit 100ms since our last update.  TODO add dirty detection for gauge_send()
  if (updating == 0 && (vol.millis() - last_update > 100)) {
    last_update = vol.millis();
    updating = 1;
  }

  // Only update one gauge at a time, so we can better handle latency for sound generation
  if (updating) {
      //display_send(updating-1);
      updating++;
      if (updating > 4)
        updating = 0;
  }
}