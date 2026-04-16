#include "GraphicsUtils.h"

// --------------------------------------------------
// Bitmap
// --------------------------------------------------

void drawBitmapFlash(Adafruit_SSD1306 &display, int16_t x, int16_t y,
                     const Bitmap *bmp) {
  display.drawBitmap(x, y, bmp->data, bmp->width, bmp->height, WHITE);
}

// --------------------------------------------------
// Centering helpers
// --------------------------------------------------

void drawBitmapCentered(Adafruit_SSD1306 &display, const Bitmap *bmp) {
  int16_t x = (display.width() - bmp->width) / 2;
  int16_t y = (display.height() - bmp->height) / 2;

  drawBitmapFlash(display, x, y, bmp);
}

void drawBitmapCenteredAt(Adafruit_SSD1306 &display, int16_t cx, int16_t cy,
                          const Bitmap *bmp) {
  int16_t x = cx - (bmp->width / 2);
  int16_t y = cy - (bmp->height / 2);

  drawBitmapFlash(display, x, y, bmp);
}

// --------------------------------------------------
// UI primitives
// --------------------------------------------------

void drawProgressBar(Adafruit_SSD1306 &display, int16_t x, int16_t y,
                     int16_t width, int16_t height, uint8_t percent) {
  if (percent > 100)
    percent = 100;

  display.drawRect(x, y, width, height, WHITE);

  int16_t fill = (width - 2) * percent / 100;

  if (fill > 0) {
    display.fillRect(x + 1, y + 1, fill, height - 2, WHITE);
  }
}

void drawFrame(Adafruit_SSD1306 &display, int16_t x, int16_t y, int16_t w,
               int16_t h) {
  display.drawRect(x, y, w, h, WHITE);
}

void clearRect(Adafruit_SSD1306 &display, int16_t x, int16_t y, int16_t w,
               int16_t h) {
  display.fillRect(x, y, w, h, BLACK);
}
