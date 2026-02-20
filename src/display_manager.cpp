#include "display_manager.h"

DisplayManager display;

void DisplayManager::init() {
    // Initialize backlight PWM
    ledcSetup(BL_PWM_CHANNEL, BL_PWM_FREQ, BL_PWM_RES);
    ledcAttachPin(PIN_TFT_BL, BL_PWM_CHANNEL);
    ledcWrite(BL_PWM_CHANNEL, BRIGHTNESS_LEVELS[DEFAULT_BRIGHTNESS]);

    // Initialize TFT
    _tft.init();
    _tft.setRotation(0);  // Portrait mode
    _tft.fillScreen(CLR_BG_DARK);

    // Create strip sprite for rendering
    _strip.setColorDepth(16);
    _strip.createSprite(SCREEN_W, STRIP_H);
}

void DisplayManager::setBrightness(uint8_t level) {
    ledcWrite(BL_PWM_CHANNEL, level);
}

void DisplayManager::setBrightnessLevel(uint8_t idx) {
    if (idx >= NUM_BRIGHTNESS) idx = NUM_BRIGHTNESS - 1;
    _brightnessIdx = idx;
    setBrightness(BRIGHTNESS_LEVELS[idx]);
}

uint8_t DisplayManager::getBrightnessLevel() const {
    return _brightnessIdx;
}

TFT_eSprite& DisplayManager::getStrip() {
    return _strip;
}

void DisplayManager::pushStrip(int y) {
    _strip.pushSprite(0, y);
}

uint16_t DisplayManager::blendColor(uint16_t c1, uint16_t c2, float ratio) {
    if (ratio <= 0.0f) return c1;
    if (ratio >= 1.0f) return c2;
    uint8_t r1 = (c1 >> 11) & 0x1F, g1 = (c1 >> 5) & 0x3F, b1 = c1 & 0x1F;
    uint8_t r2 = (c2 >> 11) & 0x1F, g2 = (c2 >> 5) & 0x3F, b2 = c2 & 0x1F;
    uint8_t r = r1 + (int)((r2 - r1) * ratio);
    uint8_t g = g1 + (int)((g2 - g1) * ratio);
    uint8_t b = b1 + (int)((b2 - b1) * ratio);
    return (r << 11) | (g << 5) | b;
}

uint16_t DisplayManager::dimColor(uint16_t color, float factor) {
    uint8_t r = ((color >> 11) & 0x1F) * factor;
    uint8_t g = ((color >> 5) & 0x3F) * factor;
    uint8_t b = (color & 0x1F) * factor;
    return (r << 11) | (g << 5) | b;
}
