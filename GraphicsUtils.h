#ifndef GRAPHICS_UTILS_H
#define GRAPHICS_UTILS_H

#include "Bitmap.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Arduino.h>
#include <cstdint>
// ---------------- Drawing Utilities ----------------

// draw bitmap stored in flash
void drawBitmapFlash(Adafruit_SSD1306 &display, int16_t x, int16_t y,
                     const Bitmap *bmp);

// draw bitmap centered on screen
void drawBitmapCentered(Adafruit_SSD1306 &display, const Bitmap *bmp);

// draw bitmap centered at arbitrary position
void drawBitmapCenteredAt(Adafruit_SSD1306 &display, int16_t cx, int16_t cy,
                          const Bitmap *bmp);

// draw horizontal progress bar
void drawProgressBar(Adafruit_SSD1306 &display, int16_t x, int16_t y,
                     int16_t width, int16_t height, uint8_t percent);

// draw simple frame
void drawFrame(Adafruit_SSD1306 &display, int16_t x, int16_t y, int16_t w,
               int16_t h);

// clear rectangular region
void clearRect(Adafruit_SSD1306 &display, int16_t x, int16_t y, int16_t w,
               int16_t h);

#endif
