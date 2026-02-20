#include <Arduino.h>
#include "constants.h"
#include "display_manager.h"
#include "splash_screen.h"
#include "touch_handler.h"
#include "settings_manager.h"

static uint32_t lastTouchPoll = 0;

void setup() {
    Serial.begin(115200);
    Serial.println("Osmosis v1.0.0 booting...");

    display.init();
    splash::show();
    touch.init();

    settingsMgr.init();
    display.setBrightnessLevel(settingsMgr.settings().brightness);
}

void loop() {
    uint32_t now = millis();

    // Poll touch at 50Hz
    if (now - lastTouchPoll >= TOUCH_POLL_MS) {
        lastTouchPoll = now;

        GestureType g = touch.update();
        if (g == GESTURE_LONG_PRESS) {
            Serial.println("[touch] long press detected");
        } else if (g == GESTURE_TAP) {
            TouchPoint p = touch.getLastTap();
            Serial.printf("[touch] tap at (%d, %d)\n", p.x, p.y);
        }
    }
}
