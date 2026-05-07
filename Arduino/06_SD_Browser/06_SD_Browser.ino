#include "display.h"
#include "sd_browser.h"

#include <Arduino.h>
#include <SD.h>

static const uint8_t BOOT_BUTTON_PIN = 0;
static const uint8_t SCREEN_BACKLIGHT_BRIGHTNESS = 70;
static const uint32_t BUTTON_DEBOUNCE_MS = 25;
static const uint32_t MULTI_CLICK_TIMEOUT_MS = 320;
static const size_t MAX_DIR_ENTRIES = 32;
static const uint8_t VISIBLE_LIST_ROWS = 7;
static const size_t FILE_PREVIEW_LIMIT = 1024;

enum class AppView : uint8_t {
  Error,
  List,
  Preview
};

static Display screen;
static AppView current_view = AppView::Error;

// SD card state used by the UI.
static bool sd_mounted = false;
static SdCardInfo sd_card_info = {CARD_NONE, 0, 0, 0};
static String app_error_message = "Insert SD card and retry";

// Current folder state.
static String current_path = "/";
static SdEntry current_entries[MAX_DIR_ENTRIES];
static size_t current_entry_count = 0;
static bool current_list_truncated = false;
static size_t selected_index = 0;
static size_t list_scroll_offset = 0;

// Current file preview state.
static String preview_path;
static String preview_name;
static String preview_content;
static uint64_t preview_size = 0;
static bool preview_is_text = false;
static bool preview_is_truncated = false;
static String preview_error_message;

// Button state for single, double, and triple click detection.
static bool button_raw_state = HIGH;
static bool button_stable_state = HIGH;
static uint32_t button_last_change_ms = 0;
static uint32_t last_release_ms = 0;
static uint8_t pending_clicks = 0;

// LVGL objects used by the simple UI.
static lv_obj_t *title_label = nullptr;
static lv_obj_t *status_label = nullptr;
static lv_obj_t *path_label = nullptr;
static lv_obj_t *card_label = nullptr;
static lv_obj_t *content_panel = nullptr;
static lv_obj_t *content_label = nullptr;
static lv_obj_t *footer_label = nullptr;

static void render_current_view();
static bool load_directory(const String &path);
static void try_mount_sd_card();

// Convert bytes to a short human-friendly size string.
static String human_size(uint64_t bytes) {
  if (bytes < 1024) {
    return String(bytes) + " B";
  }

  const char *units[] = {"KB", "MB", "GB"};
  double value = bytes / 1024.0;
  size_t unit_index = 0;
  while (value >= 1024.0 && unit_index < 2) {
    value /= 1024.0;
    unit_index++;
  }

  char buffer[24];
  snprintf(buffer, sizeof(buffer), "%.1f %s", value, units[unit_index]);
  return String(buffer);
}

// Return the parent folder path.
static String parent_path(const String &path) {
  if (path.length() <= 1) {
    return "/";
  }

  int last_slash = path.lastIndexOf('/');
  if (last_slash <= 0) {
    return "/";
  }
  return path.substring(0, last_slash);
}

// Shorten long text so it fits on the small screen.
static String shorten_text(const String &text, size_t max_chars) {
  if (text.length() <= max_chars) {
    return text;
  }
  if (max_chars <= 3) {
    return text.substring(0, max_chars);
  }
  return text.substring(0, max_chars - 3) + "...";
}

// Count visible rows, including the "Up" item when inside a subfolder.
static size_t visible_item_count() {
  return current_entry_count + (current_path != "/" ? 1 : 0);
}

// Build one line of text for the file list.
static String list_item_label(size_t visible_index) {
  if (current_path != "/" && visible_index == 0) {
    return "[..] Up";
  }

  size_t entry_index = current_path != "/" ? visible_index - 1 : visible_index;
  const SdEntry &entry = current_entries[entry_index];
  String prefix = entry.isDirectory ? "[D] " : "[F] ";
  return prefix + shorten_text(entry.name, 22);
}

// Build the short SD card summary shown near the top.
static String card_summary() {
  if (!sd_mounted) {
    return "Card: unavailable";
  }

  return String("Card: ") + sdCardTypeLabel(sd_card_info.cardType) +
         "  Size: " + human_size(sd_card_info.cardSizeBytes);
}

