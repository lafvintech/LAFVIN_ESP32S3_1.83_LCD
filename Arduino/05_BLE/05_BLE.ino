#include "display.h"

#include <Arduino.h>
#include <BLE2902.h>
#include <BLECharacteristic.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>

// Comfortable screen brightness for normal indoor testing.
static const uint8_t SCREEN_BACKLIGHT_BRIGHTNESS = 70;

// Limit the text length shown on screen so the layout stays clean.
static const size_t MAX_MESSAGE_LEN = 160;

// BLE UART-style service UUIDs.
// Many phone tools such as nRF Connect or LightBlue can work with this pattern.
static const char *DEVICE_NAME = "ESP32S3 BLE";
static const char *SERVICE_UUID = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E";
static const char *RX_CHAR_UUID = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E";
static const char *TX_CHAR_UUID = "6E400003-B5A3-F393-E0A9-E50E24DCCA9E";

static Display screen;

static BLEServer *ble_server = nullptr;
static BLECharacteristic *rx_characteristic = nullptr;
static BLECharacteristic *tx_characteristic = nullptr;

static bool ble_connected = false;
static bool pending_message_ready = false;
static uint32_t received_message_count = 0;

static char latest_message[MAX_MESSAGE_LEN + 1] = "Waiting for phone message...";
static char pending_message[MAX_MESSAGE_LEN + 1] = "";

static lv_obj_t *title_label = nullptr;
static lv_obj_t *status_label = nullptr;
static lv_obj_t *device_label = nullptr;
static lv_obj_t *count_label = nullptr;
static lv_obj_t *message_panel = nullptr;
static lv_obj_t *message_label = nullptr;
static lv_obj_t *footer_label = nullptr;

// BLE callbacks do not run in the same place as the normal LVGL update loop.
// We use a very small critical section to safely pass text into loop().
static portMUX_TYPE message_lock = portMUX_INITIALIZER_UNLOCKED;

static void copy_printable_text(char *dest, size_t dest_size, const String &src) {
  if (dest_size == 0) {
    return;
  }

  size_t out = 0;
  for (size_t i = 0; i < src.length() && out < (dest_size - 1); ++i) {
    uint8_t ch = static_cast<uint8_t>(src[i]);
    if (ch == '\r') {
      continue;
    }
    if (ch == '\n' || (ch >= 32 && ch <= 126)) {
      dest[out++] = static_cast<char>(ch);
    } else {
      dest[out++] = '.';
    }
  }

  dest[out] = '\0';

  // Show a friendly placeholder when the phone sends an empty payload.
  if (out == 0) {
    strncpy(dest, "(empty message)", dest_size - 1);
    dest[dest_size - 1] = '\0';
  }
}

static void refresh_status_label() {
  if (!status_label) {
    return;
  }

  if (ble_connected) {
    lv_label_set_text(status_label, "BLE: connected");
    lv_obj_set_style_text_color(status_label, lv_color_hex(0x22c55e), 0);
  } else {
    lv_label_set_text(status_label, "BLE: advertising");
    lv_obj_set_style_text_color(status_label, lv_color_hex(0xf59e0b), 0);
  }
}

static void refresh_count_label() {
  if (!count_label) {
    return;
  }

  char line[48];
  snprintf(line, sizeof(line), "RX count: %lu", static_cast<unsigned long>(received_message_count));
  lv_label_set_text(count_label, line);
}

static void refresh_message_label() {
  if (!message_label) {
    return;
  }

  lv_label_set_text(message_label, latest_message);
}

static void refresh_ui() {
  refresh_status_label();
  refresh_count_label();
  refresh_message_label();
}

static void queue_received_message(const String &value) {
  char local_buffer[MAX_MESSAGE_LEN + 1];
  copy_printable_text(local_buffer, sizeof(local_buffer), value);

  // Only cache the data here.
  // The actual LVGL label update is handled later in loop().
  portENTER_CRITICAL(&message_lock);
  strncpy(pending_message, local_buffer, sizeof(pending_message) - 1);
  pending_message[sizeof(pending_message) - 1] = '\0';
  pending_message_ready = true;
  portEXIT_CRITICAL(&message_lock);
}

class TextServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *server) override {
    LV_UNUSED(server);
    ble_connected = true;
    Serial.println("[BLE] Phone connected");
  }

  void onDisconnect(BLEServer *server) override {
    LV_UNUSED(server);
    ble_connected = false;
    BLEDevice::startAdvertising();
    Serial.println("[BLE] Phone disconnected, advertising restarted");
  }
};

class RxCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *characteristic) override {
    // RX is the characteristic the phone writes text into.
    String value = characteristic->getValue();
    queue_received_message(value);

    if (tx_characteristic != nullptr) {
      // Echo the received text back through TX.
      // This makes it easy to confirm the BLE link is working in phone apps.
      String reply = "RX:";
      reply += value;
      tx_characteristic->setValue(reply);
      tx_characteristic->notify();
    }

    Serial.print("[BLE] RX: ");
    Serial.println(value);
  }
};

