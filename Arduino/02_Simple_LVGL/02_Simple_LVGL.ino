#include "display.h"

// Comfortable screen brightness for normal indoor testing.
static const uint8_t SCREEN_BACKLIGHT_BRIGHTNESS = 60;

Display screen;
static lv_obj_t *status_label = nullptr;

static void update_status(lv_timer_t *timer) {
  static uint32_t tick = 0;
  (void)timer;
  lv_label_set_text_fmt(status_label, "LVGL running\nTick: %lu", static_cast<unsigned long>(tick++));
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("02 minimal LVGL demo");

  screen.init();
  screen.setBacklight(SCREEN_BACKLIGHT_BRIGHTNESS);

  lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x101418), 0);
  lv_obj_set_style_bg_opa(lv_scr_act(), LV_OPA_COVER, 0);

  lv_obj_t *title = lv_label_create(lv_scr_act());
  lv_label_set_text(title, "1.83 inch ESP32-S3");
  lv_obj_set_style_text_color(title, lv_color_hex(0x7ee787), 0);
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 40);

  status_label = lv_label_create(lv_scr_act());
  lv_obj_set_style_text_color(status_label, lv_color_hex(0xffffff), 0);
  lv_obj_set_style_text_align(status_label, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align(status_label, LV_ALIGN_CENTER, 0, 0);

  update_status(nullptr);
  lv_timer_create(update_status, 500, NULL);
}

void loop() {
  screen.routine();
  delay(5);
}
