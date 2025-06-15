#include <Arduino.h>

#include <MIDIUSB.h>
#include <Encoder.h>
//#include <Adafruit_NeoPixel.h>

//#define LED_PIN 22
//#define NUM_FEEDBACK_LEDS 8

const int enc_num = 13;
const int N_BUTTONS = 13;



//first 5 are for encoders, next 8 are for XKeys
const byte custom_midi[enc_num] = {1, 2, 3, 4, 5, 24, 25, 26, 27, 28, 29, 30, 31};

//first 5 for encoders, next 8 for XKeys
const byte buttonNotes[N_BUTTONS] = {6,7,8,9,10,32,33,34,35,36,37,38,39};

//const byte feedbackNotes[NUM_FEEDBACK_LEDS] = {56,57,58,59,60,61,62,63};
//const int feedbackStartIndex = 5;

//const int velocityScale[8] = {1, 1, 2, 2, 3, 3, 4, 4};  //defualt from first sketch
const int velocityScale[8] = {1, 1, 2, 2, 3, 4, 5, 6};

const int enc_pins[enc_num][2] = {
  {1,0}, {3,2}, {5,4}, {7,6}, {9,8}, {11,10}, {15,14},
  {17,16}, {19,18}, {21,20}, {23,22}, {25,24}, {27,26}
};

const int BUTTON_ARDUINO_PIN[N_BUTTONS] = {
  28,29,30,31,32,33,34,35,36,37,38,39,40
};

//midi channel to send on
byte midiCh = 1;

Encoder* encoders[enc_num];
long lastPos[enc_num] = {0};
unsigned long lastMoveTime[enc_num] = {0};
int buttonPState[N_BUTTONS] = {};
unsigned long lastDebounceTime[N_BUTTONS] = {0};
unsigned long debounceDelay = 50;
static int encoderBuffer[enc_num] = {0};

//Adafruit_NeoPixel strip(NUM_FEEDBACK_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

void setup() {
  Serial.begin(115200);
  Serial.print(" setup start ");


  //strip.begin();
  //strip.fill(strip.Color(255, 255, 0)); strip.show();
  //delay(50);
  //strip.show();

  usbMIDI.sendControlChange(10, 42, 1, 0); // Port 1
  usbMIDI.sendControlChange(11, 43, 1, 1); // Port 2
  usbMIDI.send_now();

  for (int i = 0; i < enc_num; i++) {
    encoders[i] = new Encoder(enc_pins[i][0], enc_pins[i][1]);
    lastPos[i] = encoders[i]->read();
  }
  for (int i = 0; i < N_BUTTONS; i++) {
    pinMode(BUTTON_ARDUINO_PIN[i], INPUT_PULLUP);
  }
  Serial.print(" setup end ");
}

void loop() {
  handleEncoders();
  handleButtons();
//handleIncomingMIDI();
  usbMIDI.send_now();
}

void handleEncoders() {
  for (int i = 0; i < enc_num; i++) {
    long movement = encoders[i]->readAndReset();
    encoderBuffer[i] += movement;

    if (movement != 0) {
      Serial.print("[ENC] Index ");
      Serial.print(i);
      Serial.print(" moved ");
      Serial.println(movement);
    }

    while (abs(encoderBuffer[i]) >= 4) {
      int dir = (encoderBuffer[i] > 0) ? 1 : -1;
      sendMidiEncoder(i, dir);
      encoderBuffer[i] -= (4 * dir);
    }
  }
}

void handleButtons() {
  for (int i = 0; i < N_BUTTONS; i++) {
    int reading = digitalRead(BUTTON_ARDUINO_PIN[i]);
    if ((millis() - lastDebounceTime[i]) > debounceDelay) {
      if (reading != buttonPState[i]) {
        lastDebounceTime[i] = millis();
        int cable = (i < 5) ? 0 : 1;
        int note = buttonNotes[i];
        int velocity = (reading == LOW) ? 1 : 0;

        usbMIDI.sendNoteOn(note, velocity, midiCh, cable);

        Serial.print("[MIDI OUT] Button ");
        Serial.print(i);
        Serial.print(" → Note: ");
        Serial.print(note);
        Serial.print(" | Vel: ");
        Serial.print(velocity);
        Serial.print(" | Ch: ");
        Serial.print(midiCh);
        Serial.print(" | Port: ");
        Serial.println(cable + 1);

        buttonPState[i] = reading;
      }
    }
  }
}

//setup for midi mode 3, can change to 2's comp mode 1 if needed

