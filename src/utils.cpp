#include "utils.h"
#include "config.h"

//================================
// DEBUG SETTINGS
//================================
#ifdef DEBUG
  bool debugMode = true;  // Enables or disables verbose serial output
#else
  bool debugMode = false;  // disables or disables verbose serial output
#endif


//================================
// DEBUG FUNCTIONS
//================================


void debugPrint(const char* message) {
  if (debugMode) {
    Serial.println(message);
  }
}

void debugPrintf(const char* format, ...) {
  if (debugMode) {
    char buffer[128];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    // Check if the format string already ends with a newline
    size_t len = strlen(format);
    if (len > 0 && format[len-1] == '\n') {
      Serial.print(buffer); // Already has newline
    } else {
      Serial.println(buffer); // Add newline
    }
  }
}


//================================
// UPLOAD Function 
//================================
//Upload without pressing button, using python script, takes one second try
void checkSerialForReboot() {
    static String commandBuffer = "";
    
    // Read all available characters and buffer them
    while (Serial.available()) {
        char c = Serial.read();
        if (c == '\n' || c == '\r') {
            // End of command received, process it
            commandBuffer.trim(); // Remove any whitespace
            String cmd = commandBuffer;
            commandBuffer = ""; // Clear buffer for next command
            
            // Process the complete command
            if (cmd.length() > 0) {
                processSerialCommand(cmd);
            }
            return;
        } else {
            // Add character to buffer
            commandBuffer += c;
        }
    }
}

// Send identiy so we can update a specific teensy when more than one is plugged in, used with teensy_auto_upload_multi.py
void processSerialCommand(String cmd) {
    if (cmd == "IDENTIFY") {
        Serial.print("[IDENT] ");
        Serial.print(PROJECT_NAME);
        Serial.print(" v");
        Serial.println(PROJECT_VERSION);
        Serial.flush();
        
    } else if (cmd == "REBOOT_BOOTLOADER") {
        Serial.print("[REBOOT] ");
        Serial.print(PROJECT_NAME);
        Serial.print(" v");
        Serial.print(PROJECT_VERSION);
        Serial.println(" entering bootloader...");
        Serial.flush(); // Important: ensure message is sent before reboot
        delay(100);
        
        // This is the correct method for ALL Teensy models
        _reboot_Teensyduino_();
        
    } else if (cmd == "REBOOT_NORMAL") {
        Serial.print("[REBOOT] ");
        Serial.print(PROJECT_NAME);
        Serial.print(" v");
        Serial.print(PROJECT_VERSION);
        Serial.println(" normal reboot requested...");
        Serial.flush();
        delay(100);
        
        // Normal restart using ARM AIRCR register
        SCB_AIRCR = 0x05FA0004;
        
    } else {
        Serial.print("[REBOOT] Unknown command: ");
        Serial.println(cmd);
    }
}