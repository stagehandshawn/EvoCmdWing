#include "neopixel.h"
#include "utils.h"
#include <algorithm>
#include <cmath>

// ================================
// NEOPIXEL GLOBAL VARIABLES
// ================================

// Default LED mapping - can be manually adjusted if physical layout differs
// Starting after logo pixels (6) (pixels 0-5), then skip 1, then 2 LEDs per key with 1 skip between
XKeyLEDMapping xkeyLEDMap[NUM_XKEYS] = {
  {7, 8},     // XKey 1
  {10, 11},   // XKey 2
  {13, 14},   // XKey 3
  {16, 17},   // XKey 4
  
  {20, 21},   // XKey 5
  {23, 24},   // XKey 6
  {26, 27},   // XKey 7
  {29, 30},   // XKey 8

  {32, 33},   // XKey 9
  {35, 36},   // XKey 10
  {38, 39},   // XKey 11
  {41, 42},   // XKey 12

  {45, 46},   // XKey 13
  {48, 49},   // XKey 14
  {51, 52},   // XKey 15
  {54, 55}    // XKey 16
};

// Note: Brightness controls moved to config struct in eeprom.h

// LED Update Debounce System
unsigned long lastMidiUpdateTime = 0;
bool hasPendingLEDUpdate = false;

// NeoPixel strip object
Adafruit_NeoPixel strip(TOTAL_PIXELS, LED_PIN, NEO_RGB + NEO_KHZ800);

// ================================
// NEOPIXEL LED CONTROL FUNCTIONS
// ================================

// Initialize the NeoPixel LED strip
// Sets up the strip and turns all LEDs off
void initializeLEDs() {
  strip.begin();
  strip.setBrightness(255);
  clearAllLEDs();
  strip.show();
  
  debugPrintf("[LED] Initialized %d pixel strip on pin %d", TOTAL_PIXELS, LED_PIN);
}

// Clear all LEDs on the strip (set to black/off)
void clearAllLEDs() {
  for (int i = 0; i < TOTAL_PIXELS; i++) {
    strip.setPixelColor(i, strip.Color(0, 0, 0));
  }
  strip.show();
  debugPrint("[LED] All LEDs cleared");
}



void setLogoPixels(uint8_t red, uint8_t green, uint8_t blue, float brightness) {
  uint8_t scaledRed = (uint8_t)((red * 2) * brightness);
  uint8_t scaledGreen = (uint8_t)((green * 2) * brightness);
  uint8_t scaledBlue = (uint8_t)((blue * 2) * brightness);
  
  for (int i = 0; i < NUM_LOGO_PIXELS; i++) {
    strip.setPixelColor(i, strip.Color(scaledRed, scaledGreen, scaledBlue));
  }
  
  strip.show();

}

// Converts RGB to HSV and scales value, then returns scaled RGB color
// This preserves hue and saturation while scaling brightness properly
uint32_t getScaledColor(uint8_t red, uint8_t green, uint8_t blue, float brightness) {
  // Scale MIDI values (0-127) to full RGB range (0-255) first
  red = red * 2;
  green = green * 2;
  blue = blue * 2;
  
  // Special case: if original color is black (0,0,0), keep it black
  if (red == 0 && green == 0 && blue == 0) {
    return strip.Color(0, 0, 0);  // Always return black regardless of brightness
  }

  float r = red / 255.0f;
  float g = green / 255.0f;
  float b = blue / 255.0f;

  float cmax = std::max(r, std::max(g, b));
  float cmin = std::min(r, std::min(g, b));
  float delta = cmax - cmin;

  float h = 0, s = 0;

  // Calculate hue
  if (delta != 0) {
    if (cmax == r) h = fmodf(((g - b) / delta), 6.0f);
    else if (cmax == g) h = ((b - r) / delta) + 2.0f;
    else h = ((r - g) / delta) + 4.0f;

    h *= 60.0f;
    if (h < 0) h += 360.0f;
  }

  // Calculate saturation
  if (cmax != 0) s = delta / cmax;

  // Use brightness parameter as the 'value' in HSV
  float scaledV = brightness;

  // Convert HSV back to RGB with scaled brightness
  float c = scaledV * s;
  float x = c * (1 - fabsf(fmodf(h / 60.0f, 2) - 1));
  float m = scaledV - c;

  float r1 = 0, g1 = 0, b1 = 0;

  if (h < 60)      { r1 = c; g1 = x; }
  else if (h < 120){ r1 = x; g1 = c; }
  else if (h < 180){ g1 = c; b1 = x; }
  else if (h < 240){ g1 = x; b1 = c; }
  else if (h < 300){ r1 = x; b1 = c; }
  else             { r1 = c; b1 = x; }

  return strip.Color(
    (uint8_t)((r1 + m) * 255),
    (uint8_t)((g1 + m) * 255),
    (uint8_t)((b1 + m) * 255)
  );
}

