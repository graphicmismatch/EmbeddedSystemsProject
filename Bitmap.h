#pragma once
#ifndef _BITMAP_H_
#define _BITMAP_H_

#include <Arduino.h>
#include <cstdint>

typedef struct {
  const uint8_t *data;
  uint8_t width;
  uint8_t height;
} Bitmap;

#endif
