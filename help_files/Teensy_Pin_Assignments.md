# Teensy 4.1 Pin Assignments - EvoCmdWing

## Overview
The EvoCmdWing uses a Teensy 4.1 mcu to manage 13 encoders with buttons, Inner/outter toggle, NeoPixel LEDs, and MIDI communication for the grandMA3.

## Encoder Pin Assignments

### Attribute Encoders (1-5)

| Encoder | Pin A | Pin B | Button Pin | MIDI Note | Function |
|---------|-------|-------|------------|-----------|----------|
| 1       | 0     | 1     | 28         | 1         | Attribute Control |
| 2       | 2     | 3     | 29         | 2         | Attribute Control |
| 3       | 4     | 5     | 30         | 3         | Attribute Control |
| 4       | 6     | 7     | 31         | 4         | Attribute Control |
| 5       | 8     | 9     | 32         | 5         | Attribute Control |

### XKey Encoders (6-13) 
*Mapped to grandMA3 XKeys 1-8 (Executors 291-298)*

| Encoder | Pin A | Pin B | Button Pin | MIDI Note | XKey # | Executor |
|---------|-------|-------|------------|-----------|--------|----------|
| 6       | 11    | 12    | 33         | 6         | 1      | 291      |
| 7       | 14    | 15    | 34         | 7         | 2      | 292      |
| 8       | 16    | 17    | 35         | 8         | 3      | 293      |
| 9       | 18    | 19    | 36         | 9         | 4      | 294      |
| 10      | 20    | 21    | 37         | 10        | 5      | 295      |
| 11      | 22    | 23    | 38         | 11        | 6      | 296      |
| 12      | 24    | 25    | 39         | 12        | 7      | 297      |
| 13      | 26    | 27    | 40         | 13        | 8      | 298      |

## Attribute Encoder Inner/Outter Toggle

| Pin | Function | MIDI Note | Description |
|-----|----------|-----------|-------------|
| 41  | Inner/Outer Toggle | 14 | Switches encoder functions between inner/outer ring |
| 13  | Latch LED || Status LED for Inner/Outer encoder mode |

**LED Indicator:**
- Pin 13 (Latch LED) shows Inner/Outer mode status
- LED ON = Outer mode active
- LED OFF = Inner mode active

## NeoPixel LED Configuration

| Pin | Function | Description |
|-----|----------|-------------|
| 10  | NeoPixel Data | WS2812B LED strip data signal (58 total pixels) |

### Total LED Count: 58 Pixels

| LED Range | Function | Count | Description |
|-----------|----------|-------|-------------|
| 0-5       | Logo     | 6     | Project logo illumination |
| 6-57      | XKey LEDs| 52    | Status indicators for XKeys |

### XKey LED Mapping
*Each XKey has 2 status LEDs*

| XKey | First LED | Second LED | Executor Range |
|------|-----------|------------|----------------|
| 1    | 7         | 8          | 291 |
| 2    | 10        | 11         | 292 |
| 3    | 13        | 14         | 293 |
| 4    | 16        | 17         | 294 |
| 5    | 20        | 21         | 295 |
| 6    | 23        | 24         | 296 |
| 7    | 26        | 27         | 297 |
| 8    | 29        | 30         | 298 |
| 9    | 32        | 33         | 191 |
| 10   | 35        | 36         | 192 |
| 11   | 38        | 39         | 193 |
| 12   | 41        | 42         | 194 |
| 13   | 45        | 46         | 195 |
| 14   | 48        | 49         | 196 |
| 15   | 51        | 52         | 197 |
| 16   | 54        | 55         | 198 |

**Note:** Pixels 9, 12, 15, 18, 19, 22, 25, 28, 31, 34, 37, 40, 43, 44, 47, 50, 53, 56 are used as spacers/gaps between XKey groups.

## MIDI Configuration

| Channel | Direction | Purpose | CC/Note Range |
|---------|-----------|---------|---------------|
| 1       | Output    | Encoder values and button presses | CC 6-13, Notes 1-14 |
| 2       | Input     | XKey status and RGB data | CC 1-64 |
| 3       | Input     | Page change notifications | CC 1 |

### MIDI Mapping Details

**Outgoing (Channel 1):**
- Encoders 6-13: Send CC 6-13 (fader values)
- All buttons: Send Note On/Off 1-14

**Incoming (Channel 2):**
- XKey Status: CC 1-16 (0=empty, 65=off, 127=on)
- XKey RGB: CC 17-64 (3 CCs per XKey for R,G,B values)

**Incoming (Channel 3):**
- Page Changes: CC 1 (page number 1-127)

## Special Modes

### Brightness Adjustment Mode
**Activation:** Hold button 14 (pin 41)

| Encoder | Function | Range |
|---------|----------|-------|
| 12      | Off-state brightness | 0-255 |
| 13      | On-state brightness | 0-255 |