// Set the color and brightness for a specific X-key's LEDs
// xkeyIndex: 0-15 for XKey 1-16
// red, green, blue: 0-127 (MIDI range) - will be scaled to 0-255
// brightness: 0.0-1.0 multiplier for the color intensity using HSV scaling
void setXKeyLED(int xkeyIndex, uint8_t red, uint8_t green, uint8_t blue, float brightness) {

  if (xkeyIndex < 0 || xkeyIndex >= NUM_XKEYS) {
    debugPrintf("[LED] Invalid XKey index: %d", xkeyIndex);
    return;
  }
  
  // Use HSV scaling
  uint32_t scaledColor = getScaledColor(red, green, blue, brightness);
  
  // Get the pixels for this X-key
  int firstPixel = xkeyLEDMap[xkeyIndex].firstPixelIndex;
  int secondPixel = xkeyLEDMap[xkeyIndex].secondPixelIndex;
  
  // Set both LEDs for this X-key to the same color
  strip.setPixelColor(firstPixel, scaledColor);
  strip.setPixelColor(secondPixel, scaledColor);
  
}

void updateXKeyLEDs() {
  static unsigned long lastUpdate = 0;
  unsigned long now = millis();
  
  // Don't update XKey LEDs when in sensitivity mode (allow brightness adjustments to show)
  if (sensitivityMode) {
    return;
  }
  
  // Wait enough time to get all 3 values rgb, for cleaner color changes
  if (now - lastUpdate < 50) {
    return;
  }
  lastUpdate = now;
  
  bool ledChanged = false;
  
  for (int i = 0; i < NUM_XKEYS; i++) {
    ExecutorStatus* status = &pageData[currentPage][i];
    
    if (!status->isPopulated) {
      // Key not populated - turn LEDs off
      setXKeyLED(i, 0, 0, 0, 0.0);
      ledChanged = true;
    } else if (status->isPopulated && !status->isOn) {
      // Key populated but not on - use offBrightness
      setXKeyLED(i, status->red, status->green, status->blue, config.offBrightness);
      ledChanged = true;
    } else if (status->isPopulated && status->isOn) {
      // Key populated and on - use onBrightness
      setXKeyLED(i, status->red, status->green, status->blue, config.onBrightness);
      ledChanged = true;
    }
  }
  
  if (ledChanged) {
    strip.show();
  }
}

void showStrip() {
  strip.show();
}


