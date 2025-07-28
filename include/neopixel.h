#ifndef NEOPIXEL_H
#define NEOPIXEL_H

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "config.h"

// ================================
// NEOPIXEL LED CONTROL FUNCTIONS
// ================================

void initializeLEDs();

void updateXKeyLEDs();
void setXKeyLED(int xkeyIndex, uint8_t red, uint8_t green, uint8_t blue, float brightness);

// Converts RGB to HSV and scales value, then returns scaled RGB color
uint32_t getScaledColor(uint8_t red, uint8_t green, uint8_t blue, float brightness);

void setLogoPixels(uint8_t red, uint8_t green, uint8_t blue, float brightness);
void clearAllLEDs();
void xkeyFadeSequenceBounce(unsigned long STAGGER_DELAY, unsigned long COLOR_CYCLE_TIME, int cycles, int bounces);
void showStrip();
void updateRelativeSensitivityLEDs();
void updateAbsoluteSensitivityLEDs();
void updateBothSensitivityLEDs();

#endif // NEOPIXEL_H