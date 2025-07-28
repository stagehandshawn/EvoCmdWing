#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <Encoder.h>

typedef unsigned char byte;

// ================================
// PROJECT INFORMATION
// ================================
const char* const PROJECT_VERSION = "0.2";
const char* const PROJECT_NAME = "EvoCmdWing";

// ================================
// PIN DEFINITIONS
// ================================
const int LED_PIN = 10;              // NeoPixel data pin
const int LATCH_LED_PIN = 13;        // GPIO pin for latching inner outter flip button

// ================================
// ENCODER/BUTTON CONFIGURATION
// ================================
const int N_ENCODERS = 13;
const int N_BUTTONS = 14;

// GPIO pins for encoder a/b
extern const int enc_pins[N_ENCODERS][2];

// GPIO pin for encoder click, and inner outter button
extern const int BUTTON_PIN[N_BUTTONS];

// ================================
// MIDI CONFIGURATION
// ================================
// first 5 are for attribute encoders, next 8 are for XKeys encoders
extern const byte midi_note[N_ENCODERS];

// first 5 for encoders, next 8 for XKeys, last one for inner outter flip
extern const byte buttonNotes[N_BUTTONS];

// Here you can play with the velocity scaling to get the sensitivity you like
// This works well for me with no accel in Pro Plugins Midi Encoders plugin
extern const int velocityScales[8][8];

// Encoder sensitivity variables
extern int relativeEncoderSensitivity;  // For encoders 0-4 (relative mode)
extern int absoluteEncoderSensitivity;   // For encoders 5-12 (absolute mode)

//midi channel to send on
extern byte midiCh;

// ================================
// LED CONFIGURATION
// ================================
const int NUM_LOGO_PIXELS = 6;       // Number of pixels in logo section
const int NUM_XKEYS = 16;            // Number of X-keys
const int LEDS_PER_XKEY = 2;         // LEDs per X-key
const int NUM_OFF_LEDS = 18;         // Number of pixels that need to be skiped (both strips need to skip the first and 1 between each key and 2 between the larger gap)
const int TOTAL_PIXELS = (NUM_LOGO_PIXELS + (NUM_XKEYS * LEDS_PER_XKEY) + NUM_OFF_LEDS); // Logo + XKey LEDs + spacer pixels

// LED mapping structure for X-key to pixel index conversion
// Each X-key has a start index for its first LED
// Layout: [6 logo pixels][skip][2 xkey1 LEDs][skip][2 xkey2 LEDs][skip][2 xkey3 LEDs][skip][2 xkey4 LEDs][skip * 2]....
struct XKeyLEDMapping {
  int firstPixelIndex;              // Index of first LED for this X-key
  int secondPixelIndex;             // Index of second LED for this X-key
};

// Default LED mapping - can be manually adjusted if physical layout differs
extern XKeyLEDMapping xkeyLEDMap[NUM_XKEYS];

// Independent brightness controls (0.0 to 1.0)
extern float onBrightness;        // Brightness for populated and on XKeys
extern float offBrightness;       // Brightness for populated but off XKeys
extern float logoBrightness;

// ================================
// LED UPDATE DEBOUNCE SYSTEM
// ================================
// This prevents LED flashing when multiple MIDI messages arrive in quick succession
extern unsigned long lastMidiUpdateTime;
const unsigned long MIDI_DEBOUNCE_MS = 50; // Wait 50ms after last MIDI message before updating LEDs
extern bool hasPendingLEDUpdate;


// ================================
// PAGE-BASED EXECUTOR STATUS DATA STRUCTURE
// ================================
// Structure to hold executor status information from grandMA3
// Receives data on MIDI channel 2 using optimized CC mapping:
// - Status: XKeys 1-16 use CC 1-16 (combined populated/on state)
// - RGB: XKeys 1-16 use CC 17-64 (3 CCs each, only when populated)
// Page changes received on MIDI channel 3, CC 1 (page number 1-127)
struct ExecutorStatus {
  bool isOn;           // On/off state of executor
  bool isPopulated;    // Whether executor has sequence assigned
  uint8_t red;         // Red  (0-127 MIDI range)
  uint8_t green;       // Green (0-127 MIDI range)
  uint8_t blue;        // Blue (0-127 MIDI range)
};

// Page-based storage for up to 127 pages, 16 XKeys each
// pageData[page][xkey] where page = 0-126 (for pages 1-127), xkey = 0-15 (for XKeys 1-16)
extern ExecutorStatus pageData[127][16];

// Current page tracking (1-127, but stored as 0-126 index)
extern int currentPage;

// Current active status (points to current page data for convenience)
extern ExecutorStatus* xkeyStatus;

// ================================
// ENCODER MANAGEMENT
// ================================
// Array to store current values for encoders 5-12 (absolute mode)
extern int encoderValues[N_ENCODERS];

extern Encoder* encoders[N_ENCODERS];

extern long lastPos[N_ENCODERS];
extern unsigned long lastMoveTime[N_ENCODERS];
extern int buttonPState[N_BUTTONS];
extern bool latchButtonState;
extern unsigned long lastDebounceTime[N_BUTTONS];
extern unsigned long debounceDelay;

extern int encoderBuffer[N_ENCODERS];
extern bool midiDataPending;

// ================================
// ADJUSTMENT MODE (BRIGHTNESS & SENSITIVITY)
// ================================
extern bool adjustMode;               // True when button 14 is held down for brightness/sensitivity adjustment
extern bool sensitivityMode;          // True when actively adjusting sensitivity (blocks normal LED updates)
extern unsigned long button14HoldTime; // Time when button 14 was pressed

// ================================
// MIDI CONSTANTS
// ================================
const int MAX_MESSAGES_PER_LOOP = 32; // Process up to 32 messages per loop

#endif // CONFIG_H