// XKey startup fade sequence with rainbow wave effect AND bouncing
void xkeyFadeSequenceBounce(unsigned long STAGGER_DELAY, unsigned long COLOR_CYCLE_TIME, int cycles, int bounces) {
  
  const int NUM_GROUPS = 8;  // 8 groups of paired XKeys
  unsigned long startTime = millis();
  bool animationComplete = false;
  
  // Calculate total animation time for all cycles and bounces
  unsigned long totalCycleTime = COLOR_CYCLE_TIME + 250;
  unsigned long singleWaveTime = (NUM_GROUPS - 1) * STAGGER_DELAY + totalCycleTime;
  

  unsigned long wavesPerCycle = 1 + (bounces * 2); // Initial L→R + bounce pairs
  unsigned long timePerCycle = wavesPerCycle * singleWaveTime;
  unsigned long fullSequenceTime = timePerCycle * cycles;
  
  // Store original XKey status colors and brightness
  struct OriginalXKeyState {
    uint8_t red, green, blue;
    float brightness;
  } originalStates[NUM_XKEYS];
  
  // Save current XKey states and start all at black
  for (int i = 0; i < NUM_XKEYS; i++) {
    ExecutorStatus* status = &xkeyStatus[i];
    originalStates[i].red = status->red;
    originalStates[i].green = status->green;
    originalStates[i].blue = status->blue;
    
    // Determine original brightness based on status
    if (!status->isPopulated) {
      originalStates[i].brightness = 0.0;
    } else if (status->isPopulated && !status->isOn) {
      originalStates[i].brightness = config.offBrightness;
    } else {
      originalStates[i].brightness = config.onBrightness;
    }
    
    // Start with black LEDs
    setXKeyLED(i, 0, 0, 0, 0.0);
  }
  strip.show();
  
  while (!animationComplete) {
    unsigned long now = millis();
    unsigned long totalElapsed = now - startTime;
    
    // Check if we've completed all cycles and bounces
    if (totalElapsed >= fullSequenceTime) {
      animationComplete = true;
      break;
    }
    
    animationComplete = true; // Will be set to false if any group is still animating
    
    // Determine which cycle and wave within cycle we're in
    unsigned long currentCycle = totalElapsed / timePerCycle;
    unsigned long cycleElapsed = totalElapsed % timePerCycle;
    unsigned long currentWaveInCycle = cycleElapsed / singleWaveTime;
    
    bool isRightToLeft = (currentWaveInCycle % 2 == 1);
    
    // Process each group (0-7 representing XKey pairs 1&9, 2&10, etc.)
    for (int group = 0; group < NUM_GROUPS; group++) {
      // Calculate effective group index based on direction
      int effectiveGroup = isRightToLeft ? (NUM_GROUPS - 1 - group) : group;
      
      // Calculate when this group should start within the current wave
      unsigned long groupStartOffset = effectiveGroup * STAGGER_DELAY;
      unsigned long groupStartTime = startTime + (currentCycle * timePerCycle) + 
                                   (currentWaveInCycle * singleWaveTime) + groupStartOffset;
      
      if (now >= groupStartTime) {
        unsigned long elapsed = now - groupStartTime;
        
        // Calculate which XKeys belong to this group
        int xkey1 = group;         // XKeys 0-7 (representing 1-8)
        int xkey2 = group + 8;     // XKeys 8-15 (representing 9-16)
        
        if (elapsed < COLOR_CYCLE_TIME) {
          // Rainbow color wave is active on this group
          animationComplete = false;
          
          // Calculate color transition through rainbow spectrum
          float colorProgress = (elapsed % COLOR_CYCLE_TIME) / (float)COLOR_CYCLE_TIME;
          float hue = colorProgress * 360.0; // Full rainbow cycle
          
          // Convert HSV to RGB (Hue, Saturation=1, Value=1)
          float c = 1.0; // Saturation at max
          float x = c * (1.0 - fabs(fmod(hue / 60.0, 2.0) - 1.0));
          
          float r1, g1, b1;
          if (hue < 60)      { r1 = c; g1 = x; b1 = 0; }
          else if (hue < 120){ r1 = x; g1 = c; b1 = 0; }
          else if (hue < 180){ r1 = 0; g1 = c; b1 = x; }
          else if (hue < 240){ r1 = 0; g1 = x; b1 = c; }
          else if (hue < 300){ r1 = x; g1 = 0; b1 = c; }
          else               { r1 = c; g1 = 0; b1 = x; }
          
          // Convert to MIDI range (0-127)
          uint8_t rainbowRed = (uint8_t)(r1 * 127);
          uint8_t rainbowGreen = (uint8_t)(g1 * 127);
          uint8_t rainbowBlue = (uint8_t)(b1 * 127);
          
          // Calculate brightness with breathing effect
          float breatheProgress = (elapsed % COLOR_CYCLE_TIME) / (float)COLOR_CYCLE_TIME;
          float breatheValue = (sin(breatheProgress * PI * 2) + 1.0) / 2.0; // Sine wave 0-1
          
          // Combine with fade-in effect
          float fadeProgress = min(1.0f, elapsed / (float)(COLOR_CYCLE_TIME * 0.3)); // Fade in over first 30%
          
          float finalBrightness = breatheValue * fadeProgress;
          
          // Set both XKeys in this group to the same color
          setXKeyLED(xkey1, rainbowRed, rainbowGreen, rainbowBlue, finalBrightness);
          setXKeyLED(xkey2, rainbowRed, rainbowGreen, rainbowBlue, finalBrightness);
          
        } else {
          // Wave has passed, fade out with color shift to original colors
          unsigned long fadeOutTime = elapsed - COLOR_CYCLE_TIME;
          const unsigned long FADE_OUT_DURATION = 250; // 250ms fade out
          
          if (fadeOutTime < FADE_OUT_DURATION) {
            animationComplete = false;
            
            // Fade to original colors while dimming
            float fadeProgress = fadeOutTime / (float)FADE_OUT_DURATION;
            
            // Process both XKeys in the group
            for (int xkeyOffset = 0; xkeyOffset < 2; xkeyOffset++) {
              int xkeyIndex = (xkeyOffset == 0) ? xkey1 : xkey2;
              
              // Blend to original color
              uint8_t targetRed = (uint8_t)((originalStates[xkeyIndex].red * 2) * fadeProgress);
              uint8_t targetGreen = (uint8_t)((originalStates[xkeyIndex].green * 2) * fadeProgress);
              uint8_t targetBlue = (uint8_t)((originalStates[xkeyIndex].blue * 2) * fadeProgress);
              
              // Fade brightness
              float brightnessFade = (1.0 - fadeProgress) * 0.5 + originalStates[xkeyIndex].brightness * fadeProgress;
              
              setXKeyLED(xkeyIndex, targetRed / 2, targetGreen / 2, targetBlue / 2, brightnessFade);
            }
          } else {
            // Completely finished - restore original states
            setXKeyLED(xkey1, originalStates[xkey1].red, originalStates[xkey1].green, 
                      originalStates[xkey1].blue, originalStates[xkey1].brightness);
            setXKeyLED(xkey2, originalStates[xkey2].red, originalStates[xkey2].green, 
                      originalStates[xkey2].blue, originalStates[xkey2].brightness);
          }
        }
      } else {
        // Group hasn't started yet
        animationComplete = false;
        int xkey1 = group;
        int xkey2 = group + 8;
        setXKeyLED(xkey1, 0, 0, 0, 0.0);
        setXKeyLED(xkey2, 0, 0, 0, 0.0);
      }
    }
    
    // Update the LED strip
    strip.show();
    delay(10);
  }
  
  // Final cleanup - ensure all XKeys are properly restored
  for (int i = 0; i < NUM_XKEYS; i++) {
    setXKeyLED(i, originalStates[i].red, originalStates[i].green, 
              originalStates[i].blue, originalStates[i].brightness);
  }
  strip.show();

}

// ================================
// SENSITIVITY LED FEEDBACK FUNCTIONS
// ================================


void updateSensitivityLEDs() {
  // Show relative sensitivity on XKeys 1-8 (green)
  for (int i = 0; i < 8; i++) {
    if (i < config.relativeEncoderSensitivity) {
      setXKeyLED(i, 0, 127, 0, config.onBrightness);      // Green for active levels
    } else {
      setXKeyLED(i, 0, 0, 0, 0.0);                 // Off for inactive levels
    }
  }
  
  // Show absolute sensitivity on XKeys 9-16 (blue)
  for (int i = 8; i < 16; i++) {
    if ((i - 8) < config.absoluteEncoderSensitivity) {
      setXKeyLED(i, 0, 0, 127, config.onBrightness);      // Blue for active levels
    } else {
      setXKeyLED(i, 0, 0, 0, 0.0);                 // Off for inactive levels
    }
  }
  
  showStrip();
}