// Keep the selected row inside the visible area.
static void sync_list_scroll() {
  if (selected_index < list_scroll_offset) {
    list_scroll_offset = selected_index;
  }

  size_t max_visible = list_scroll_offset + VISIBLE_LIST_ROWS;
  if (selected_index >= max_visible) {
    list_scroll_offset = selected_index - VISIBLE_LIST_ROWS + 1;
  }
}

// Open one file and load a short preview into memory.
static bool open_preview(const SdEntry &entry) {
  preview_path = entry.path;
  preview_name = entry.name;
  preview_content = "";
  preview_size = 0;
  preview_is_text = false;
  preview_is_truncated = false;
  preview_error_message = "";

  String error_message;
  if (!sdReadPreview(SD, entry.path, FILE_PREVIEW_LIMIT, preview_content,
                     preview_size, preview_is_text, preview_is_truncated,
                     error_message)) {
    preview_error_message = error_message;
    return false;
  }

  current_view = AppView::Preview;
  render_current_view();
  return true;
}

// Read a folder and refresh the list page.
static bool load_directory(const String &path) {
  String error_message;
  size_t entry_count = 0;
  bool truncated = false;

  if (!sdListDirectory(SD, path, current_entries, MAX_DIR_ENTRIES,
                       entry_count, truncated, error_message)) {
    app_error_message = error_message;
    current_view = AppView::Error;
    render_current_view();
    return false;
  }

  current_path = path;
  current_entry_count = entry_count;
  current_list_truncated = truncated;
  selected_index = 0;
  list_scroll_offset = 0;
  current_view = AppView::List;
  render_current_view();
  return true;
}

// Mount the SD card and jump to the root folder on success.
static void try_mount_sd_card() {
  sd_mounted = false;
  sd_card_info = {CARD_NONE, 0, 0, 0};

  if (!sdBegin(sd_card_info, app_error_message)) {
    current_view = AppView::Error;
    render_current_view();
    return;
  }

  sd_mounted = true;
  Serial.printf("[SD] Mounted card type=%s size=%lluMB used=%lluMB\n",
                sdCardTypeLabel(sd_card_info.cardType),
                sd_card_info.cardSizeBytes / (1024ULL * 1024ULL),
                sd_card_info.usedBytes / (1024ULL * 1024ULL));

  load_directory("/");
}

// Move the selection to the next visible item.
static void select_next_item() {
  if (current_view != AppView::List) {
    return;
  }

  size_t item_count = visible_item_count();
  if (item_count == 0) {
    return;
  }

  selected_index = (selected_index + 1) % item_count;
  sync_list_scroll();
  render_current_view();
}

// Open the current item, or go up when the special "Up" item is selected.
static void activate_selected_item() {
  if (current_view != AppView::List) {
    return;
  }

  if (current_path != "/" && selected_index == 0) {
    load_directory(parent_path(current_path));
    return;
  }

  if (current_entry_count == 0) {
    return;
  }

  size_t entry_index = current_path != "/" ? selected_index - 1 : selected_index;
  const SdEntry &entry = current_entries[entry_index];

  if (entry.isDirectory) {
    load_directory(entry.path);
  } else {
    open_preview(entry);
  }
}

// Single click only moves the cursor in the list.
static void handle_single_click() {
  if (current_view == AppView::Error) {
    try_mount_sd_card();
    return;
  }

  if (current_view == AppView::List) {
    select_next_item();
  }
}

// Double click opens the selected folder or file.
static void handle_double_click() {
  if (current_view == AppView::Error) {
    try_mount_sd_card();
    return;
  }

  if (current_view == AppView::List) {
    activate_selected_item();
  }
}

// Triple click goes back from preview or from a subfolder.
static void handle_triple_click() {
  if (current_view == AppView::Error) {
    try_mount_sd_card();
    return;
  }

  if (current_view == AppView::Preview) {
    current_view = AppView::List;
    render_current_view();
    return;
  }

  if (current_view == AppView::List && current_path != "/") {
    load_directory(parent_path(current_path));
  }
}

// Send the final click count to the correct action.
static void dispatch_clicks(uint8_t click_count) {
  if (click_count == 1) {
    handle_single_click();
  } else if (click_count == 2) {
    handle_double_click();
  } else if (click_count >= 3) {
    handle_triple_click();
  }
}

