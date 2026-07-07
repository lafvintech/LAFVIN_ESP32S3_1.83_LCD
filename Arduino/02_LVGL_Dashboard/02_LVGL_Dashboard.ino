#include "display.h"

// Comfortable screen brightness for normal indoor testing.
static const uint8_t SCREEN_BACKLIGHT_BRIGHTNESS = 60;

Display screen;
static lv_obj_t *uptime_label = nullptr;
static lv_obj_t *load_arc = nullptr;
static lv_obj_t *load_value_label = nullptr;
static lv_obj_t *load_caption_label = nullptr;
static lv_obj_t *brightness_bar = nullptr;
static lv_obj_t *brightness_label = nullptr;
static lv_obj_t *control_switch = nullptr;
static lv_obj_t *control_state_label = nullptr;

static uint8_t triangle_wave(uint16_t phase) {
  phase %= 200;
  return phase <= 100 ? phase : 200 - phase;
}

static lv_obj_t *create_panel(lv_obj_t *parent, int16_t w, int16_t h, lv_align_t align,
                              int16_t x, int16_t y) {
  lv_obj_t *panel = lv_obj_create(parent);
  lv_obj_remove_style_all(panel);
  lv_obj_set_size(panel, w, h);
  lv_obj_align(panel, align, x, y);
  lv_obj_set_style_bg_color(panel, lv_color_hex(0xffffff), 0);
  lv_obj_set_style_bg_opa(panel, LV_OPA_COVER, 0);
  lv_obj_set_style_border_color(panel, lv_color_hex(0xd8dee4), 0);
  lv_obj_set_style_border_width(panel, 1, 0);
  lv_obj_set_style_radius(panel, 8, 0);
  lv_obj_set_style_pad_all(panel, 8, 0);
  return panel;
}

