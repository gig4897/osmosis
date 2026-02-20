#include <Arduino.h>
#include <SPIFFS.h>
#include "constants.h"
#include "display_manager.h"
#include "splash_screen.h"
#include "touch_handler.h"
#include "settings_manager.h"
#include "card_manager.h"
#include "card_screen.h"
#include "ui_settings.h"
#include "image_renderer.h"

static uint32_t lastTouchPoll = 0;
static uint32_t lastRender = 0;
static bool needsRender = true;

void setup() {
    Serial.begin(115200);
    Serial.println("Osmosis v1.0.0 booting...");

    // 1. Init display and show splash
    display.init();

    // 2. Init SPIFFS for images
    if (!SPIFFS.begin(true)) {
        Serial.println("SPIFFS mount failed!");
    }

    // 3. Show splash screen
    splash::show();

    // 4. Init touch
    touch.init();

    // 5. Load settings
    settingsMgr.init();
    display.setBrightnessLevel(settingsMgr.settings().brightness);

    // 6. Init card manager (loads progress, builds batch, preloads first image)
    cardMgr.init();

    // 7. Ready for first render
    needsRender = true;
    Serial.println("Osmosis ready!");
}

void loop() {
    uint32_t now = millis();

    // Touch polling at 50Hz
    if (now - lastTouchPoll >= TOUCH_POLL_MS) {
        lastTouchPoll = now;
        GestureType gesture = touch.update();

        if (gesture == GESTURE_LONG_PRESS && !settingsUI.isActive()) {
            // Open settings menu
            settingsUI.show();
            needsRender = true;
            Serial.println("[ui] Settings opened");
        } else if (gesture == GESTURE_TAP && settingsUI.isActive()) {
            // Handle tap in settings
            bool changed = settingsUI.handleTap(touch.getLastTap());
            if (changed) {
                needsRender = true;
            }
            if (!settingsUI.isActive()) {
                // Returned from settings
                needsRender = true;
                Serial.println("[ui] Settings closed");
            }
        } else if (gesture == GESTURE_LONG_PRESS && settingsUI.isActive()) {
            // Long press while settings open = close settings
            settingsUI.hide();
            settingsMgr.save();
            needsRender = true;
            Serial.println("[ui] Settings closed (long press)");
        }
    }

    // Card rotation (only when not in settings)
    if (!settingsUI.isActive()) {
        bool cardChanged = cardMgr.update();
        if (cardChanged) needsRender = true;
        cardMgr.checkDayChange();
    }

    // Render at 1Hz or on demand
    if (needsRender || now - lastRender > 1000) {
        if (settingsUI.isActive()) {
            // Render settings screen
            TFT_eSprite& spr = display.getStrip();
            for (int strip = 0; strip < NUM_STRIPS; strip++) {
                int stripY = strip * STRIP_H;
                spr.fillSprite(CLR_BG_DARK);
                settingsUI.draw(spr, stripY);
                display.pushStrip(stripY);
            }
        } else {
            // Render flashcard screen
            cardScreen::render();
        }
        lastRender = now;
        needsRender = false;
    }
}
