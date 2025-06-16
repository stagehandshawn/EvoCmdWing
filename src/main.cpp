// Written by ShawnR
// Built for Teensy 4.1, way overpowered but has many pins
// Sends MIDI for Grandma3 Encoders on first 5 CCs of channel 1 using relative velocity
// Sends absolute CC values for next 8 encoders for 8 Xkeys, and recieves MIDI back on same CCs for storing current value

// Sends notes on encoder click notes: 1-13
// Setup Additional Midiremotes for encoder click actions, can be an off key or flash, and will be seperate from the XKey

#include <Arduino.h>
#include <MIDIUSB.h>
#include <Encoder.h>


void handleEncoders();
void handleButtons();
void sendMidiEncoder(int index, int direction);
void handleIncomingMIDI();
void checkSerialForReboot();


const int enc_num = 13;
const int N_BUTTONS = 13;

//first 5 are for encoders, next 8 are for XKeys
const byte custom_midi[enc_num] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13};

//first 5 for encoders, next 8 for XKeys
const byte buttonNotes[N_BUTTONS] = {1,2,3,4,5,6,7,8,9,10,11,12,13};

// Here you can play with the velocity scaling to get the sensitivity you like
// This works well for me with no accel in Pro Plugins Midi Encoders plugin
const int velocityScale[8] = {1, 1, 2, 2, 3, 4, 5, 6};

// Array to store current values for encoders 5-12 (absolute mode)
int encoderValues[enc_num] = {0}; // Initialize all to 0

// I wired the encoders backwards so the pins are reversed here
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

void setup() {
  Serial.begin(115200);
  Serial.print("EvoCmdWing Encoder setup");

  for (int i = 0; i < enc_num; i++) {
    encoders[i] = new Encoder(enc_pins[i][0], enc_pins[i][1]);
    lastPos[i] = encoders[i]->read();

    if (i >= 5) {
      encoderValues[i] = 0;
    }
  }
  for (int i = 0; i < N_BUTTONS; i++) {
    pinMode(BUTTON_ARDUINO_PIN[i], INPUT_PULLUP);
  }
  Serial.print("Setup complete");
}

void loop() {
  handleEncoders();
  handleButtons();
  usbMIDI.send_now();

  handleIncomingMIDI();

  checkSerialForReboot();
}

void handleEncoders() {
  for (int i = 0; i < enc_num; i++) {
    long movement = encoders[i]->readAndReset();
    encoderBuffer[i] += movement;

    // if (movement != 0) {
    //   Serial.print("[ENC] Index ");
    //   Serial.print(i);
    //   Serial.print(" moved ");
    //   Serial.println(movement);
    // }

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
        int note = buttonNotes[i];
        int velocity = (reading == LOW) ? 1 : 0;

        usbMIDI.sendNoteOn(note, velocity, midiCh, 0);

        Serial.print("[MIDI OUT] Button ");
        Serial.print(i);
        Serial.print(" → Note: ");
        Serial.print(note);
        Serial.print(" | Vel: ");
        Serial.print(velocity);
        Serial.print(" | Ch: ");
        Serial.println(midiCh);

        buttonPState[i] = reading;
      }
    }
  }
}

//setup for midi mode 3, can change to 2's comp mode 1 if needed
// Using grandma3 plugin MidiEncoders from ProPlugins

void sendMidiEncoder(int index, int direction) {
  unsigned long now = millis();
  unsigned long elapsed = now - lastMoveTime[index];
  lastMoveTime[index] = now;

  int final_value;

  if (index < 5) {
    // First 5 encoders: velocity-based mode for plugin
    int level = constrain((int)(elapsed / 15), 0, 7);
    int scaled = velocityScale[7 - level];  // fast = high index = smaller scaled value

    if (direction > 0) {
      final_value = scaled;  // ➕ right = 1–8 (no change)
    } else {
      final_value = 64 + scaled ; // left 65 and up
    }
  } else {
    // Encoders 5-12: absolute value mode (0-127)
    int level = constrain((int)(elapsed / 15), 0, 7);
    int step = velocityScale[7 - level];  // Use same velocity scaling for step size
    
    if (direction > 0) {
      encoderValues[index] = constrain(encoderValues[index] + step, 0, 127);
    } else {
      encoderValues[index] = constrain(encoderValues[index] - step, 0, 127);
    }
    
    final_value = encoderValues[index];
  }

  usbMIDI.sendControlChange(custom_midi[index], final_value, midiCh, 0);

  Serial.print("[ENCODER] Index: ");
  Serial.print(index);
  Serial.print(" | Dir: ");
  Serial.print(direction > 0 ? "+" : "-");
  Serial.print(" | CC: ");
  Serial.print(custom_midi[index]);
  Serial.print(" | Value Sent: ");
  Serial.print(final_value);
  if (index >= 5) {
    Serial.print(" | Stored: ");
    Serial.print(encoderValues[index]);
  }
  Serial.print(" | Elapsed: ");
  Serial.print(elapsed);
  Serial.println(" ms");
}

void handleIncomingMIDI() {
  while (usbMIDI.read()) {
    byte type = usbMIDI.getType();
    byte ch = usbMIDI.getChannel();
    byte d1 = usbMIDI.getData1();
    byte d2 = usbMIDI.getData2();

    // Only process CC messages on our MIDI channel
    if (type == usbMIDI.ControlChange && ch == midiCh) {  // CC message
      // Check if this CC matches one of our encoders 5-12
      for (int i = 5; i < enc_num; i++) {
        if (d1 == custom_midi[i]) {
          encoderValues[i] = constrain(d2, 0, 127);
          
          Serial.print("[MIDI IN] CC Update - Encoder ");
          Serial.print(i);
          Serial.print(" | CC: ");
          Serial.print(d1);
          Serial.print(" | Value: ");
          Serial.println(encoderValues[i]);
          break;
        }
      }
    }
    
    Serial.print("[MIDI IN] Type: ");
    switch (type) {
      case 0x08: Serial.print("Note Off   "); break;
      case 0x09: Serial.print("Note On    "); break;
      case 0x0A: Serial.print("Aftertouch "); break;
      case 0x0B: Serial.print("CC         "); break;
      case 0x0C: Serial.print("Program    "); break;
      case 0x0D: Serial.print("Channel Pressure"); break;
      case 0x0E: Serial.print("Pitch Bend "); break;
      default:   Serial.print("Other      "); break;
    }
    Serial.print(" Ch: "); Serial.print(ch);
    Serial.print(" D1: "); Serial.print(d1);
    Serial.print(" D2: "); Serial.println(d2);
    
  }
}


//================================
// UPLOAD Function 
//================================
//Upload without pressing button, using python script, takes one second try
void checkSerialForReboot() {
    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim(); // Remove any whitespace/newlines
        
        if (cmd == "REBOOT_BOOTLOADER") {
            Serial.println("[REBOOT] Command received. Entering bootloader...");
            Serial.flush(); // Important: ensure message is sent before reboot
            delay(100);
            
            // This is the correct method for ALL Teensy models
            _reboot_Teensyduino_();
            
        } else if (cmd == "REBOOT_NORMAL") {
            Serial.println("[REBOOT] Normal reboot requested...");
            Serial.flush();
            delay(100);
            
            // Normal restart using ARM AIRCR register
            SCB_AIRCR = 0x05FA0004;
            
        } else {
            Serial.print("[REBOOT] Unknown command: ");
            Serial.println(cmd);
        }
    }
}