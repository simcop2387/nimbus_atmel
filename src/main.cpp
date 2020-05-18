#include <Arduino.h>
#include <Wire.h>

const int output_pins[]={LED_BUILTIN, 16};

const uint8_t display_ctl_addr[]  = {0x3A, 0x3E, 0x3C, 0x38};
const uint8_t display_data_addr[] = {0x3B, 0x3F, 0x3D, 0x39};

const uint8_t gauge_addr[] = {0xC8, 0xE8, 0xA8, 0x88};

uint8_t display_buffers[4][41];

struct gauge_t {
  uint8_t cur_pos;
  uint8_t tgt_pos;
  uint32_t last_update;
} gauge_status[4];

#define I2C_LSTR(ADDR, STR) {const size_t l = sizeof(STR); write_addr(ADDR, STR, l);};

void write_addr(uint8_t addr, const char *bytes, size_t length) {
  size_t real_len;

  for (size_t pos = 0; pos < length; pos += 32) {
    real_len = length > 32 ? 32 : length; // Send in 32 byte chunks
  
    Wire.beginTransmission(addr);
    Wire.write(&bytes[pos], real_len);
    Wire.endTransmission();
  }
}

void init_display() {
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

void display_set(uint8_t disp) {
  if (disp >= 4)
    return;

  display_set_xpos(disp, 0);

  write_addr(display_data_addr[disp], (const char *) display_buffers[disp], 41);
}

void set_write_gauge(uint8_t dial, uint8_t direction, uint16_t value) {
  if (dial >= 4)
    dial = 0x0F;

  const char data[] = {(char)gauge_addr[dial], (char) (value >> 8), (char) (value & 0xFF)};

  write_addr(0x25, data, 3);
}

uint8_t serial_pos = 0;
uint8_t serial_end = 0;
uint8_t serial_buf[128] = {0};

void check_decode_serial() {
  if (Serial.available() > 0) {
    
  }
}


void setup() {
  delay(5000);
  Serial.begin(115200);

  Serial.println("Setting up pins");

  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(16, OUTPUT);

  Serial.println("Setting backlight");
  
  digitalWrite(16, HIGH);
  digitalWrite(LED_BUILTIN, HIGH);

  Serial.println("Beginning i2c");

  Wire.begin();
  // 400kHz clock speed for i2c
  Wire.setClock(400000);

  Serial.println("Initializing displays");
  init_display();
  Serial.println("Displays initted");
}

uint32_t last_update = 0;
uint8_t led_status = 0;

void loop() {
  // Make some display stuff

  for (int d = 0; d < 4; d++) {
    for (int x = 0; x < 41; x++) {
      display_buffers[d][x] = x+d*41;
    }
    display_buffers[d][0] = disp_ctr;
  }

  if (millis() - last_update > 100) {
    display_set(0);
    display_set(1);
    display_set(2);
    display_set(3);
    last_update = millis();
    disp_ctr++;

    led_status ^= 1;
    digitalWrite(LED_BUILTIN, led_status ? HIGH : LOW);
  }


}