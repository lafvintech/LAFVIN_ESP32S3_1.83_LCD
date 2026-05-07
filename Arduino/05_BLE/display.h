#ifndef __DISPLAY_H
#define __DISPLAY_H

#include <lvgl.h>

// Screen configuration
#define SCREEN_WIDTH  240
#define SCREEN_HEIGHT 284

// Screen rotation
// 0: Portrait
// 1: Landscape
// 2: Portrait inverted
// 3: Landscape inverted
#define SCREEN_ROTATION 0

class Display {
public:
  // Initialize LVGL, the TFT driver, and the backlight.
  void init();

  // Run LVGL timers, animations, and pending display work.
  // Call this regularly from loop(), typically every 5 ms.
  void routine();

  // Set backlight brightness as a percentage from 0 to 100.
  void setBacklight(uint8_t brightness_percent);
};

#endif
