// Written by ShawnR
// Built for Teensy 4.1
// Sends MIDI for Grandma3 Encoders on first 5 CCs of channel 1 using relative velocity for use with Pro Plugins Midi Encoders plugin
// Sends absolute CC values for next 8 encoders for Xkeys 1-8, and recieves MIDI back on same CCs for storing current fader value

// Has one latching button with led status for Flipping from inner to outter encoder function using Pro Plugins Midi Encoders plugin
// Hold the button down and rotate encoder 13 to set brightness for XKeys ON state, rotate encoder 12 for XKeys OFF state brightness
// Use Midi ch1 note 14 for inner outter flip

// Sends notes on encoder click ch1, notes: 1-13
// Encoder click is handled with Midi Remotes created by EvoCmdWingMidi_Main.lua plugin

#include <Arduino.h>
#include <MIDIUSB.h>
#include "config.h"
#include "neopixel.h"
#include "encoders.h"
#include "midi.h"
#include "utils.h"

void setup() {
  Serial.begin(115200);
  if (debugMode) delay(200);

  debugPrint("EvoCmdWing setup");

  initializeEncoders();
  initializeLEDs();
  
  xkeyFadeSequenceBounce(50, 1000, 2, 0);  

  setLogoPixels(127, 64, 0, logoBrightness); // orange

  debugPrint("Setup complete");
}

void loop() {

  handleIncomingMIDI();
  handleEncoders();
  handleButtons();
  
  if (midiDataPending) {
    usbMIDI.send_now();
    midiDataPending = false;
  }

  // Handle midi often to keep teensy buffer from overflow
  handleIncomingMIDI();
  
  // LED Update all colors at once 
  if (hasPendingLEDUpdate && (millis() - lastMidiUpdateTime) >= MIDI_DEBOUNCE_MS) {
    showStrip();
    hasPendingLEDUpdate = false;
  }
  
  updateXKeyLEDs();
  
  checkSerialForReboot();

}