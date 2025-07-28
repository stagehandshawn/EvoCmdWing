#include "encoders.h"
#include "neopixel.h"
#include "utils.h"
#include <MIDIUSB.h>

// ================================
// ENCODER/BUTTON GLOBAL VARIABLES
// ================================

// GPIO pins for encoder a/b
const int enc_pins[N_ENCODERS][2] = {
  {0,1}, {2,3}, {4,5}, {6,7}, {8,9}, {11,12}, {14,15},
  {16,17}, {18,19}, {20,21}, {22,23}, {24,25}, {26,27}
};

// GPIO pin for encoder click, and inner outter button
const int BUTTON_PIN[N_BUTTONS] = {
  28,29,30,31,32,33,34,35,36,37,38,39,40,41
};

// first 5 are for attribute encoders, next 8 are for XKeys encoders
const byte midi_note[N_ENCODERS] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13};

// first 5 for encoders, next 8 for XKeys, last one for inner outter flip
const byte buttonNotes[N_BUTTONS] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14};

// Here you can play with the velocity scaling to get the sensitivity you like
// This works well for me with no accel in Pro Plugins Midi Encoders plugin
const int velocityScales[8][8] = {
  {1, 1, 1, 1, 1, 1, 2, 2},    // Level 1: Very slow (added variation)
  {1, 1, 1, 1, 1, 2, 2, 3},    // Level 2: Slow
  {1, 1, 1, 1, 2, 2, 3, 3},    // Level 3: Somewhat slow
  {1, 1, 1, 2, 2, 3, 4, 4},    // Level 4: Slightly slow
  {1, 1, 2, 2, 3, 4, 5, 6},    // Level 5: Current (your existing scale)
  {1, 2, 3, 4, 5, 6, 7, 8},    // Level 6: Slightly fast
  {2, 3, 4, 5, 6, 8, 10, 12},  // Level 7: Fast
  {3, 4, 6, 8, 10, 12, 15, 18} // Level 8: Very fast
};

// Array to store current values for encoders 5-12 (absolute mode)
int encoderValues[N_ENCODERS] = {0}; // Initialize all to 0

// Encoder sensitivity variables
int relativeEncoderSensitivity = 5;  // Default level 5 for relative encoders
int absoluteEncoderSensitivity = 5;   // Default level 5 for absolute encoders

// Midi channel to send on
byte midiCh = 1;

Encoder* encoders[N_ENCODERS];

long lastPos[N_ENCODERS] = {0};
unsigned long lastMoveTime[N_ENCODERS] = {0};
int buttonPState[N_BUTTONS] = {};
bool latchButtonState = false;
unsigned long lastDebounceTime[N_BUTTONS] = {0};
unsigned long debounceDelay = 50;  // debounce for encoder buttons

int encoderBuffer[N_ENCODERS] = {0};
bool midiDataPending = false;


// Adjustment mode (brightness & sensitivity)
bool adjustMode = false;              // True when button 14 is held down for brightness/sensitivity adjustment
bool sensitivityMode = false;         // True when actively adjusting sensitivity (blocks normal LED updates)
unsigned long button14HoldTime = 0;   // Time when button 14 was pressed

// ================================
// ENCODER AND BUTTON FUNCTIONS
// ================================

void initializeEncoders() {
  for (int i = 0; i < N_ENCODERS; i++) {
    encoders[i] = new Encoder(enc_pins[i][0], enc_pins[i][1]);
    lastPos[i] = encoders[i]->read();

    if (i >= 5) {
      encoderValues[i] = 0;
    }
  }
  for (int i = 0; i < N_BUTTONS; i++) {
    pinMode(BUTTON_PIN[i], INPUT_PULLUP);
  }
  
  pinMode(LATCH_LED_PIN, OUTPUT);
  digitalWrite(LATCH_LED_PIN, LOW);
}

