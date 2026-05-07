#include "display.h"

#include <Arduino.h>
#include <TFT_eSPI.h>

// Internal configuration

// Buffer enough pixels for 10 rows so LVGL can render the screen in chunks.
#define LVGL_BUF_SIZE (SCREEN_WIDTH * 10)

#if defined(TFT_BL)
static constexpr uint32_t BACKLIGHT_PWM_FREQ = 5000;
static constexpr uint8_t BACKLIGHT_PWM_RESOLUTION = 8;
#endif

// Internal state

// LVGL draw buffer descriptor used by the registered display driver.
static lv_disp_draw_buf_t draw_buf;

// Pixel storage backing the LVGL draw buffer for chunked screen updates.
static lv_color_t draw_buf_pixels[LVGL_BUF_SIZE];

// True when the backlight pin has been configured for PWM control.
static bool backlight_pwm_ready = false;

// TFT driver instance used for low-level display access.
static TFT_eSPI tft = TFT_eSPI(SCREEN_WIDTH, SCREEN_HEIGHT);

// Helper functions

static uint8_t brightness_percent_to_duty(uint8_t brightness_percent) {
  return static_cast<uint8_t>((static_cast<uint16_t>(brightness_percent) * 255) / 100);
}

static void init_backlight() {
#if defined(TFT_BL)
  pinMode(TFT_BL, OUTPUT);

  // Prefer PWM for adjustable brightness. If PWM setup is unavailable on the
  // configured pin, fall back to the original digital on/off behavior.
  backlight_pwm_ready = ledcAttach(TFT_BL, BACKLIGHT_PWM_FREQ, BACKLIGHT_PWM_RESOLUTION);

  if (backlight_pwm_ready) {
    ledcWrite(TFT_BL, TFT_BACKLIGHT_ON ? 255 : 0);
  } else {
    digitalWrite(TFT_BL, TFT_BACKLIGHT_ON);
  }
#endif
}

// LVGL flush callback

static void display_flush(lv_disp_drv_t *disp, const lv_area_t *area,
                          lv_color_t *color_p) {
  uint32_t w = area->x2 - area->x1 + 1;
  uint32_t h = area->y2 - area->y1 + 1;

  // LVGL renders into draw_buf_pixels first, then calls this callback to push
  // the completed rectangle to the TFT controller.
  tft.startWrite();
  tft.setAddrWindow(area->x1, area->y1, w, h);
  tft.pushColors((uint16_t *)&color_p->full, w * h, true);
  tft.endWrite();

  lv_disp_flush_ready(disp);
}

// Display methods

void Display::init() {
  // 1. Initialize the LVGL core.
  lv_init();

  // 2. Initialize the TFT panel.
  tft.begin();

  // 3. Apply rotation and clear the screen.
  tft.setRotation(SCREEN_ROTATION);
  tft.fillScreen(TFT_BLACK);

  // 4. Initialize the backlight output.
  init_backlight();

  // 5. Initialize the LVGL draw buffer.
  lv_disp_draw_buf_init(&draw_buf, draw_buf_pixels, NULL, LVGL_BUF_SIZE);

  // 6. Register the LVGL display driver.
  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res  = SCREEN_WIDTH;
  disp_drv.ver_res  = SCREEN_HEIGHT;
  disp_drv.flush_cb = display_flush;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);
}

void Display::routine() {
  // Call this regularly from loop() so LVGL can run timers, animations, and
  // deferred UI rendering.
  lv_timer_handler();
}

void Display::setBacklight(uint8_t brightness_percent) {
#if defined(TFT_BL)
  if (brightness_percent > 100) {
    brightness_percent = 100;
  }

  uint8_t pwm_duty_8bit = brightness_percent_to_duty(brightness_percent);

  if (backlight_pwm_ready) {
    uint8_t output_duty = TFT_BACKLIGHT_ON
                              ? pwm_duty_8bit
                              : static_cast<uint8_t>(255 - pwm_duty_8bit);
    ledcWrite(TFT_BL, output_duty);
  } else {
    // Without PWM support, keep the existing on/off fallback semantics.
    digitalWrite(TFT_BL, brightness_percent > 0 ? TFT_BACKLIGHT_ON : !TFT_BACKLIGHT_ON);
  }
#else
  LV_UNUSED(brightness_percent);
#endif
}
