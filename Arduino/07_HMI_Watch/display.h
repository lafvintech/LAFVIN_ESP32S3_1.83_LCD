#ifndef __DISPLAY_H
#define __DISPLAY_H

#include <lvgl.h>
#include <stdint.h>

#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 284
#define SCREEN_ROTATION 0

class Display {
public:
  void init();
  void routine();
  void setBacklight(uint8_t brightness_percent);
};

#endif
