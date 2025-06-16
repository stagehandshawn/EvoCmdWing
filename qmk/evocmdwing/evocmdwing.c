/* Copyright 2020 QMK
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "quantum.h"

#define LED_POWER LINE_PIN13

void matrix_init_kb(void) {
    matrix_init_user();

    // Turn on the Teensy 4.x Power LED:
    setPinOutput(LED_POWER);
    writePinHigh(LED_POWER);
}

// delay_inline sleeps for |cycles| (e.g. sleeping for F_CPU will sleep 1s).
// delay_inline assumes the cycle counter has already been initialized and
// should not be modified, i.e. it is safe to call during keyboard matrix scan.
//
// ChibiOS enables the cycle counter in chcore_v7m.c:
// https://github.com/ChibiOS/ChibiOS/blob/b63023915c304092acb9f33bbab40f3ec07a7f0e/os/common/ports/ARMCMx/chcore_v7m.c#L263
static void delay_inline(const uint32_t cycles) {
    const uint32_t start = DWT->CYCCNT;
    while ((DWT->CYCCNT - start) < cycles) {
        // busy-loop until time has passed
    }
}

void matrix_output_unselect_delay(uint8_t line, bool key_pressed) {
    // Use the cycle counter to do precise timing in microseconds. The ChibiOS
    // thread sleep functions only allow sleep durations starting at 1 tick, which
    // is 100μs in our configuration.

    // Empirically: e.g. 5μs is not enough, will result in keys that don’t work
    // and ghost key presses. 10μs seems to work well.

    // On some variants of the hardware, 20μs seems to be required. This was found
    // on a combination of KB600LF+stapelberg v2020-06-30+teensy41.

    // 600 cycles at 0.6 cycles/ns == 1μs
    const uint32_t cycles_per_us = 600;
    delay_inline(20 * cycles_per_us);
}



#include "grandma3.h"

#define LAYOUT( \
  K000, K001, K002, K003, K004, K005, K006, K007, K008, K009, K010, K011, K012, K013, \
  K100, K101, K102, K103, K104, K105, K106, K107, K108, K109, K110, K111, K112, K113, \
  K200, K201, K202, K203, K204, K205, K206, K207, K208, K209, K210, K211, K212, K213, \
  K300, K301, K302, K303, K304, K305, K306, K307, K308, K309, K310, K311, K312, K313, \
  K400, K401, K402, K403, K404, K405, K406, K407, K408, K409, K410, K411, K412, K413, \
  K500, K501, K502, K503, K504, K505, K506, K507, K508, K509, K510, K511, K512, K513, \
  K600, K601, K602, K603, K604, K605, K606, K607, K608, K609, K610, K611, K612, K613, \
  K700, K701, K702, K703, K704, K705, K706, K707, K708, K709, K710, K711, K712, K713, \
  K800, K801, K802, K803, K804, K805, K806, K807, K808, K809, K810, K811, K812, K813, \
  K900, K901, K902, K903, K904, K905, K906, K907, K908, K909, K910, K911, K912, K913  \
) \
{ \
  { K000, K001, K002, K003, K004, K005, K006, K007, K008, K009, K010, K011, K012, K013 }, \
  { K100, K101, K102, K103, K104, K105, K106, K107, K108, K109, K110, K111, K112, K113 }, \
  { K200, K201, K202, K203, K204, K205, K206, K207, K208, K209, K210, K211, K212, K213 }, \
  { K300, K301, K302, K303, K304, K305, K306, K307, K308, K309, K310, K311, K312, K313 }, \
  { K400, K401, K402, K403, K404, K405, K406, K407, K408, K409, K410, K411, K412, K413 }, \
  { K500, K501, K502, K503, K504, K505, K506, K507, K508, K509, K510, K511, K512, K513 }, \
  { K600, K601, K602, K603, K604, K605, K606, K607, K608, K609, K610, K611, K612, K613 }, \
  { K700, K701, K702, K703, K704, K705, K706, K707, K708, K709, K710, K711, K712, K713 }, \
  { K800, K801, K802, K803, K804, K805, K806, K807, K808, K809, K810, K811, K812, K813 }, \
  { K900, K901, K902, K903, K904, K905, K906, K907, K908, K909, K910, K911, K912, K913 }  \
}