static void create_ui() {
  lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0x081018), 0);
  lv_obj_set_style_bg_opa(lv_scr_act(), LV_OPA_COVER, 0);

  title_label = lv_label_create(lv_scr_act());
  lv_label_set_text(title_label, "05 BLE comm.");
  lv_obj_set_style_text_color(title_label, lv_color_hex(0xe2e8f0), 0);
  lv_obj_set_style_text_font(title_label, &lv_font_montserrat_16, 0);
  lv_obj_align(title_label, LV_ALIGN_TOP_MID, 0, 10);

  status_label = lv_label_create(lv_scr_act());
  lv_obj_set_style_text_font(status_label, &lv_font_montserrat_14, 0);
  lv_obj_align(status_label, LV_ALIGN_TOP_LEFT, 14, 40);

  count_label = lv_label_create(lv_scr_act());
  lv_obj_set_style_text_color(count_label, lv_color_hex(0x94a3b8), 0);
  lv_obj_set_style_text_font(count_label, &lv_font_montserrat_14, 0);
  lv_obj_align(count_label, LV_ALIGN_TOP_RIGHT, -14, 40);

  device_label = lv_label_create(lv_scr_act());
  lv_label_set_text(device_label, "Name: ESP32S3 BLE");
  lv_obj_set_style_text_color(device_label, lv_color_hex(0x38bdf8), 0);
  lv_obj_set_style_text_font(device_label, &lv_font_montserrat_14, 0);
  lv_obj_align(device_label, LV_ALIGN_TOP_LEFT, 14, 66);

  message_panel = lv_obj_create(lv_scr_act());
  lv_obj_set_size(message_panel, 212, 120);
  lv_obj_align(message_panel, LV_ALIGN_CENTER, 0, 8);
  lv_obj_set_style_radius(message_panel, 18, 0);
  lv_obj_set_style_border_width(message_panel, 1, 0);
  lv_obj_set_style_border_color(message_panel, lv_color_hex(0x1e293b), 0);
  lv_obj_set_style_bg_color(message_panel, lv_color_hex(0x111827), 0);
  lv_obj_set_style_bg_opa(message_panel, LV_OPA_COVER, 0);
  lv_obj_set_style_pad_all(message_panel, 12, 0);
  lv_obj_clear_flag(message_panel, LV_OBJ_FLAG_SCROLLABLE);

  message_label = lv_label_create(message_panel);
  lv_label_set_long_mode(message_label, LV_LABEL_LONG_WRAP);
  lv_obj_set_width(message_label, 188);
  lv_obj_set_style_text_color(message_label, lv_color_hex(0xf8fafc), 0);
  lv_obj_set_style_text_font(message_label, &lv_font_montserrat_16, 0);
  lv_obj_align(message_label, LV_ALIGN_TOP_LEFT, 0, 0);

  footer_label = lv_label_create(lv_scr_act());
  lv_label_set_text(footer_label,
                    "App: nRF Connect / LightBlue\n"
                    "Write text to RX characteristic");
  lv_obj_set_style_text_color(footer_label, lv_color_hex(0x94a3b8), 0);
  lv_obj_set_style_text_font(footer_label, &lv_font_montserrat_12, 0);
  lv_obj_align(footer_label, LV_ALIGN_BOTTOM_MID, 0, -10);

  refresh_ui();
}

static void init_ble_text_lab() {
  // 1. Start the BLE stack and set the device name shown on the phone.
  BLEDevice::init(DEVICE_NAME);

  // 2. Create a BLE server and attach connect/disconnect callbacks.
  ble_server = BLEDevice::createServer();
  ble_server->setCallbacks(new TextServerCallbacks());

  // 3. Create one custom service that contains TX and RX characteristics.
  BLEService *service = ble_server->createService(SERVICE_UUID);

  // TX sends notifications back to the phone.
  tx_characteristic = service->createCharacteristic(
      TX_CHAR_UUID, BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_READ);
  tx_characteristic->addDescriptor(new BLE2902());
  tx_characteristic->setValue("ESP32S3 ready");

  // RX receives text written by the phone.
  rx_characteristic = service->createCharacteristic(
      RX_CHAR_UUID, BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR);
  rx_characteristic->setCallbacks(new RxCallbacks());

  // 4. Start the service and begin advertising.
  service->start();

  BLEAdvertising *advertising = BLEDevice::getAdvertising();
  advertising->addServiceUUID(SERVICE_UUID);
  advertising->setScanResponse(true);
  advertising->start();

  Serial.println("[BLE] Advertising as ESP32S3 BLE Text");
}

static void apply_pending_message_if_needed() {
  if (!pending_message_ready) {
    return;
  }

  char local_copy[MAX_MESSAGE_LEN + 1];

  // Move the cached message from the BLE side into the UI side.
  portENTER_CRITICAL(&message_lock);
  strncpy(local_copy, pending_message, sizeof(local_copy) - 1);
  local_copy[sizeof(local_copy) - 1] = '\0';
  pending_message_ready = false;
  portEXIT_CRITICAL(&message_lock);

  strncpy(latest_message, local_copy, sizeof(latest_message) - 1);
  latest_message[sizeof(latest_message) - 1] = '\0';
  received_message_count++;

  refresh_count_label();
  refresh_message_label();
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("05 BLE comm.");

  // Bring up the shared LVGL display helper first.
  screen.init();
  screen.setBacklight(SCREEN_BACKLIGHT_BRIGHTNESS);

  // Build the screen, then start BLE advertising.
  create_ui();
  init_ble_text_lab();
}

void loop() {
  // Keep the status fresh, apply any new BLE text, then let LVGL render.
  refresh_status_label();
  apply_pending_message_if_needed();
  screen.routine();
  delay(5);
}
