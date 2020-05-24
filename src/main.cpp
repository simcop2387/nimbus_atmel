#include <Arduino.h>
#include <Wire.h>
#include <RingBuf.h>
#include <Volume.h>

Volume vol; // TODO move this to a more appropriate place

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

const int output_pins[]={LED_BUILTIN, 16};

const PROGMEM uint8_t display_ctl_addr[]  = {0x3A, 0x3E, 0x3C, 0x38};
const PROGMEM uint8_t display_data_addr[] = {0x3B, 0x3F, 0x3D, 0x39};

const PROGMEM uint8_t gauge_addr[] = {0xC8, 0xE8, 0xA8, 0x88};

uint8_t display_buffers[4][41];
uint8_t brightness = 255;

struct gauge_t {
  uint8_t cur_pos;
  uint8_t tgt_pos;
  uint32_t last_update;
} gauge_status[4];

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
} audiobuff[64]; // 64 commands/notes
uint8_t audiobuff_length = 0;
uint8_t audiobuff_loop_size = 0;
uint8_t audiobuff_loop_count = 0;

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

  write_addr(display_data_addr[disp], (const char *) display_buffers[4], 41);
}

void set_write_gauge(uint8_t dial, uint8_t direction, uint16_t value) {
  if (dial >= 4)
    return;

  const char data[] = {(char)gauge_addr[dial], (char) (value >> 8), (char) (value & 0xFF)};

  write_addr(0x25, data, 3);
}

RingBuf<uint8_t, 132> serial_data;

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
          // TODO analogWrite here? probably not, do that next loop
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
          uint8_t notes = serial_data[1]; // This is the length

          if (notes < 64 && avail >= 2 + notes * 2) {
            serial_data.pop(temp); // Remove command
            // TODO actually store the audio
            for (uint8_t p = 0; p < notes; p++) {
              serial_data.pop(audiobuff[p].fill.high);
              serial_data.pop(audiobuff[p].fill.low); 
            }
            audiobuff_length = notes;
          } else if (notes > 64) {
            Serial.write(SER_COM_AUDIO_BAD); // Sent bad audio data, fix it
            // Clear the bad data
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

void sound_state_machine() {

}

void loop() {
  // Make some display stuff
  check_decode_serial();

  if (vol.millis() - last_update > 100) {
    last_update = vol.millis();

    led_status ^= 1;
    digitalWrite(LED_BUILTIN, led_status ? HIGH : LOW);
  }


}