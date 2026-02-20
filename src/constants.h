#pragma once
#include <cstdint>

// === Firmware ===
constexpr const char* FW_NAME    = "Osmosis";
constexpr const char* FW_VERSION = "1.0.0";

// === Pin Definitions ===
constexpr int PIN_TFT_BL = 21;

// Touch HSPI pins
constexpr int PIN_TOUCH_CS   = 33;
constexpr int PIN_TOUCH_CLK  = 25;
constexpr int PIN_TOUCH_MOSI = 32;
constexpr int PIN_TOUCH_MISO = 39;

// === Display Layout (Portrait) ===
constexpr int SCREEN_W = 240;
constexpr int SCREEN_H = 320;
constexpr int STRIP_H  = 10;
constexpr int NUM_STRIPS = SCREEN_H / STRIP_H;  // 32

// Header bar
constexpr int HEADER_H = 30;

// Image area
constexpr int IMG_W = 120;
constexpr int IMG_H = 120;
constexpr int IMG_X = (SCREEN_W - IMG_W) / 2;  // 60
constexpr int IMG_Y = HEADER_H + 15;            // 45

// Word box
constexpr int WORD_BOX_Y = IMG_Y + IMG_H + 15;  // 180
constexpr int WORD_BOX_W = 200;
constexpr int WORD_BOX_H = 70;
constexpr int WORD_BOX_X = (SCREEN_W - WORD_BOX_W) / 2;  // 20

// Card counter
constexpr int COUNTER_Y = SCREEN_H - 20;  // 300

// === Backlight PWM ===
constexpr int BL_PWM_CHANNEL = 0;
constexpr int BL_PWM_FREQ    = 5000;
constexpr int BL_PWM_RES     = 8;
constexpr uint8_t BRIGHTNESS_LEVELS[] = {50, 150, 255};
constexpr int NUM_BRIGHTNESS = 3;

// === Touch Calibration ===
constexpr int TOUCH_X_MIN = 300;
constexpr int TOUCH_X_MAX = 3800;
constexpr int TOUCH_Y_MIN = 300;
constexpr int TOUCH_Y_MAX = 3800;
constexpr unsigned long LONG_PRESS_MS = 2000;

// === Color Palette (RGB565) ===
constexpr uint16_t CLR_BG_DARK       = 0x0000;
constexpr uint16_t CLR_BG_CARD       = 0x0861;
constexpr uint16_t CLR_HEADER_BG     = 0x1269;
constexpr uint16_t CLR_ACCENT        = 0x4E1F;
constexpr uint16_t CLR_TEXT_PRIMARY   = 0xFFFF;
constexpr uint16_t CLR_TEXT_SECONDARY = 0x8C51;
constexpr uint16_t CLR_TEXT_DIM       = 0x4208;
constexpr uint16_t CLR_WORD_BOX_BG   = 0x0882;
constexpr uint16_t CLR_WORD_BOX_BORDER = 0x4E1F;
constexpr uint16_t CLR_SPLASH_GREEN  = 0x06D5;
constexpr uint16_t CLR_BTN_ACTIVE    = 0x4E1F;
constexpr uint16_t CLR_BTN_INACTIVE  = 0x2104;

// === Settings Defaults ===
constexpr uint8_t DEFAULT_WORDS_PER_DAY  = 15;
constexpr uint16_t DEFAULT_DISPLAY_SECS  = 60;
constexpr uint8_t DEFAULT_BRIGHTNESS     = 1;

// === Timing ===
constexpr unsigned long TOUCH_POLL_MS = 20;
constexpr unsigned long SPLASH_DURATION_MS = 3000;