void sendMidiEncoder(int index, int direction) {
  unsigned long now = millis();
  unsigned long elapsed = now - lastMoveTime[index];
  lastMoveTime[index] = now;

  int level = constrain((int)(elapsed / 15), 0, 7);
  int scaled = velocityScale[7 - level];  // fast = high index = smaller scaled value

  int final_value;
  if (direction > 0) {
    final_value = scaled;  // ➕ right = 1–8 (no change)
  } else {
    final_value = 72 - (scaled - 1);  // ➖ left: 1 → 72, 2 → 71, ..., 6 → 67
  }

  int cable = (index < 5) ? 0 : 1;
  usbMIDI.sendControlChange(custom_midi[index], final_value, midiCh, cable);

  Serial.print("[ENCODER] Index: ");
  Serial.print(index);
  Serial.print(" | Dir: ");
  Serial.print(direction > 0 ? "+" : "-");
  Serial.print(" | CC: ");
  Serial.print(custom_midi[index]);
  Serial.print(" | Value Sent: ");
  Serial.print(final_value);
  Serial.print(" | Elapsed: ");
  Serial.print(elapsed);
  Serial.print(" ms | Step: ");
  Serial.print(scaled);
  Serial.print(" | Port: ");
  Serial.println(cable + 1);
}






// void handleIncomingMIDI() {
//   while (usbMIDI.read()) {
//     byte type = usbMIDI.getType();

//     if (type == usbMIDI.SystemExclusive) {
//       const uint8_t* sysex = usbMIDI.getSysExArray();
//       unsigned int len = usbMIDI.getSysExArrayLength();

//       Serial.print("[MIDI IN] SysEx: ");
//       for (unsigned int i = 0; i < len; i++) {
//         if (sysex[i] < 0x10) Serial.print("0");
//         Serial.print(sysex[i], HEX);
//         Serial.print(" ");
//       }
//       Serial.println();

//       if (len >= 14 && sysex[0] == 0xF0 && sysex[1] == 0x00 && sysex[2] == 0x00 && sysex[3] == 0x66 && sysex[4] == 0x14 && sysex[5] == 0x72) {
//         for (int i = 0; i < NUM_FEEDBACK_LEDS; i++) {
//           byte colorId = sysex[6 + i];
//           uint32_t color;
//           switch (colorId) {
//             case 0x01: color = strip.Color(255, 0, 0); break;
//             case 0x02: color = strip.Color(0, 255, 0); break;
//             case 0x03: color = strip.Color(255, 255, 0); break;
//             case 0x04: color = strip.Color(0, 0, 255); break;
//             case 0x05: color = strip.Color(255, 0, 255); break;
//             case 0x06: color = strip.Color(0, 255, 255); break;
//             case 0x07: color = strip.Color(255, 255, 255); break;
//             default:   color = strip.Color(0, 0, 0); break;
//           }
//           strip.setPixelColor(i, color);
//         }
//         strip.show();
//       }
//       return;
//     }

//     byte ch = usbMIDI.getChannel();
//     byte d1 = usbMIDI.getData1();
//     byte d2 = usbMIDI.getData2();

//     if (type == 0xE) {  // Pitch Bend
//       int pitchValue = ((d2 << 7) | d1); // 14-bit
//       int scaled = map(pitchValue, 0, 16383, 0, 127);
//       Serial.print("[PITCH] Raw: ");
//       Serial.print(pitchValue);
//       Serial.print(" → Scaled: ");
//       Serial.println(scaled);
//     }

//     Serial.print("[MIDI IN] Type: ");
//     switch (type) {
//       case 0x08: Serial.print("Note Off   "); break;
//       case 0x09: Serial.print("Note On    "); break;
//       case 0x0A: Serial.print("Aftertouch "); break;
//       case 0x0B: Serial.print("CC         "); break;
//       case 0x0C: Serial.print("Program    "); break;
//       case 0x0D: Serial.print("Channel Pressure"); break;
//       case 0x0E: Serial.print("Pitch Bend "); break;
//       default:   Serial.print("Other      "); break;
//     }

//     Serial.print(" Ch: "); Serial.print(ch);
//     Serial.print(" D1: "); Serial.print(d1);
//     Serial.print(" D2: "); Serial.println(d2);

//     if (type == 0x09) {
//       for (int i = 0; i < NUM_FEEDBACK_LEDS; i++) {
//         if (d1 == feedbackNotes[i]) {
//           strip.setPixelColor(i, d2 > 0 ? strip.Color(255, 255, 255) : 0);
//           strip.show();
//         }
//       }
//     }
//   }
//}
