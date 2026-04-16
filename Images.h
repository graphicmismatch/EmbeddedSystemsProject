#pragma once

#include "Bitmap.h"

// Replace these placeholders with the output from img2bitmap.py while keeping
// the same symbol names. The game logic expects the sprite list documented in
// SPRITES.md.

const uint8_t pet_placeholder_data[] PROGMEM = {
    0x00, 0x00, 0x07, 0xE0, 0x0F, 0xF0, 0x1F, 0xF8,
    0x1F, 0xF8, 0x38, 0x1C, 0x3D, 0xBC, 0x3E, 0x7C,
    0x3F, 0xFC, 0x1F, 0xF8, 0x1F, 0xF8, 0x0F, 0xF0,
    0x0F, 0xF0, 0x0C, 0x30, 0x18, 0x18, 0x00, 0x00,
};

const uint8_t pet_dead_data[] PROGMEM = {
    0x00, 0x00, 0x07, 0xE0, 0x0F, 0xF0, 0x19, 0x98,
    0x15, 0xA8, 0x32, 0x4C, 0x3C, 0x3C, 0x38, 0x1C,
    0x3C, 0x3C, 0x1D, 0xB8, 0x1F, 0xF8, 0x0F, 0xF0,
    0x0F, 0xF0, 0x0C, 0x30, 0x18, 0x18, 0x00, 0x00,
};

const uint8_t pet_leave_data[] PROGMEM = {
    0x00, 0x00, 0x01, 0xC0, 0x03, 0xE0, 0x07, 0xF0,
    0x0F, 0xF8, 0x1F, 0xFC, 0x1D, 0xBC, 0x39, 0x9C,
    0x38, 0x1C, 0x30, 0x0C, 0x20, 0x04, 0x60, 0x06,
    0x40, 0x02, 0xC0, 0x03, 0x80, 0x01, 0x00, 0x00,
};

const Bitmap pet_healthy_sleep_0 = {pet_placeholder_data, 16, 16};
const Bitmap pet_healthy_sleep_1 = {pet_placeholder_data, 16, 16};
const Bitmap pet_healthy_awake_0 = {pet_placeholder_data, 16, 16};
const Bitmap pet_healthy_awake_1 = {pet_placeholder_data, 16, 16};
const Bitmap pet_healthy_happy_0 = {pet_placeholder_data, 16, 16};
const Bitmap pet_healthy_happy_1 = {pet_placeholder_data, 16, 16};
const Bitmap pet_healthy_surprised_0 = {pet_placeholder_data, 16, 16};
const Bitmap pet_healthy_surprised_1 = {pet_placeholder_data, 16, 16};

const Bitmap pet_mid_sleep_0 = {pet_placeholder_data, 16, 16};
const Bitmap pet_mid_sleep_1 = {pet_placeholder_data, 16, 16};
const Bitmap pet_mid_awake_0 = {pet_placeholder_data, 16, 16};
const Bitmap pet_mid_awake_1 = {pet_placeholder_data, 16, 16};
const Bitmap pet_mid_happy_0 = {pet_placeholder_data, 16, 16};
const Bitmap pet_mid_happy_1 = {pet_placeholder_data, 16, 16};
const Bitmap pet_mid_surprised_0 = {pet_placeholder_data, 16, 16};
const Bitmap pet_mid_surprised_1 = {pet_placeholder_data, 16, 16};

const Bitmap pet_low_sleep_0 = {pet_placeholder_data, 16, 16};
const Bitmap pet_low_sleep_1 = {pet_placeholder_data, 16, 16};
const Bitmap pet_low_awake_0 = {pet_placeholder_data, 16, 16};
const Bitmap pet_low_awake_1 = {pet_placeholder_data, 16, 16};
const Bitmap pet_low_happy_0 = {pet_placeholder_data, 16, 16};
const Bitmap pet_low_happy_1 = {pet_placeholder_data, 16, 16};
const Bitmap pet_low_surprised_0 = {pet_placeholder_data, 16, 16};
const Bitmap pet_low_surprised_1 = {pet_placeholder_data, 16, 16};

const Bitmap pet_dead = {pet_dead_data, 16, 16};
const Bitmap pet_leave = {pet_leave_data, 16, 16};
