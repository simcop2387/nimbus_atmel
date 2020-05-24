#include "comms.h"
#include "displays.h"
#include "sound.h"

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