static void update_dashboard(lv_timer_t *timer) {
  (void)timer;

  static uint16_t phase = 0;
  phase += 4;

  uint8_t load = triangle_wave(phase);
  uint8_t brightness = 35 + (triangle_wave(phase + 60) / 2);
  bool control_enabled = (phase % 160) < 80;
  uint32_t seconds = millis() / 1000;

  lv_label_set_text_fmt(uptime_label, "%02lu:%02lu",
                        static_cast<unsigned long>(seconds / 60),
                        static_cast<unsigned long>(seconds % 60));

  lv_arc_set_value(load_arc, load);
  lv_label_set_text_fmt(load_value_label, "%u%%", load);
  lv_label_set_text_fmt(load_caption_label, "%u fps", 24 + (load / 5));

  lv_bar_set_value(brightness_bar, brightness, LV_ANIM_ON);
  lv_label_set_text_fmt(brightness_label, "Backlight %u%%", brightness);

  if (control_enabled) {
    lv_obj_add_state(control_switch, LV_STATE_CHECKED);
    lv_label_set_text(control_state_label, "ON");
  } else {
    lv_obj_clear_state(control_switch, LV_STATE_CHECKED);
    lv_label_set_text(control_state_label, "OFF");
  }
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("02 LVGL dashboard demo");

  screen.init();
  screen.setBacklight(SCREEN_BACKLIGHT_BRIGHTNESS);

  lv_obj_t *root = lv_scr_act();
  lv_obj_clear_flag(root, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_style_bg_color(root, lv_color_hex(0xf1f1f1), 0);
  lv_obj_set_style_bg_opa(root, LV_OPA_COVER, 0);

  lv_obj_t *header = lv_obj_create(root);
  lv_obj_remove_style_all(header);
  lv_obj_set_size(header, SCREEN_WIDTH, 34);
  lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 0);
  lv_obj_set_style_bg_color(header, lv_color_hex(0xffffff), 0);
  lv_obj_set_style_bg_opa(header, LV_OPA_COVER, 0);
  lv_obj_set_style_border_color(header, lv_color_hex(0xd8dee4), 0);
  lv_obj_set_style_border_side(header, LV_BORDER_SIDE_BOTTOM, 0);
  lv_obj_set_style_border_width(header, 1, 0);

  lv_obj_t *title = lv_label_create(header);
  lv_label_set_text(title, "1.83 LVGL");
  lv_obj_set_style_text_color(title, lv_color_hex(0x0f172a), 0);
  lv_obj_set_style_text_font(title, &lv_font_montserrat_16, 0);
  lv_obj_align(title, LV_ALIGN_LEFT_MID, 15, 0);

  uptime_label = lv_label_create(header);
  lv_label_set_text(uptime_label, "00:00");
  lv_obj_set_style_text_color(uptime_label, lv_color_hex(0x64748b), 0);
  lv_obj_set_style_text_font(uptime_label, &lv_font_montserrat_14, 0);
  lv_obj_align(uptime_label, LV_ALIGN_RIGHT_MID, -15, 0);

  lv_obj_t *arc_panel = create_panel(root, 214, 122, LV_ALIGN_TOP_MID, 0, 44);

  load_arc = lv_arc_create(arc_panel);
  lv_obj_set_size(load_arc, 96, 96);
  lv_obj_align(load_arc, LV_ALIGN_LEFT_MID, 4, 0);
  lv_arc_set_rotation(load_arc, 135);
  lv_arc_set_bg_angles(load_arc, 0, 270);
  lv_arc_set_range(load_arc, 0, 100);
  lv_obj_clear_flag(load_arc, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_arc_width(load_arc, 10, LV_PART_MAIN);
  lv_obj_set_style_arc_color(load_arc, lv_color_hex(0xe2e8f0), LV_PART_MAIN);
  lv_obj_set_style_arc_width(load_arc, 10, LV_PART_INDICATOR);
  lv_obj_set_style_arc_color(load_arc, lv_color_hex(0x2563eb), LV_PART_INDICATOR);
  lv_obj_set_style_bg_opa(load_arc, LV_OPA_TRANSP, LV_PART_KNOB);

  load_value_label = lv_label_create(arc_panel);
  lv_label_set_text(load_value_label, "0%");
  lv_obj_set_style_text_color(load_value_label, lv_color_hex(0x0f172a), 0);
  lv_obj_set_style_text_font(load_value_label, &lv_font_montserrat_24, 0);
  lv_obj_align_to(load_value_label, load_arc, LV_ALIGN_CENTER, 0, -3);

  lv_obj_t *panel_title = lv_label_create(arc_panel);
  lv_label_set_text(panel_title, "Dashboard");
  lv_obj_set_style_text_color(panel_title, lv_color_hex(0x0f172a), 0);
  lv_obj_set_style_text_font(panel_title, &lv_font_montserrat_18, 0);
  lv_obj_align(panel_title, LV_ALIGN_TOP_RIGHT, -6, 8);

  load_caption_label = lv_label_create(arc_panel);
  lv_label_set_text(load_caption_label, "24 fps");
  lv_obj_set_style_text_color(load_caption_label, lv_color_hex(0x475569), 0);
  lv_obj_set_style_text_font(load_caption_label, &lv_font_montserrat_12, 0);
  lv_obj_align(load_caption_label, LV_ALIGN_RIGHT_MID, -4, 6);

  lv_obj_t *status = lv_label_create(arc_panel);
  lv_label_set_text(status, "RUNNING");
  lv_obj_set_style_text_color(status, lv_color_hex(0x16a34a), 0);
  lv_obj_set_style_text_font(status, &lv_font_montserrat_12, 0);
  lv_obj_align(status, LV_ALIGN_BOTTOM_RIGHT, -6, -8);

  lv_obj_t *bar_panel = create_panel(root, 214, 42, LV_ALIGN_TOP_MID, 0, 172);

  brightness_label = lv_label_create(bar_panel);
  lv_label_set_text(brightness_label, "Backlight 60%");
  lv_obj_set_style_text_color(brightness_label, lv_color_hex(0x0f172a), 0);
  lv_obj_set_style_text_font(brightness_label, &lv_font_montserrat_12, 0);
  lv_obj_align(brightness_label, LV_ALIGN_TOP_LEFT, 0, -2);

  brightness_bar = lv_bar_create(bar_panel);
  lv_obj_set_size(brightness_bar, 198, 10);
  lv_obj_align(brightness_bar, LV_ALIGN_BOTTOM_MID, 0, -1);
  lv_bar_set_range(brightness_bar, 0, 100);
  lv_obj_set_style_bg_color(brightness_bar, lv_color_hex(0xe2e8f0), LV_PART_MAIN);
  lv_obj_set_style_bg_color(brightness_bar, lv_color_hex(0xf59e0b), LV_PART_INDICATOR);
  lv_obj_set_style_radius(brightness_bar, 5, LV_PART_MAIN);
  lv_obj_set_style_radius(brightness_bar, 5, LV_PART_INDICATOR);

  lv_obj_t *control_panel = create_panel(root, 214, 54, LV_ALIGN_TOP_MID, 0, 222);

  lv_obj_t *spinner = lv_spinner_create(control_panel, 900, 60);
  lv_obj_set_size(spinner, 30, 30);
  lv_obj_align(spinner, LV_ALIGN_BOTTOM_LEFT, 0, 1);
  lv_obj_set_style_arc_width(spinner, 4, LV_PART_MAIN);
  lv_obj_set_style_arc_color(spinner, lv_color_hex(0xe2e8f0), LV_PART_MAIN);
  lv_obj_set_style_arc_width(spinner, 4, LV_PART_INDICATOR);
  lv_obj_set_style_arc_color(spinner, lv_color_hex(0x2563eb), LV_PART_INDICATOR);

  lv_obj_t *button = lv_btn_create(control_panel);
  lv_obj_set_size(button, 54, 28);
  lv_obj_align(button, LV_ALIGN_BOTTOM_MID, -4, 0);
  lv_obj_clear_flag(button, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_bg_color(button, lv_color_hex(0x2563eb), 0);
  lv_obj_set_style_radius(button, 6, 0);
  lv_obj_t *button_label = lv_label_create(button);
  lv_label_set_text(button_label, "OK");
  lv_obj_set_style_text_color(button_label, lv_color_hex(0xffffff), 0);
  lv_obj_set_style_text_font(button_label, &lv_font_montserrat_12, 0);
  lv_obj_center(button_label);

  control_switch = lv_switch_create(control_panel);
  lv_obj_set_size(control_switch, 42, 22);
  lv_obj_align(control_switch, LV_ALIGN_RIGHT_MID, -2, 6);
  lv_obj_clear_flag(control_switch, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_style_bg_color(control_switch, lv_color_hex(0xe2e8f0), LV_PART_MAIN);
  lv_obj_set_style_bg_color(control_switch, lv_color_hex(0x16a34a), LV_PART_INDICATOR | LV_STATE_CHECKED);
  lv_obj_set_style_bg_color(control_switch, lv_color_hex(0xffffff), LV_PART_KNOB);

  control_state_label = lv_label_create(control_panel);
  lv_label_set_text(control_state_label, "ON");
  lv_obj_set_style_text_color(control_state_label, lv_color_hex(0x475569), 0);
  lv_obj_set_style_text_font(control_state_label, &lv_font_montserrat_10, 0);
  lv_obj_align_to(control_state_label, control_switch, LV_ALIGN_OUT_TOP_MID, 0, -2);

  update_dashboard(nullptr);
  lv_timer_create(update_dashboard, 120, NULL);
}

void loop() {
  screen.routine();
  delay(5);
}