// Read the button with debounce and collect click events.
static void poll_button() {
  uint32_t now = millis();
  bool raw_state = digitalRead(BOOT_BUTTON_PIN);

  if (raw_state != button_raw_state) {
    button_raw_state = raw_state;
    button_last_change_ms = now;
  }

  if ((now - button_last_change_ms) < BUTTON_DEBOUNCE_MS) {
    return;
  }

  if (button_stable_state == button_raw_state) {
    return;
  }

  button_stable_state = button_raw_state;

  if (button_stable_state == LOW) {
    return;
  }

  pending_clicks++;
  last_release_ms = now;
}

// Wait until the multi-click window ends, then run the action.
static void process_pending_clicks() {
  if (pending_clicks == 0) {
    return;
  }

  if ((millis() - last_release_ms) < MULTI_CLICK_TIMEOUT_MS) {
    return;
  }

  uint8_t click_count = pending_clicks;
  pending_clicks = 0;
  dispatch_clicks(click_count);
}

// Create the simple LVGL widgets used by this demo.
static void create_ui() {
  lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x081018), 0);
  lv_obj_set_style_bg_opa(lv_scr_act(), LV_OPA_COVER, 0);

  title_label = lv_label_create(lv_scr_act());
  lv_obj_set_style_text_font(title_label, &lv_font_montserrat_16, 0);
  lv_obj_set_style_text_color(title_label, lv_color_hex(0xe2e8f0), 0);
  lv_obj_align(title_label, LV_ALIGN_TOP_LEFT, 10, 10);

  status_label = lv_label_create(lv_scr_act());
  lv_obj_set_width(status_label, 220);
  lv_label_set_long_mode(status_label, LV_LABEL_LONG_WRAP);
  lv_obj_set_style_text_font(status_label, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(status_label, lv_color_hex(0x93c5fd), 0);
  lv_obj_align(status_label, LV_ALIGN_TOP_LEFT, 10, 34);

  path_label = lv_label_create(lv_scr_act());
  lv_obj_set_width(path_label, 220);
  lv_label_set_long_mode(path_label, LV_LABEL_LONG_WRAP);
  lv_obj_set_style_text_font(path_label, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(path_label, lv_color_hex(0x94a3b8), 0);
  lv_obj_align(path_label, LV_ALIGN_TOP_LEFT, 10, 52);

  card_label = lv_label_create(lv_scr_act());
  lv_obj_set_width(card_label, 220);
  lv_label_set_long_mode(card_label, LV_LABEL_LONG_WRAP);
  lv_obj_set_style_text_font(card_label, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(card_label, lv_color_hex(0x94a3b8), 0);
  lv_obj_align(card_label, LV_ALIGN_TOP_LEFT, 10, 92);

  content_panel = lv_obj_create(lv_scr_act());
  lv_obj_set_size(content_panel, 220, 128);
  lv_obj_align(content_panel, LV_ALIGN_TOP_MID, 0, 128);
  lv_obj_set_style_radius(content_panel, 14, 0);
  lv_obj_set_style_border_width(content_panel, 1, 0);
  lv_obj_set_style_border_color(content_panel, lv_color_hex(0x1e293b), 0);
  lv_obj_set_style_bg_color(content_panel, lv_color_hex(0x0f172a), 0);
  lv_obj_set_style_bg_opa(content_panel, LV_OPA_COVER, 0);
  lv_obj_set_style_pad_all(content_panel, 8, 0);
  lv_obj_clear_flag(content_panel, LV_OBJ_FLAG_SCROLLABLE);

  content_label = lv_label_create(content_panel);
  lv_obj_set_width(content_label, 204);
  lv_label_set_long_mode(content_label, LV_LABEL_LONG_WRAP);
  lv_obj_set_style_text_font(content_label, &lv_font_montserrat_14, 0);
  lv_obj_set_style_text_color(content_label, lv_color_hex(0xe2e8f0), 0);
  lv_obj_align(content_label, LV_ALIGN_TOP_LEFT, 0, 0);

  footer_label = lv_label_create(lv_scr_act());
  lv_obj_set_width(footer_label, 220);
  lv_label_set_long_mode(footer_label, LV_LABEL_LONG_WRAP);
  lv_obj_set_style_text_font(footer_label, &lv_font_montserrat_12, 0);
  lv_obj_set_style_text_color(footer_label, lv_color_hex(0xf8fafc), 0);
  lv_obj_align(footer_label, LV_ALIGN_BOTTOM_LEFT, 20, -10);
}

// Error page shown when the SD card cannot be mounted.
static void render_error_view() {
  lv_label_set_text(title_label, "06 SD Browser");
  lv_obj_set_style_text_color(status_label, lv_color_hex(0xf87171), 0);
  lv_label_set_text(status_label, "SD init error");
  lv_label_set_text(path_label, app_error_message.c_str());
  lv_label_set_text(card_label, "Check card and wiring");
  lv_label_set_text(content_label,
                    "Unable to mount SD card over SPI.\n\n"
                    "Single/double/triple click: retry");
  lv_label_set_text(footer_label, "SPI: SCLK14 MISO16 MOSI15 CS21");
}

// Folder list page.
static void render_list_view() {
  String status = String("Items: ") + current_entry_count;
  if (current_list_truncated) {
    status += " (showing first ";
    status += MAX_DIR_ENTRIES;
    status += ")";
  }

  lv_label_set_text(title_label, "06 SD Browser");
  lv_obj_set_style_text_color(status_label, lv_color_hex(0x86efac), 0);
  lv_label_set_text(status_label, status.c_str());
  lv_label_set_text(path_label, (String("Path: ") + current_path).c_str());
  lv_label_set_text(card_label,
                    (card_summary() + "\nUsed: " + human_size(sd_card_info.usedBytes) +
                     " / " + human_size(sd_card_info.totalBytes))
                        .c_str());

  String content;
  size_t item_count = visible_item_count();
  if (item_count == 0) {
    content = "(empty directory)";
  } else {
    for (size_t row = 0; row < VISIBLE_LIST_ROWS; ++row) {
      size_t visible_index = list_scroll_offset + row;
      if (visible_index >= item_count) {
        break;
      }

      content += (visible_index == selected_index) ? "> " : "  ";
      content += list_item_label(visible_index);
      if ((row + 1) < VISIBLE_LIST_ROWS && (visible_index + 1) < item_count) {
        content += '\n';
      }
    }
  }

  lv_label_set_text(content_label, content.c_str());
  lv_label_set_text(footer_label, "1x next  2x open 3x back/close");
}

// File preview page.
static void render_preview_view() {
  lv_label_set_text(title_label, "File Preview");
  lv_obj_set_style_text_color(status_label, lv_color_hex(0xfde68a), 0);
  lv_label_set_text(status_label, shorten_text(preview_name, 28).c_str());
  lv_label_set_text(path_label, (String("Path: ") + preview_path).c_str());

  String meta = String("Size: ") + human_size(preview_size);
  meta += preview_is_text ? "\nType: text" : "\nType: binary/unsupported";
  lv_label_set_text(card_label, meta.c_str());

  String content;
  if (preview_error_message.length() > 0) {
    content = preview_error_message;
  } else if (!preview_is_text) {
    content = "Preview not supported for this file type.\n\n"
              "Text preview currently focuses on .txt, .log, .csv, .json and files that look like plain text.";
  } else if (preview_content.length() == 0) {
    content = "(empty file)";
  } else {
    content = preview_content;
    if (preview_is_truncated) {
      content += "\n\n--- preview truncated ---";
    }
  }

  lv_label_set_text(content_label, content.c_str());
  lv_label_set_text(footer_label, "3x back to list");
}

// Pick the correct renderer for the current page.
static void render_current_view() {
  if (current_view == AppView::Error) {
    render_error_view();
  } else if (current_view == AppView::Preview) {
    render_preview_view();
  } else {
    render_list_view();
  }
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("06 SD browser preview");

  pinMode(BOOT_BUTTON_PIN, INPUT_PULLUP);
  button_raw_state = digitalRead(BOOT_BUTTON_PIN);
  button_stable_state = button_raw_state;
  button_last_change_ms = millis();

  // Start the display first so error messages can also be shown on screen.
  screen.init();
  screen.setBacklight(SCREEN_BACKLIGHT_BRIGHTNESS);
  create_ui();
  render_current_view();
  try_mount_sd_card();
}

void loop() {
  poll_button();
  process_pending_clicks();
  screen.routine();
  delay(5);
}
