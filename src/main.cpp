#include <Arduino.h>
#include <SPIFFS.h>
#include <Esp.h>
#include "constants.h"
#include "display_manager.h"
#include "splash_screen.h"
#include "touch_handler.h"
#include "settings_manager.h"
#include "card_manager.h"
#include "card_screen.h"
#include "ui_settings.h"
#include "image_renderer.h"
#include "wifi_manager.h"
#include "pack_manager.h"
#include "vocab_loader.h"

static uint32_t lastTouchPoll = 0;
static uint32_t lastRender = 0;
static bool needsRender = true;
static bool packInstalled = false;

enum class AppState : uint8_t {
    Cards,          // Normal flashcard display
    Settings,       // Settings menu
    NoPack,         // No pack installed — shows setup prompt
    Downloading     // Pack download in progress
};

static AppState appState = AppState::NoPack;

void setup() {
    Serial.begin(115200);
    Serial.println("Osmosis v2.0.0 booting...");

    display.init();
    delay(100);

    if (!SPIFFS.begin(true)) {
        Serial.println("SPIFFS mount failed!");
    }

    splash::show();
    touch.init();
    settingsMgr.init();
    display.setBrightnessLevel(settingsMgr.settings().brightness);

    // Init WiFi (non-blocking connect attempt)
    wifiMgr::init();

    // Check for installed pack
    bool hasManifest = packMgr::hasInstalledPack();
    Serial.printf("[boot] SPIFFS manifest.json exists: %s\n", hasManifest ? "YES" : "NO");
    Serial.printf("[boot] Settings installedLang: '%s'\n", settingsMgr.settings().installedLang);
    Serial.printf("[boot] Free heap: %u\n", ESP.getFreeHeap());

    if (hasManifest) {
        bool loaded = vocabLoader::load();
        Serial.printf("[boot] vocabLoader::load() = %s\n", loaded ? "OK" : "FAIL");

        if (loaded) {
            packInstalled = true;
            cardMgr.init();
            cardScreen::init();
            appState = AppState::Cards;
            Serial.println("[boot] Pack loaded, entering card mode");
        } else {
            packInstalled = false;
            appState = AppState::NoPack;
            Serial.println("[boot] Manifest exists but vocab load failed, removing corrupt file");
            SPIFFS.remove("/manifest.json");
        }
    } else {
        packInstalled = false;
        appState = AppState::NoPack;
        Serial.println("[boot] No pack installed, entering setup mode");
    }

    needsRender = true;
    Serial.println("Osmosis ready!");
}

// Render the "No Pack Installed" screen using strip-based rendering
static void renderNoPack() {
    for (int strip = 0; strip < NUM_STRIPS; strip++) {
        int stripY = strip * STRIP_H;
        TFT_eSprite& spr = display.getStrip();
        spr.fillSprite(CLR_BG_DARK);

        // Header band
        if (stripY < HEADER_H) {
            int hEnd = HEADER_H - stripY;
            if (hEnd > STRIP_H) hEnd = STRIP_H;
            spr.fillRect(0, 0, SCREEN_W, hEnd, CLR_HEADER_BG);
        }

        // "OSMOSIS" title
        {
            int y = 10 - stripY;
            if (y >= -18 && y < STRIP_H) {
                spr.setTextDatum(TC_DATUM);
                spr.setTextColor(CLR_ACCENT, CLR_HEADER_BG);
                spr.drawString("OSMOSIS", SCREEN_W / 2, y, 2);
            }
        }

        // "No Language Pack"
        {
            int y = 100 - stripY;
            if (y >= -26 && y < STRIP_H) {
                spr.setTextDatum(TC_DATUM);
                spr.setTextColor(CLR_TEXT_PRIMARY, CLR_BG_DARK);
                spr.drawString("No Language Pack", SCREEN_W / 2, y, 4);
            }
        }

        // Instructions
        {
            int y = 150 - stripY;
            if (y >= -16 && y < STRIP_H) {
                spr.setTextDatum(TC_DATUM);
                spr.setTextColor(CLR_TEXT_SECONDARY, CLR_BG_DARK);
                spr.drawString("Connect to WiFi to", SCREEN_W / 2, y, 2);
            }
        }
        {
            int y = 170 - stripY;
            if (y >= -16 && y < STRIP_H) {
                spr.setTextDatum(TC_DATUM);
                spr.setTextColor(CLR_TEXT_SECONDARY, CLR_BG_DARK);
                spr.drawString("download a language", SCREEN_W / 2, y, 2);
            }
        }

        // "Long press for settings"
        {
            int y = 220 - stripY;
            if (y >= -16 && y < STRIP_H) {
                spr.setTextDatum(TC_DATUM);
                spr.setTextColor(CLR_TEXT_DIM, CLR_BG_DARK);
                spr.drawString("Long press for settings", SCREEN_W / 2, y, 2);
            }
        }

        display.pushStrip(stripY);
    }
}

void loop() {
    uint32_t now = millis();

    // WiFi manager update (handles captive portal, connection state)
    wifiMgr::update();

    // Touch polling at 50Hz
    if (now - lastTouchPoll >= TOUCH_POLL_MS) {
        lastTouchPoll = now;
        GestureType gesture = touch.update();

        switch (appState) {
            case AppState::Cards:
                if (gesture == GESTURE_TAP && !settingsUI.isActive()) {
                    cardMgr.nextCard();
                    needsRender = true;
                } else if (gesture == GESTURE_LONG_PRESS && !settingsUI.isActive()) {
                    settingsUI.show();
                    appState = AppState::Settings;
                    needsRender = true;
                }
                break;

            case AppState::NoPack:
                if (gesture == GESTURE_LONG_PRESS) {
                    settingsUI.show();
                    appState = AppState::Settings;
                    needsRender = true;
                }
                break;

            case AppState::Settings:
                if (gesture == GESTURE_TAP) {
                    bool changed = settingsUI.handleTap(touch.getLastTap());
                    if (changed) needsRender = true;

                    // Check if settings closed (via close button or back)
                    if (!settingsUI.isActive()) {
                        appState = packInstalled ? AppState::Cards : AppState::NoPack;
                        needsRender = true;
                    }
                } else if (gesture == GESTURE_LONG_PRESS) {
                    settingsUI.hide();
                    settingsMgr.save();
                    appState = packInstalled ? AppState::Cards : AppState::NoPack;
                    needsRender = true;
                }
                break;

            case AppState::Downloading:
                needsRender = true;  // Keep refreshing progress
                break;
        }
    }

    // (Download completion is handled by ESP.restart() in ui_settings.cpp)

    // Card rotation (only in card mode)
    if (appState == AppState::Cards && !settingsUI.isActive()) {
        bool cardChanged = cardMgr.update();
        if (cardChanged) needsRender = true;
        cardMgr.checkDayChange();
    }

    // Render at demand or 1Hz refresh
    if (needsRender || now - lastRender > 1000) {
        switch (appState) {
            case AppState::Cards:
                cardScreen::render();
                break;

            case AppState::NoPack:
                renderNoPack();
                break;

            case AppState::Settings:
            case AppState::Downloading: {
                TFT_eSprite& spr = display.getStrip();
                for (int strip = 0; strip < NUM_STRIPS; strip++) {
                    int stripY = strip * STRIP_H;
                    spr.fillSprite(CLR_BG_DARK);
                    settingsUI.draw(spr, stripY);
                    display.pushStrip(stripY);
                }
                break;
            }
        }
        lastRender = now;
        needsRender = false;
    }
}
