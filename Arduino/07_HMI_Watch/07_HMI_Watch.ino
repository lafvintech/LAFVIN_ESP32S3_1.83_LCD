/**
 * @file main.ino
 * @brief Arduino Main File - 1.83-inch ST7789 UI Framework
 */

#include <Arduino.h>
#include <WiFi.h>
#include <lvgl.h>
#include <time.h>

// This demo uses the main Arduino thread for LVGL and input handling.
// Network work is queued to a background task so the UI stays responsive.

#include "display.h"
#include "input_handler.h"
#include "menu.h"
#include "page_manager.h"
#include "rgb_control_page.h"
#include "watchface.h"
#include "weather_client.h"
#include "weather_page.h"

const char* WIFI_SSID = "YourWiFiSSID";
const char* WIFI_PASSWORD = "YourWiFiPassword";

const lv_font_t ui_font_text = lv_font_montserrat_16;
const lv_font_t ui_font_icon = lv_font_montserrat_14;
const lv_font_t ui_font_large_icon = lv_font_montserrat_24;

static Display g_display;
static page_manager_t g_page_manager;
static WeatherData g_weather_data = {};
static unsigned long g_next_weather_update_due = 0;
static const unsigned long WEATHER_UPDATE_INTERVAL = 10UL * 60UL * 1000UL;
static const unsigned long WEATHER_RETRY_INTERVAL = 30UL * 1000UL;
static unsigned long g_last_time_update = 0;
static bool g_weather_demo_loaded = false;
static bool g_weather_has_real_data = false;
static bool g_wifi_connect_started = false;
static bool g_ntp_configured = false;
static wl_status_t g_last_wifi_status = WL_IDLE_STATUS;
static unsigned long g_next_wifi_retry_due = 0;
static const unsigned long WIFI_RETRY_INTERVAL = 15UL * 1000UL;
static TaskHandle_t g_weather_task_handle = nullptr;
static portMUX_TYPE g_weather_lock = portMUX_INITIALIZER_UNLOCKED;
static volatile bool g_weather_fetch_requested = false;
static volatile bool g_weather_fetch_in_progress = false;
static volatile bool g_weather_result_ready = false;
static volatile bool g_weather_result_success = false;

static void weather_task(void* parameter);

void print_serial_help() {
    Serial.println();
    Serial.println("=== Serial Input Debug ===");
    Serial.println("1  -> single click");
    Serial.println("2  -> double click");
    Serial.println("3  -> triple click");
    Serial.println("w  -> watchface");
    Serial.println("m  -> menu");
    Serial.println("t  -> weather");
    Serial.println("r  -> backlight page");
    Serial.println("d  -> rgb page");
    Serial.println("h  -> help");
    Serial.println();
}

void begin_wifi_connect() {
    UI_LOGI("Connecting to WiFi: %s", WIFI_SSID);
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    g_wifi_connect_started = true;
    g_next_wifi_retry_due = millis() + WIFI_RETRY_INTERVAL;
}

void poll_wifi_connection(unsigned long now) {
    const wl_status_t current_status = WiFi.status();

    if (current_status != g_last_wifi_status) {
        g_last_wifi_status = current_status;

        if (current_status == WL_CONNECTED) {
            UI_LOGI("WiFi connected, IP: %s", WiFi.localIP().toString().c_str());
            g_ntp_configured = false;
        } else {
            UI_LOGW("WiFi status changed: %d", static_cast<int>(current_status));
        }
    }

    if (current_status == WL_CONNECTED) {
        // Start SNTP only once after the station is connected.
        if (!g_ntp_configured) {
            configTime(8 * 3600, 0, "ntp.ntsc.ac.cn", "cn.pool.ntp.org");
            g_ntp_configured = true;
            UI_LOGI("NTP sync started");
        }
        return;
    }

    if (!g_weather_has_real_data && !g_weather_demo_loaded) {
        weather_page_update_demo();
        g_weather_demo_loaded = true;
    }

    if (!g_wifi_connect_started) {
        begin_wifi_connect();
        return;
    }

    if (now >= g_next_wifi_retry_due) {
        UI_LOGI("Retrying WiFi connection...");
        WiFi.disconnect();
        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        g_next_wifi_retry_due = now + WIFI_RETRY_INTERVAL;
    }
}

