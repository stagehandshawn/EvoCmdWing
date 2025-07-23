#ifndef ENCODERS_H
#define ENCODERS_H

#include <Arduino.h>
#include <Encoder.h>
#include "config.h"

// ================================
// ENCODER AND BUTTON FUNCTIONS
// ================================

void initializeEncoders();
void handleEncoders();
void handleButtons();
void sendMidiEncoder(int index, int direction);

#endif // ENCODERS_H