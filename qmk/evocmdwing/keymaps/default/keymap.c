// Copyright 2023 QMK
// SPDX-License-Identifier: GPL-2.0-or-later
 
#include QMK_KEYBOARD_H

#define QWERTY 0 // Base qwerty


const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {

[QWERTY] = LAYOUT(
    // Row 0
    LALT(KC_P),     LALT(KC_N),     LALT(KC_H),     LCTL(KC_O),     LCTL(KC_U),     KC_NO,         KC_NO,         KC_NO,         KC_NO,         KC_NO,         KC_NO,         KC_NO,         KC_NO,         KC_NO,
    // Row 1
    LALT(KC_F),     LALT(KC_U),     LALT(KC_E),     LCTL(KC_M),     LCTL(KC_C),     KC_F1,         KC_F2,         KC_F3,         KC_F4,         KC_NO,         KC_F5,         KC_F6,         KC_F7,         KC_F8,
    // Row 2
    LALT(KC_F),     LALT(KC_D),     LALT(KC_B),     LCTL(KC_D),     LCTL(KC_L),     KC_INSERT,     KC_HOME,       KC_PGUP,       KC_F12,        KC_NO,         KC_DELETE,     KC_END,        KC_PGDN,     KC_KP_PLUS,
    // Row 3
    KC_NO,          KC_NO,          LALT(KC_W),     LCTL(KC_S),     LCTL(KC_H),     KC_NO,         KC_NO,         KC_NO,         KC_NO,         KC_NO,         KC_NO,         KC_NO,         KC_NO,         KC_NO,
    // Row 4
    KC_NO,          KC_NO,          LALT(KC_V),     LCTL(KC_E),     LCTL(KC_Z),     LALT(KC_R),    LALT(KC_K),    LALT(KC_Z),    KC_NO,         LALT(KC_J),    KC_NO,         LALT(KC_L),    LALT(KC_DOT),  LALT(KC_COMMA),
    // Row 5
    KC_NO,          KC_NO,          LALT(KC_LBRC),  LCTL(KC_F),     LCTL(KC_N),     LCTL(KC_G),    KC_NO,         KC_7,          KC_8,          KC_9,          LALT(KC_EQUAL),KC_NO,         LALT(KC_O),    KC_NO,
    // Row 6
    KC_NO,         KC_NO,           LALT(KC_RBRC),  LCTL(KC_P),     LCTL(KC_Q),     LCTL(KC_W),    KC_NO,         KC_4,           KC_5,         KC_6,          LALT(KC_T),    KC_NO,         KC_NO,         KC_NO,       
    // Row 7
    KC_NO,          KC_PAST,        KC_NO,          KC_NO,          KC_NO,          KC_NO,         KC_NO,         KC_1,          KC_2,          KC_3,          KC_MINUS,      KC_NO,         KC_NO,         KC_NO,
    // Row 8
    KC_NO,          KC_RBRC,        LALT(KC_QUOTE), LCTL(KC_X),     LCTL(KC_A),     LCTL(KC_J),    KC_NO,         KC_0,          KC_DOT,  LALT(KC_8),    LALT(KC_2),    KC_NO,         KC_ESC,    KC_NO,
    // Row 9
    KC_NO,          KC_LBRC,        LALT(KC_SCLN),  LCTL(KC_B),     KC_NO,          LALT(KC_S),    KC_NO,         KC_LEFT_SHIFT, LCTL(KC_SLSH), KC_NO,         KC_ENTER,      KC_NO,         LALT(KC_BSPC),LALT(KC_Y)
)

};

