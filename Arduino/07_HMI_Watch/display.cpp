#include "display.h"

#include <Arduino.h>
#include <TFT_eSPI.h>

#include "ui_common.h"

#define LVGL_BUF_SIZE (SCREEN_WIDTH * 20)

#if defined(TFT_BL)
static constexpr uint32_t BACKLIGHT_PWM_FREQ = 5000;
static constexpr uint8_t BACKLIGHT_PWM_RESOLUTION = 8;
#endif

static lv_disp_draw_buf_t draw_buf;
static lv_color_t draw_buf_pixels[LVGL_BUF_SIZE];
static bool backlight_pwm_ready = false;
static TFT_eSPI tft = TFT_eSPI(SCREEN_WIDTH, SCREEN_HEIGHT);

static uint8_t brightness_percent_to_duty(uint8_t brightness_percent) {
  return static_cast<uint8_t>((static_cast<uint16_t>(brightness_percent) * 255) / 100);
}

static void init_backlight() {
#if defined(TFT_BL)
  pinMode(TFT_BL, OUTPUT);
  backlight_pwm_ready = ledcAttach(TFT_BL, BACKLIGHT_PWM_FREQ, BACKLIGHT_PWM_RESOLUTION);

  if (backlight_pwm_ready) {
    ledcWrite(TFT_BL, TFT_BACKLIGHT_ON ? 255 : 0);
  } else {
    digitalWrite(TFT_BL, TFT_BACKLIGHT_ON);
  }
#endif
}

static void display_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
  const uint32_t w = area->x2 - area->x1 + 1;
  const uint32_t h = area->y2 - area->y1 + 1;

  tft.startWrite();
  tft.setAddrWindow(area->x1, area->y1, w, h);
  tft.pushColors(reinterpret_cast<uint16_t *>(&color_p->full), w * h, true);
  tft.endWrite();

  lv_disp_flush_ready(disp);
}

void Display::init() {
  lv_init();

  tft.begin();
  tft.setRotation(SCREEN_ROTATION);
  tft.fillScreen(TFT_BLACK);

  init_backlight();

  ui_screen_width = SCREEN_WIDTH;
  ui_screen_height = SCREEN_HEIGHT;

  lv_disp_draw_buf_init(&draw_buf, draw_buf_pixels, nullptr, LVGL_BUF_SIZE);

  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = SCREEN_WIDTH;
  disp_drv.ver_res = SCREEN_HEIGHT;
  disp_drv.flush_cb = display_flush;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);
}

void Display::routine() {
  lv_timer_handler();
}

void Display::setBacklight(uint8_t brightness_percent) {
#if defined(TFT_BL)
  if (brightness_percent > 100) {
    brightness_percent = 100;
  }

  const uint8_t pwm_duty_8bit = brightness_percent_to_duty(brightness_percent);

  if (backlight_pwm_ready) {
    const uint8_t output_duty = TFT_BACKLIGHT_ON
                                    ? pwm_duty_8bit
                                    : static_cast<uint8_t>(255 - pwm_duty_8bit);
    ledcWrite(TFT_BL, output_duty);
  } else {
    digitalWrite(TFT_BL, brightness_percent > 0 ? TFT_BACKLIGHT_ON : !TFT_BACKLIGHT_ON);
  }
#else
  LV_UNUSED(brightness_percent);
#endif
}
