#include <Arduino.h>
#include <Wire.h>
#include <RingBuf.h>

#define SER_COM_GAUGE 0x01
#define SER_COM_DISP  0x02
#define SER_COM_BRIGHT 0x03
#define SER_COM_AUDIO  0x04
// This should in theory get sent to the esp when a button is hit
#define SER_COM_BUTTON_A 0x70
#define SER_COM_BUTTON_B 0x71
#define SER_COM_AUDIO_DONE 0xA0
// For when you send audio and we're currently playing it
#define SER_COM_AUDIO_BUSY 0xA1

#define SER_COM_RESET_ACK 0xFE
// A command to flush the buffer and pretend like we never got any data
#define SER_COM_RESET 0xFF

const int output_pins[]={LED_BUILTIN, 16};

const uint8_t display_ctl_addr[]  = {0x3A, 0x3E, 0x3C, 0x38};
const uint8_t display_data_addr[] = {0x3B, 0x3F, 0x3D, 0x39};

const uint8_t gauge_addr[] = {0xC8, 0xE8, 0xA8, 0x88};

uint8_t display_buffers[4][41];
uint8_t brightness = 255;

struct gauge_t {
  uint8_t cur_pos;
  uint8_t tgt_pos;
  uint32_t last_update;
} gauge_status[4];

#define I2C_LSTR(ADDR, STR) {const size_t l = sizeof(STR); write_addr(ADDR, STR, l);};

void write_addr(uint8_t addr, const char *bytes, size_t length) {
  for (size_t pos = 0; pos < length; pos += 32) {
    size_t real_len = length > 32 ? 32 : length; // Send in 32 byte chunks
  
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
    return;

  const char data[] = {(char)gauge_addr[dial], (char) (value >> 8), (char) (value & 0xFF)};

  write_addr(0x25, data, 3);
}

RingBuf<uint8_t, 128> serial_data;

void check_decode_serial() {
  uint8_t temp;
  size_t ready = Serial.available();
  if (ready > 0) {
    while(ready > 0) {
      ready--;

      serial_data.push(Serial.read());
    }
  }

  uint8_t avail = serial_data.size();
  if (avail > 0) {
    switch(serial_data[0]) {
      case SER_COM_GAUGE:
        if (avail >= 3) {
          serial_data.pop(temp); // drop the command
          serial_data.pop(temp); // temp is now the target gauge


          // ignore any other gauges
          if (temp < 4) {
            serial_data.pop(gauge_status[temp].tgt_pos);
          } else {
            serial_data.pop(temp); // drop the position this was an invalid gauge.
          }
        }
        break;
      case SER_COM_BRIGHT:
        if (avail >= 2) {
          serial_data.pop(temp); // drop the command
          serial_data.pop(brightness);
          // TODO analogWrite here? probably not
        }
        break;
      case SER_COM_DISP:
        if (avail >= 43) { // command, display, data.  1 + 1 + 41
          serial_data.pop(temp); // drop the command
          serial_data.pop(temp); // temp is now the target display

          for (uint8_t p = 0; p < 41; p++)
            serial_data.pop(display_buffers[temp][p]);
        }
      case SER_COM_AUDIO:
        if (avail >= 2) {
          uint8_t notes = serial_data[1]; // This is the length, will never be more than 32 notes

          if (avail >= 2 + notes * 2) {
            // each note is 2 bytes, 0xL_N_VV, length note, and volume.  only 12 notes, in a single octave though.  maybe change to 4 lengths and 6 bits for four octaves?

          // TODO actually store the audio
          for (uint8_t p = 0; p < 2 + notes * 2; p++)
            serial_data.pop(temp);
          }
        }
        break;
      case SER_COM_RESET: // flush the buffer, used to reset things
        while (serial_data.size() > 0)
          serial_data.pop(temp);
        // TODO send ACK
        Serial.write(SER_COM_RESET_ACK);
        break;
      default: // Shouldn't happen, we'll just consume a byte
        serial_data.pop(temp);
        break;
    }
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