bool update_time_display() {
    if (!g_ntp_configured) {
        return false;
    }

    struct tm timeinfo;
    // Keep the timeout short so the watchface never stalls while waiting for time sync.
    if (!getLocalTime(&timeinfo, 10)) {
        return false;
    }

    char time_str[16];
    strftime(time_str, sizeof(time_str), "%H:%M", &timeinfo);
    watchface_update_time(time_str);

    char date_str[64];
    const char* weekdays[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    snprintf(date_str, sizeof(date_str), "%04d-%02d-%02d %s",
             timeinfo.tm_year + 1900,
             timeinfo.tm_mon + 1,
             timeinfo.tm_mday,
             weekdays[timeinfo.tm_wday]);
    watchface_update_date(date_str);
    return true;
}

void request_weather_update() {
    if (WiFi.status() != WL_CONNECTED) {
        if (!g_weather_has_real_data && !g_weather_demo_loaded) {
            weather_page_update_demo();
            g_weather_demo_loaded = true;
        }
        UI_LOGW("Cannot update weather: WiFi not connected");
        g_next_weather_update_due = millis() + WEATHER_RETRY_INTERVAL;
        return;
    }

    if (g_weather_fetch_in_progress || g_weather_fetch_requested) {
        return;
    }

    // The actual HTTP request runs in a FreeRTOS task, not in the LVGL loop.
    UI_LOGI("Queueing weather update...");
    g_weather_fetch_requested = true;
}

void process_weather_result(unsigned long now) {
    if (!g_weather_result_ready) {
        return;
    }

    WeatherData latest_weather = {};
    bool fetch_success = false;

    // Copy the latest result locally before updating LVGL objects on the main thread.
    portENTER_CRITICAL(&g_weather_lock);
    latest_weather = g_weather_data;
    fetch_success = g_weather_result_success;
    g_weather_result_ready = false;
    portEXIT_CRITICAL(&g_weather_lock);

    if (fetch_success) {
        weather_page_update(&latest_weather);
        g_weather_demo_loaded = false;
        g_weather_has_real_data = true;
        g_next_weather_update_due = now + WEATHER_UPDATE_INTERVAL;
        UI_LOGI("Weather UI updated");
    } else {
        UI_LOGW("Weather update failed");
        if (!g_weather_has_real_data && !g_weather_demo_loaded) {
            weather_page_update_demo();
            g_weather_demo_loaded = true;
        }
        g_next_weather_update_due = now + WEATHER_RETRY_INTERVAL;
    }
}

void handle_serial_debug_command() {
    char cmd = '\0';
    if (!input_handler_peek_serial_debug_char(&cmd)) {
        return;
    }

    input_handler_consume_serial_debug_char();

    switch (cmd) {
        case 'w':
        case 'W':
            page_manager_back_to_watchface(&g_page_manager);
            UI_LOGI("Serial debug: watchface");
            break;
        case 'm':
        case 'M':
            page_manager_switch_to(&g_page_manager, PAGE_MENU, false);
            UI_LOGI("Serial debug: menu");
            break;
        case 't':
        case 'T':
            page_manager_switch_to(&g_page_manager, PAGE_WEATHER, false);
            UI_LOGI("Serial debug: weather");
            break;
        case 'r':
        case 'R':
            page_manager_switch_to(&g_page_manager, PAGE_RGB_CONTROL, false);
            UI_LOGI("Serial debug: backlight");
            break;
        case 'd':
        case 'D':
            page_manager_switch_to(&g_page_manager, PAGE_RGB_LED, false);
            UI_LOGI("Serial debug: rgb led");
            break;
        case 'h':
        case 'H':
        case '?':
            print_serial_help();
            break;
        default:
            UI_LOGW("Unknown serial debug command: %c", cmd);
            print_serial_help();
            break;
    }
}

static void apply_rgb_control_backlight() {
    g_display.setBacklight(rgb_control_page_get_backlight());
}

static void restore_default_backlight() {
    g_display.setBacklight(100);
}

static void dispatch_input_event(InputEvent event) {
    const int current_page = page_manager_get_current(&g_page_manager);

    switch (current_page) {
        case PAGE_WATCHFACE:
            if (event == INPUT_EVENT_SINGLE_CLICK || event == INPUT_EVENT_DOUBLE_CLICK) {
                page_manager_next_from_watchface(&g_page_manager);
            }
            break;
        case PAGE_MENU:
            if (event == INPUT_EVENT_SINGLE_CLICK) {
                menu_select_next();
            } else if (event == INPUT_EVENT_DOUBLE_CLICK) {
                menu_activate_selected();
            } else if (event == INPUT_EVENT_TRIPLE_CLICK) {
                page_manager_back_to_watchface(&g_page_manager);
            }
            break;
        case PAGE_WEATHER:
            if (event == INPUT_EVENT_TRIPLE_CLICK) {
                page_manager_switch_to(&g_page_manager, PAGE_MENU, true);
            }
            break;
        case PAGE_RGB_CONTROL:
            if (event == INPUT_EVENT_SINGLE_CLICK) {
                rgb_control_page_handle_single_click(PAGE_RGB_CONTROL);
                apply_rgb_control_backlight();
            } else if (event == INPUT_EVENT_DOUBLE_CLICK) {
                rgb_control_page_handle_double_click(PAGE_RGB_CONTROL);
                apply_rgb_control_backlight();
            } else if (event == INPUT_EVENT_TRIPLE_CLICK) {
                restore_default_backlight();
                page_manager_switch_to(&g_page_manager, PAGE_MENU, true);
            }
            break;
        case PAGE_RGB_LED:
            if (event == INPUT_EVENT_SINGLE_CLICK) {
                rgb_control_page_handle_single_click(PAGE_RGB_LED);
            } else if (event == INPUT_EVENT_DOUBLE_CLICK) {
                rgb_control_page_handle_double_click(PAGE_RGB_LED);
            } else if (event == INPUT_EVENT_TRIPLE_CLICK) {
                rgb_control_page_turn_off_rgb_led();
                page_manager_switch_to(&g_page_manager, PAGE_MENU, true);
            }
            break;
        default:
            break;
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    UI_LOGI("=== Watch UI Starting ===");

    input_handler_init();
    g_display.init();
    g_display.setBacklight(100);

    if (!page_manager_init(&g_page_manager)) {
        UI_LOGE("Page manager init failed!");
        return;
    }

    watchface_update_steps(0);
    weather_page_update_demo();
    g_weather_demo_loaded = true;
    begin_wifi_connect();
    g_next_weather_update_due = millis();
    // Weather fetching is blocking, so it is isolated in a background task.
    xTaskCreate(weather_task, "weather_task", 8192, nullptr, 1, &g_weather_task_handle);

    update_time_display();
    print_serial_help();
    UI_LOGI("=== Setup Complete ===");
}

void loop() {
    g_display.routine();

    const unsigned long now = millis();
    // Fast UI work stays here. Network retries and results are polled in small steps.
    poll_wifi_connection(now);
    process_weather_result(now);

    if (now - g_last_time_update >= 1000) {
        update_time_display();
        g_last_time_update = now;
    }

    if (now >= g_next_weather_update_due) {
        request_weather_update();
    }

    static int demo_steps = 0;
    static unsigned long last_step_update = 0;
    if (now - last_step_update >= 5000) {
        demo_steps += random(3, 8);
        watchface_update_steps(demo_steps);
        last_step_update = now;
    }

    handle_serial_debug_command();

    const InputEvent input_event = input_handler_poll();
    if (input_event != INPUT_EVENT_NONE) {
        dispatch_input_event(input_event);
    }

    delay(5);
}

static void weather_task(void* parameter) {
    LV_UNUSED(parameter);

    for (;;) {
        if (!g_weather_fetch_requested) {
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        g_weather_fetch_requested = false;
        g_weather_fetch_in_progress = true;

        WeatherData fetched_weather = {};
        UI_LOGI("Updating weather data...");
        // Blocking HTTP is safe here because this task is separate from the UI loop.
        const bool fetch_success = weather_client_fetch(&fetched_weather);

        portENTER_CRITICAL(&g_weather_lock);
        if (fetch_success) {
            g_weather_data = fetched_weather;
        }
        g_weather_result_success = fetch_success;
        g_weather_result_ready = true;
        portEXIT_CRITICAL(&g_weather_lock);

        g_weather_fetch_in_progress = false;
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
