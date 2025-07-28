#ifndef EEPROM_H
#define EEPROM_H

#include <Arduino.h>

// ================================
// EEPROM CONFIGURATION SYSTEM
// ================================

#define CONFIG_SIGNATURE 0xE7042024

// Configuration
struct ConfigData {
  uint32_t signature;                    // Magic number to validate data
  uint8_t version;                       // Version for future compatibility
  
  // Encoder sensitivity settings
  int relativeEncoderSensitivity;        // 1-8, default 5
  int absoluteEncoderSensitivity;        // 1-8, default 5
  
  // Brightness settings (0.0 to 1.0)
  float onBrightness;                    // Default 1.0
  float offBrightness;                   // Default 0.05
  float logoBrightness;                  // Default 1.0
  
};

// Default configuration values
extern const ConfigData defaultConfig;

// Global config instance
extern ConfigData config;

// ================================
// EEPROM FUNCTIONS
// ================================

void initializeEEPROM();
bool loadConfig();
void saveConfig();
void resetConfigToDefaults();
void printConfig();

#endif // EEPROM_H