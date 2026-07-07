#include "display.h"

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

// Fill these to let the board join your router in STA mode.
static const char *WIFI_SSID = "Your_WIFI_SSID";
static const char *WIFI_PASSWORD = "Your_WIFI_PASSWORD";

static const int RGB_LED_PIN = 38;
static const uint8_t RGB_LED_BRIGHTNESS = 25;
static const uint8_t SCREEN_BACKLIGHT_BRIGHTNESS = 70;

// Avoid pins already used by display/backlight/IMU.
static const uint8_t CONTROL_PINS[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
static const size_t CONTROL_PIN_COUNT = sizeof(CONTROL_PINS) / sizeof(CONTROL_PINS[0]);

Display screen;
WebServer server(80);

struct GpioState {
  uint8_t pin;
  bool level;
};

static GpioState gpio_states[CONTROL_PIN_COUNT];

static lv_obj_t *title_label = nullptr;
static lv_obj_t *mode_label = nullptr;
static lv_obj_t *ip_label = nullptr;
static lv_obj_t *gpio_left_label = nullptr;
static lv_obj_t *gpio_right_label = nullptr;
static lv_obj_t *led_label = nullptr;
static uint16_t led_label_hue = 0;

static String wifi_mode_text = "WiFi: starting";
static String wifi_ip_text = "IP: --";
static String led_color_text = "LED RGB: 0,0,0";
static uint8_t led_r = 0;
static uint8_t led_g = 0;
static uint8_t led_b = 0;

static bool is_control_pin(uint8_t pin) {
  for (size_t i = 0; i < CONTROL_PIN_COUNT; ++i) {
    if (CONTROL_PINS[i] == pin) {
      return true;
    }
  }
  return false;
}

static uint8_t scale_led_channel(uint8_t value) {
  return static_cast<uint8_t>((value * RGB_LED_BRIGHTNESS) / 255);
}

static void set_rgb_led(uint8_t r, uint8_t g, uint8_t b) {
  led_r = r;
  led_g = g;
  led_b = b;
  neopixelWrite(RGB_LED_PIN,
                scale_led_channel(r),
                scale_led_channel(g),
                scale_led_channel(b));
  led_color_text = "LED RGB: " + String(r) + "," + String(g) + "," + String(b);
}

static String gpio_state_text(size_t start_index, size_t end_index) {
  String text;
  for (size_t i = start_index; i < end_index && i < CONTROL_PIN_COUNT; ++i) {
    text += "IO";
    text += String(gpio_states[i].pin);
    text += ": ";
    text += gpio_states[i].level ? "ON" : "OFF";
    if (i + 1 < end_index && i + 1 < CONTROL_PIN_COUNT) {
      text += '\n';
    }
  }
  return text;
}

static void refresh_screen() {
  if (!mode_label) {
    return;
  }

  String gpio_left_text = gpio_state_text(0, 5);
  String gpio_right_text = gpio_state_text(5, 11);

  lv_label_set_text(mode_label, wifi_mode_text.c_str());
  lv_label_set_text(ip_label, wifi_ip_text.c_str());
  lv_label_set_text(gpio_left_label, gpio_left_text.c_str());
  lv_label_set_text(gpio_right_label, gpio_right_text.c_str());
  lv_label_set_text(led_label, led_color_text.c_str());
}

static void animate_led_label(lv_timer_t *timer) {
  LV_UNUSED(timer);
  if (!led_label) {
    return;
  }

  lv_color_t rainbow = lv_color_hsv_to_rgb(led_label_hue, 90, 100);
  lv_obj_set_style_text_color(led_label, rainbow, 0);
  led_label_hue = static_cast<uint16_t>((led_label_hue + 6) % 360);
}

static String html_page() {
  String html;
  html.reserve(5000);
  html += "<!doctype html><html><head><meta charset='utf-8'>";
  html += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
  html += "<title>ESP32S3 Control</title>";
  html += "<style>";
  html += "body{font-family:Arial,sans-serif;background:#101418;color:#f3f4f6;margin:0;padding:20px;}";
  html += ".wrap{max-width:900px;margin:0 auto;}";
  html += ".card{background:#192028;border:1px solid #2f3945;border-radius:16px;padding:18px;margin-bottom:16px;}";
  html += "h1,h2{margin:0 0 12px;} .grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(150px,1fr));gap:12px;}";
  html += "button,input{font-size:16px;border-radius:12px;border:none;padding:12px;}";
  html += "button{background:#3b82f6;color:#fff;cursor:pointer;} button.off{background:#374151;} ";
  html += ".pin{display:flex;flex-direction:column;gap:8px;background:#111827;padding:12px;border-radius:12px;}";
  html += ".row{display:flex;gap:10px;flex-wrap:wrap;align-items:center;} ";
  html += "input[type='number']{width:90px;background:#0f172a;color:#fff;border:1px solid #334155;}";
  html += "input[type='color']{width:70px;height:48px;padding:4px;background:#0f172a;}";
  html += ".status{color:#93c5fd;font-size:14px;}</style></head><body><div class='wrap'>";
  html += "<div class='card'><h1>ESP32-S3 WiFi GPIO Control</h1>";
  html += "<div class='status' id='netinfo'></div></div>";
  html += "<div class='card'><h2>GPIO Switches</h2><div class='grid'>";

  for (size_t i = 0; i < CONTROL_PIN_COUNT; ++i) {
    html += "<div class='pin'><strong>GPIO ";
    html += String(CONTROL_PINS[i]);
    html += "</strong><div class='row'><button onclick='setPin(";
    html += String(CONTROL_PINS[i]);
    html += ",1)'>ON</button><button class='off' onclick='setPin(";
    html += String(CONTROL_PINS[i]);
    html += ",0)'>OFF</button></div><div id='pin-";
    html += String(CONTROL_PINS[i]);
    html += "'>--</div></div>";
  }

  html += "</div></div>";
  html += "<div class='card'><h2>Board RGB LED</h2><div class='row'>";
  html += "<input id='color' type='color' value='#000000'>";
  html += "<button onclick='applyColor()'>Apply Color</button>";
  html += "<button class='off' onclick='turnOffLed()'>LED Off</button>";
  html += "<div id='ledinfo'>RGB(0,0,0)</div></div></div>";
  html += "</div><script>";
  html += "async function api(url){const r=await fetch(url);return await r.json();}";
  html += "function rgbHex(n){return n.toString(16).padStart(2,'0');}";
  html += "async function refresh(){const s=await api('/api/status');";
  html += "document.getElementById('netinfo').textContent=`${s.mode} | ${s.ip}`;";
  html += "document.getElementById('ledinfo').textContent=`RGB(${s.led.r},${s.led.g},${s.led.b})`;";
  html += "document.getElementById('color').value=`#${rgbHex(s.led.r)}${rgbHex(s.led.g)}${rgbHex(s.led.b)}`;";
  html += "s.gpios.forEach(g=>{const el=document.getElementById(`pin-${g.pin}`); if(el){el.textContent=g.level?'ON':'OFF';}});}";
  html += "async function setPin(pin,val){await api(`/api/gpio?pin=${pin}&value=${val}`);refresh();}";
  html += "async function applyColor(){const c=document.getElementById('color').value; const r=parseInt(c.slice(1,3),16); const g=parseInt(c.slice(3,5),16); const b=parseInt(c.slice(5,7),16); await api(`/api/led?r=${r}&g=${g}&b=${b}`); refresh();}";
  html += "async function turnOffLed(){await api('/api/led?r=0&g=0&b=0'); refresh();}";
  html += "refresh(); setInterval(refresh,1500);</script></body></html>";
  return html;
}

static void handle_root() {
  server.send(200, "text/html", html_page());
}

static void handle_status() {
  String json = "{";
  json += "\"mode\":\"" + wifi_mode_text + "\",";
  json += "\"ip\":\"" + wifi_ip_text.substring(4) + "\",";
  json += "\"led\":{\"r\":" + String(led_r) + ",\"g\":" + String(led_g) + ",\"b\":" + String(led_b) + "},";
  json += "\"gpios\":[";
  for (size_t i = 0; i < CONTROL_PIN_COUNT; ++i) {
    json += "{\"pin\":" + String(gpio_states[i].pin) + ",\"level\":" + String(gpio_states[i].level ? "true" : "false") + "}";
    if (i + 1 < CONTROL_PIN_COUNT) {
      json += ",";
    }
  }
  json += "]}";
  server.send(200, "application/json", json);
}

static void handle_gpio() {
  if (!server.hasArg("pin") || !server.hasArg("value")) {
    server.send(400, "application/json", "{\"ok\":false,\"msg\":\"missing pin/value\"}");
    return;
  }

  int pin = server.arg("pin").toInt();
  int value = server.arg("value").toInt();
  bool allowed = is_control_pin(static_cast<uint8_t>(pin));

  if (!allowed) {
    server.send(400, "application/json", "{\"ok\":false,\"msg\":\"pin not in safe list\"}");
    return;
  }

  digitalWrite(pin, value ? HIGH : LOW);
  for (size_t i = 0; i < CONTROL_PIN_COUNT; ++i) {
    if (gpio_states[i].pin == pin) {
      gpio_states[i].level = value != 0;
      break;
    }
  }

  refresh_screen();
  server.send(200, "application/json", "{\"ok\":true}");
}

static void handle_led() {
  uint8_t r = static_cast<uint8_t>(server.arg("r").toInt());
  uint8_t g = static_cast<uint8_t>(server.arg("g").toInt());
  uint8_t b = static_cast<uint8_t>(server.arg("b").toInt());
  set_rgb_led(r, g, b);
  refresh_screen();
  server.send(200, "application/json", "{\"ok\":true}");
}

static void setup_wifi() {
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);

  if (strlen(WIFI_SSID) == 0) {
    wifi_mode_text = "WiFi STA";
    wifi_ip_text = "IP: no SSID";
    return;
  }

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 12000) {
    delay(250);
  }

  if (WiFi.status() == WL_CONNECTED) {
    wifi_mode_text = "WiFi STA";
    wifi_ip_text = "IP: " + WiFi.localIP().toString();
    return;
  }

  WiFi.disconnect(true, true);
  WiFi.mode(WIFI_STA);
  wifi_mode_text = "WiFi STA failed";
  wifi_ip_text = "IP: not connected";
}

