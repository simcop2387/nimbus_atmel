#include <Arduino.h>
#include <Wire.h>
#include <RingBuf.h>
#include "sound.h"
#include "comms.h"
#include "displays.h"

const int output_pins[]={LED_BUILTIN, 16};


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
  init_displays();
  Serial.println("Displays initted");
}

uint32_t last_update = 0;
uint8_t led_status = 0;

void loop() {
  // Make some display stuff
  check_decode_serial();
  sound_state_machine();
  display_state_machine();
  gauge_state_machine();
}