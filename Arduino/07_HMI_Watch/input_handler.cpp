/**
 * @file input_handler.cpp
 * @brief Abstract Implementation of Input Events
 */

#include "input_handler.h"

#include <Arduino.h>

// This module turns the BOOT button into single/double/triple click events.
// It also accepts serial debug input so page behavior can be tested quickly.

static const uint8_t BOOT_BUTTON_PIN = 0;
static const uint32_t BUTTON_DEBOUNCE_MS = 25;
static const uint32_t MULTI_CLICK_TIMEOUT_MS = 320;

static InputEvent g_pending_event = INPUT_EVENT_NONE;
static bool g_button_raw_state = HIGH;
static bool g_button_stable_state = HIGH;
static uint32_t g_button_last_change_ms = 0;
static uint32_t g_last_release_ms = 0;
static uint8_t g_pending_clicks = 0;

static InputEvent click_count_to_event(uint8_t click_count) {
    if (click_count == 1) {
        return INPUT_EVENT_SINGLE_CLICK;
    }
    if (click_count == 2) {
        return INPUT_EVENT_DOUBLE_CLICK;
    }
    if (click_count >= 3) {
        return INPUT_EVENT_TRIPLE_CLICK;
    }
    return INPUT_EVENT_NONE;
}

static bool serial_char_to_event(char cmd, InputEvent* out_event) {
    if (out_event == nullptr) {
        return false;
    }

    switch (cmd) {
        case '1':
            *out_event = INPUT_EVENT_SINGLE_CLICK;
            return true;
        case '2':
            *out_event = INPUT_EVENT_DOUBLE_CLICK;
            return true;
        case '3':
            *out_event = INPUT_EVENT_TRIPLE_CLICK;
            return true;
        default:
            *out_event = INPUT_EVENT_NONE;
            return false;
    }
}

static void poll_button(void) {
    const uint32_t now = millis();
    const bool raw_state = digitalRead(BOOT_BUTTON_PIN);

    if (raw_state != g_button_raw_state) {
        g_button_raw_state = raw_state;
        g_button_last_change_ms = now;
    }

    if ((now - g_button_last_change_ms) < BUTTON_DEBOUNCE_MS) {
        return;
    }

    if (g_button_stable_state == g_button_raw_state) {
        return;
    }

    g_button_stable_state = g_button_raw_state;
    // Count clicks on button release so a press does not generate repeated events.
    if (g_button_stable_state == LOW) {
        return;
    }

    g_pending_clicks++;
    g_last_release_ms = now;
}

static void process_pending_clicks(void) {
    if (g_pending_clicks == 0) {
        return;
    }

    if ((millis() - g_last_release_ms) < MULTI_CLICK_TIMEOUT_MS) {
        return;
    }

    // Wait a short window so one click can still become a double or triple click.
    g_pending_event = click_count_to_event(g_pending_clicks);
    g_pending_clicks = 0;
}

void input_handler_init(void) {
    g_pending_event = INPUT_EVENT_NONE;
    pinMode(BOOT_BUTTON_PIN, INPUT_PULLUP);
    g_button_raw_state = digitalRead(BOOT_BUTTON_PIN);
    g_button_stable_state = g_button_raw_state;
    g_button_last_change_ms = millis();
    g_last_release_ms = 0;
    g_pending_clicks = 0;
}

bool input_handler_peek_serial_debug_char(char* out_char) {
    while (Serial.available() > 0) {
        const char cmd = static_cast<char>(Serial.peek());
        if (cmd == '\r' || cmd == '\n') {
            Serial.read();
            continue;
        }

        InputEvent event = INPUT_EVENT_NONE;
        if (serial_char_to_event(cmd, &event)) {
            return false;
        }

        if (out_char != nullptr) {
            *out_char = cmd;
        }
        return true;
    }

    return false;
}

void input_handler_consume_serial_debug_char(void) {
    if (Serial.available() > 0) {
        Serial.read();
    }
}

InputEvent input_handler_poll(void) {
    poll_button();
    process_pending_clicks();

    if (g_pending_event != INPUT_EVENT_NONE) {
        const InputEvent event = g_pending_event;
        g_pending_event = INPUT_EVENT_NONE;
        return event;
    }

    while (Serial.available() > 0) {
        const char cmd = static_cast<char>(Serial.read());
        if (cmd == '\r' || cmd == '\n') {
            continue;
        }

        InputEvent event = INPUT_EVENT_NONE;
        if (serial_char_to_event(cmd, &event)) {
            return event;
        }

        break;
    }

    return INPUT_EVENT_NONE;
}