static void create_ui() {
  lv_obj_set_style_bg_color(lv_scr_act(), lv_color_hex(0xf1f1f1), 0);
  lv_obj_set_style_bg_opa(lv_scr_act(), LV_OPA_COVER, 0);

  title_label = lv_label_create(lv_scr_act());
  lv_label_set_text(title_label, "WiFi Web Server");
  lv_obj_set_style_text_color(title_label, lv_color_hex(0x1f9436), 0);
  lv_obj_set_style_text_font(title_label, &lv_font_montserrat_20, 0);
  lv_obj_align(title_label, LV_ALIGN_TOP_MID, 0, 12);

  mode_label = lv_label_create(lv_scr_act());
  lv_obj_set_style_text_color(mode_label, lv_color_hex(0x050505), 0);
  lv_obj_align(mode_label, LV_ALIGN_TOP_LEFT, 10, 44);

  ip_label = lv_label_create(lv_scr_act());
  lv_obj_set_style_text_color(ip_label, lv_color_hex(0x1f9436), 0);
  lv_obj_align(ip_label, LV_ALIGN_TOP_LEFT, 10, 68);

  gpio_left_label = lv_label_create(lv_scr_act());
  lv_obj_set_style_text_color(gpio_left_label, lv_color_hex(0x050505), 0);
  lv_obj_set_style_text_font(gpio_left_label, &lv_font_montserrat_14, 0);
  lv_obj_align(gpio_left_label, LV_ALIGN_TOP_LEFT, 10, 94);

  gpio_right_label = lv_label_create(lv_scr_act());
  lv_obj_set_style_text_color(gpio_right_label, lv_color_hex(0x050505), 0);
  lv_obj_set_style_text_font(gpio_right_label, &lv_font_montserrat_14, 0);
  lv_obj_align(gpio_right_label, LV_ALIGN_TOP_LEFT, 126, 94);

  led_label = lv_label_create(lv_scr_act());
  lv_obj_set_style_text_color(led_label, lv_color_hex(0xf472b6), 0);
  lv_obj_set_style_text_font(led_label, &lv_font_montserrat_14, 0);
  lv_obj_align(led_label, LV_ALIGN_BOTTOM_LEFT, 10, -18);

  lv_timer_create(animate_led_label, 60, nullptr);

  refresh_screen();
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("04 WiFi Web Server");

  for (size_t i = 0; i < CONTROL_PIN_COUNT; ++i) {
    gpio_states[i].pin = CONTROL_PINS[i];
    gpio_states[i].level = false;
    pinMode(gpio_states[i].pin, OUTPUT);
    digitalWrite(gpio_states[i].pin, LOW);
  }

  pinMode(RGB_LED_PIN, OUTPUT);
  set_rgb_led(0, 0, 0);

  screen.init();
  screen.setBacklight(SCREEN_BACKLIGHT_BRIGHTNESS);
  create_ui();
  setup_wifi();
  refresh_screen();

  server.on("/", handle_root);
  server.on("/api/status", handle_status);
  server.on("/api/gpio", handle_gpio);
  server.on("/api/led", handle_led);
  server.begin();

  Serial.println(wifi_mode_text);
  Serial.println(wifi_ip_text);
}

void loop() {
  server.handleClient();
  screen.routine();
  delay(5);
}
