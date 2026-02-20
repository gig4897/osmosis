#include "splash_screen.h"

void splash::show() {
    TFT_eSPI& tft = display.tft();
    unsigned long startTime = millis();

    tft.fillScreen(CLR_BG_DARK);

    // Background gradient: vertical bands blending from dark blue (center) to black (edges)
    uint16_t darkBlue = 0x0011;  // Very dark navy
    for (int x = 0; x < SCREEN_W; x++) {
        // Distance from center (0.0 at center, 1.0 at edges)
        float dist = fabs((float)(x - SCREEN_W / 2)) / (float)(SCREEN_W / 2);
        uint16_t col = DisplayManager::blendColor(darkBlue, CLR_BG_DARK, dist);
        tft.drawFastVLine(x, 0, SCREEN_H, col);
    }

    // "OSMOSIS" centered at y=110, Font 6
    tft.setTextDatum(TC_DATUM);
    tft.setTextColor(CLR_SPLASH_GREEN);
    tft.drawString("OSMOSIS", SCREEN_W / 2, 110, 6);

    // "LEARN BY IMMERSION" at y=155, Font 2, dimmed green (50%)
    uint16_t dimGreen = DisplayManager::dimColor(CLR_SPLASH_GREEN, 0.5f);
    tft.setTextColor(dimGreen);
    tft.drawString("LEARN BY IMMERSION", SCREEN_W / 2, 155, 2);

    // Three animated dots at y=200 with 400ms delay between each
    tft.setTextColor(CLR_SPLASH_GREEN);
    const int dotY = 200;
    const int dotSpacing = 20;
    const int dotStartX = SCREEN_W / 2 - dotSpacing;

    for (int i = 0; i < 3; i++) {
        int dotX = dotStartX + i * dotSpacing;
        tft.fillCircle(dotX, dotY, 4, CLR_SPLASH_GREEN);
        delay(400);
    }

    // "v1.0 - Spanish" at y=260, Font 2
    tft.setTextColor(CLR_TEXT_DIM);
    tft.drawString("v1.0 - Spanish", SCREEN_W / 2, 260, 2);

    // "by VCodeworks LLC" at y=280, Font 2, dimmed green (50%)
    tft.setTextColor(dimGreen);
    tft.drawString("by VCodeworks LLC", SCREEN_W / 2, 280, 2);

    // Hold for remaining time up to SPLASH_DURATION_MS
    unsigned long elapsed = millis() - startTime;
    if (elapsed < SPLASH_DURATION_MS) {
        delay(SPLASH_DURATION_MS - elapsed);
    }
}