// Handles encoder moves (every 4 in the same direction) and sends midi output
void handleEncoders() {
// Buffer the encoder reads to keep from getting bouncing or jumpy readings
  for (int i = 0; i < N_ENCODERS; i++) {
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

// Handles and sends midi for button presses from encoder and from encoder flip button
void handleButtons() {
  for (int i = 0; i < N_BUTTONS; i++) {
    int reading = digitalRead(BUTTON_PIN[i]);
    if ((millis() - lastDebounceTime[i]) > debounceDelay) {
      if (reading != buttonPState[i]) {
        lastDebounceTime[i] = millis();
        int note = buttonNotes[i];
        int velocity;
        
        if (i == 13) {
          if (reading == LOW) {
            // Button 14 pressed down
            if (!adjustMode) {
              // Start tracking hold time for adjustment mode
              button14HoldTime = millis();
              adjustMode = true;
              debugPrint("[ADJUST] Button 14 held - adjustment mode ON");
            }
            velocity = -1; // Don't send MIDI while held
          } else {
            // Button 14 released
            if (adjustMode) {
              unsigned long holdDuration = millis() - button14HoldTime;
              adjustMode = false;
              sensitivityMode = false;
              updateXKeyLEDs();
              
              // Only toggle latch if it was a quick press (less than 500ms)
              if (holdDuration < 500) {
                latchButtonState = !latchButtonState;
                velocity = latchButtonState ? 127 : 0;
                digitalWrite(LATCH_LED_PIN, latchButtonState ? HIGH : LOW);
                
                debugPrintf("[LATCH BUTTON] Quick press - State: %s | LED: %s", 
                           latchButtonState ? "ON" : "OFF", 
                           latchButtonState ? "ON" : "OFF");
              } else {
                velocity = -1; // Don't send MIDI for long press release
                debugPrintf("[ADJUST] Button 14 released after %lu ms - adjustment mode OFF", holdDuration);
              }
            } else {
              velocity = -1;
            }
          }
        } else {
          velocity = (reading == LOW) ? 1 : 0;
        }
        
        if (velocity != -1) {
          usbMIDI.sendNoteOn(note, velocity, midiCh, 0);
          midiDataPending = true;

          debugPrintf("[MIDI OUT] Button %d → Note: %d | Vel: %d | Ch: %d", i, note, velocity, midiCh);
        }

        buttonPState[i] = reading;
      }
    }
  }
}

// setup for midi mode 3, can change to 2's comp mode 1 if needed
// Using grandma3 plugin MidiEncoders from ProPlugins
void sendMidiEncoder(int index, int direction) {
  unsigned long now = millis();
  unsigned long elapsed = now - lastMoveTime[index];
  if (elapsed < 5) return;  // Skip if less than 5ms since last move, keep from dumping buffer all at once issue we sometimes have got
  lastMoveTime[index] = now;

  // Special handling for encoders 5, 6, 11, 12 and 13 when button 14 is held (brightness/sensitivity adjustment)
  if ((index == 4 || index == 5 || index == 10 || index == 11 || index == 12) && adjustMode) {
    // Calculate step size using same velocity scaling as normal encoders
    int level = constrain((int)(elapsed / 15), 0, 7);
    int baseStep = velocityScales[4][7 - level]; // Use level 5 scale for consistent adjustment speed
    
    if (index == 4) {
      // Encoder 5 controls relativeEncoderSensitivity
      sensitivityMode = true;  // Block normal LED updates
      int step = (baseStep > 3) ? 1 : 1; // Always step by 1 for sensitivity settings
      if (direction > 0) {
        relativeEncoderSensitivity = constrain(relativeEncoderSensitivity + step, 1, 8);
      } else {
        relativeEncoderSensitivity = constrain(relativeEncoderSensitivity - step, 1, 8);
      }
      updateRelativeSensitivityLEDs();
      debugPrintf("[RELATIVE SENSITIVITY] Encoder 5 → Level: %d", relativeEncoderSensitivity);
      
    } else if (index == 5) {
      // Encoder 6 controls absoluteEncoderSensitivity  
      sensitivityMode = true;  // Block normal LED updates
      int step = (baseStep > 3) ? 1 : 1; // Always step by 1 for sensitivity settings
      if (direction > 0) {
        absoluteEncoderSensitivity = constrain(absoluteEncoderSensitivity + step, 1, 8);
      } else {
        absoluteEncoderSensitivity = constrain(absoluteEncoderSensitivity - step, 1, 8);
      }
      updateAbsoluteSensitivityLEDs();
      debugPrintf("[ABSOLUTE SENSITIVITY] Encoder 6 → Level: %d", absoluteEncoderSensitivity);
      
    } else if (index == 10) {
      // Encoder 11 controls logoBrightness
      sensitivityMode = false;  // Allow normal LED updates for brightness adjustment
      float step = baseStep * 0.01f;  // Convert to 0.01 increments (1% steps)
      if (direction > 0) {
        logoBrightness = constrain(logoBrightness + step, 0.0f, 1.0f);
      } else {
        logoBrightness = constrain(logoBrightness - step, 0.0f, 1.0f);
      }
      setLogoPixels(127, 64, 0, logoBrightness);
      debugPrintf("[BRIGHTNESS] Encoder 11 → Dir: %s | Logo brightness: %.2f | Step: %.2f", 
                 direction > 0 ? "+" : "-", logoBrightness, step);

    } else if (index == 11) {
      // Encoder 12 controls offBrightness (populated but off state)
      sensitivityMode = false;  // Allow normal LED updates for brightness adjustment
      float step = baseStep * 0.01f;  // Convert to 0.01 increments (1% steps)
      if (direction > 0) {
        offBrightness = constrain(offBrightness + step, 0.0f, 1.0f);
      } else {
        offBrightness = constrain(offBrightness - step, 0.0f, 1.0f);
      }
      updateXKeyLEDs();
      debugPrintf("[BRIGHTNESS] Encoder 12 → Dir: %s | Off brightness: %.2f | Step: %.2f", 
                 direction > 0 ? "+" : "-", offBrightness, step);

    } else if (index == 12) {
      // Encoder 13 controls onBrightness (populated and on state)
      sensitivityMode = false;  // Allow normal LED updates for brightness adjustment
      float step = baseStep * 0.01f;  // Convert to 0.01 increments (1% steps)
      if (direction > 0) {
        onBrightness = constrain(onBrightness + step, 0.0f, 1.0f);
      } else {
        onBrightness = constrain(onBrightness - step, 0.0f, 1.0f);
      }
      updateXKeyLEDs();
      debugPrintf("[BRIGHTNESS] Encoder 13 → Dir: %s | On brightness: %.2f | Step: %.2f", 
                 direction > 0 ? "+" : "-", onBrightness, step);
    }
    
    // Don't send MIDI in adjustment mode
    return;
  }

  int final_value;

  // Get the appropriate velocity scale for current encoder type
  const int* currentScale;
  if (index < 5) {
    // Relative encoders (0-4): use relative sensitivity
    currentScale = velocityScales[relativeEncoderSensitivity - 1];
  } else {
    // Absolute encoders (5-12): use absolute sensitivity  
    currentScale = velocityScales[absoluteEncoderSensitivity - 1];
  }

  if (index < 5) {
    // First 5 encoders: velocity-based mode for plugin
    int level = constrain((int)(elapsed / 15), 0, 7);
    int scaled = currentScale[7 - level];  // fast = high index = smaller scaled value

    if (direction > 0) {
      final_value = scaled;  // ➕ right = 1–8
    } else {
      final_value = 64 + scaled ; // left 65 and up
    }
  } else {
    // Encoders 5-12: absolute value mode (0-127)
    int level = constrain((int)(elapsed / 15), 0, 7);
    int step = currentScale[7 - level];  // Use sensitivity-specific velocity scaling for step size
    
    if (direction > 0) {
      encoderValues[index] = constrain(encoderValues[index] + step, 0, 127);
    } else {
      encoderValues[index] = constrain(encoderValues[index] - step, 0, 127);
    }
    
    final_value = encoderValues[index];
  }

  usbMIDI.sendControlChange(midi_note[index], final_value, midiCh, 0);
  midiDataPending = true;

  if (index >= 5) {
    debugPrintf("[ENCODER] Index: %d | Dir: %s | CC: %d | Value Sent: %d | Stored: %d | Elapsed: %lu ms", 
               index, direction > 0 ? "+" : "-", midi_note[index], final_value, encoderValues[index], elapsed);
  } else {
    debugPrintf("[ENCODER] Index: %d | Dir: %s | CC: %d | Value Sent: %d | Elapsed: %lu ms", 
               index, direction > 0 ? "+" : "-", midi_note[index], final_value, elapsed);
  }
}