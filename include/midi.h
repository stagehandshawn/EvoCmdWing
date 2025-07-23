#ifndef MIDI_H
#define MIDI_H

#include <Arduino.h>
#include "config.h"

// ================================
// MIDI COMMUNICATION FUNCTIONS
// ================================

void handleIncomingMIDI();
void handleStatusMIDI(byte ch, byte cc, byte value);
void handlePageMIDI(byte ch, byte cc, byte value);

#endif // MIDI_H