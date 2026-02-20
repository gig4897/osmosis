#include "ui_settings.h"
#include "display_manager.h"
#include "settings_manager.h"
#include "constants.h"

SettingsScreen settingsUI;

// --- Words per day row ---
static const int WPD_VALUES[]   = {5, 10, 15, 20, 25};
static const char* WPD_LABELS[] = {"5", "10", "15", "20", "25"};
static const int WPD_COUNT      = 5;
static const int WPD_Y          = 55;
static const int WPD_H          = 30;
static const int WPD_W          = 40;
static const int WPD_X[]        = {10, 55, 100, 145, 190};

// --- Display time row ---
static const int DT_VALUES[]    = {30, 60, 120, 300};
static const char* DT_LABELS[]  = {"30s", "1m", "2m", "5m"};
static const int DT_COUNT       = 4;
static const int DT_Y           = 125;
static const int DT_H           = 30;
static const int DT_W           = 50;
static const int DT_X[]         = {10, 65, 120, 175};

// --- Brightness row ---
static const int BR_VALUES[]    = {0, 1, 2};
static const char* BR_LABELS[]  = {"Low", "Med", "High"};
static const int BR_COUNT       = 3;
static const int BR_Y           = 195;
static const int BR_H           = 30;
static const int BR_W           = 68;
static const int BR_X[]         = {10, 86, 162};

// --- Back button ---
static const int BACK_X = 70;
static const int BACK_Y = 270;
static const int BACK_W = 100;
static const int BACK_H = 34;

// -------------------------------------------------------
void SettingsScreen::show() {
    _active = true;
}

void SettingsScreen::hide() {
    _active = false;
}

// -------------------------------------------------------
bool SettingsScreen::hitTest(const Button& btn, TouchPoint pt) {
    return pt.x >= btn.x && pt.x < btn.x + btn.w &&
           pt.y >= btn.y && pt.y < btn.y + btn.h;
}

// -------------------------------------------------------
void SettingsScreen::drawButton(TFT_eSprite& spr, const Button& btn,
                                int stripY, const char* label, bool selected) {
    // Determine the portion of the button visible in this strip
    int btnTop    = btn.y;
    int btnBottom = btn.y + btn.h;
    int stripTop  = stripY;
    int stripBot  = stripY + STRIP_H;

    if (btnBottom <= stripTop || btnTop >= stripBot) return;  // not visible

    int drawY = btnTop - stripY;
    int drawH = btn.h;

    // Clip to strip boundaries
    if (drawY < 0) { drawH += drawY; drawY = 0; }
    if (drawY + drawH > STRIP_H) { drawH = STRIP_H - drawY; }

    uint16_t fillClr  = selected ? CLR_BTN_ACTIVE : CLR_BTN_INACTIVE;
    uint16_t textClr  = selected ? CLR_TEXT_PRIMARY : CLR_TEXT_SECONDARY;
    uint16_t borderClr = fillClr;

    spr.fillRect(btn.x, drawY, btn.w, drawH, fillClr);
    // Draw border edges that fall within this strip
    // Top edge
    if (btnTop >= stripTop && btnTop < stripBot)
        spr.drawFastHLine(btn.x, btnTop - stripY, btn.w, borderClr);
    // Bottom edge
    if (btnBottom - 1 >= stripTop && btnBottom - 1 < stripBot)
        spr.drawFastHLine(btn.x, btnBottom - 1 - stripY, btn.w, borderClr);
    // Left edge
    spr.drawFastVLine(btn.x, drawY, drawH, borderClr);
    // Right edge
    spr.drawFastVLine(btn.x + btn.w - 1, drawY, drawH, borderClr);

    // Draw label centred (only if the text baseline lands in this strip)
    int textY = btnTop + btn.h / 2;
    if (textY >= stripTop && textY < stripBot) {
        spr.setTextColor(textClr);
        spr.setTextDatum(MC_DATUM);
        spr.setTextFont(1);
        spr.drawString(label, btn.x + btn.w / 2, textY - stripY);
    }
}

// -------------------------------------------------------
void SettingsScreen::draw(TFT_eSprite& spr, int stripY) {
    int stripBot = stripY + STRIP_H;

    // Background
    spr.fillSprite(CLR_BG_DARK);

    const OsmosisSettings& s = settingsMgr.settings();

    // --- Title "SETTINGS" at y=10, Font 4, CLR_ACCENT ---
    {
        int textY = 10;
        if (textY >= stripY && textY < stripBot) {
            spr.setTextColor(CLR_ACCENT);
            spr.setTextDatum(TC_DATUM);
            spr.setTextFont(4);
            spr.drawString("SETTINGS", SCREEN_W / 2, textY - stripY);
        }
    }

    // --- Row 1 label: "Words per day" at y=42 ---
    {
        int textY = 42;
        if (textY >= stripY && textY < stripBot) {
            spr.setTextColor(CLR_TEXT_SECONDARY);
            spr.setTextDatum(TC_DATUM);
            spr.setTextFont(1);
            spr.drawString("Words per day", SCREEN_W / 2, textY - stripY);
        }
    }

    // Row 1 buttons
    for (int i = 0; i < WPD_COUNT; i++) {
        Button btn = {WPD_X[i], WPD_Y, WPD_W, WPD_H};
        bool sel = (s.wordsPerDay == WPD_VALUES[i]);
        drawButton(spr, btn, stripY, WPD_LABELS[i], sel);
    }

    // --- Row 2 label: "Display time" at y=100 ---
    {
        int textY = 100;
        if (textY >= stripY && textY < stripBot) {
            spr.setTextColor(CLR_TEXT_SECONDARY);
            spr.setTextDatum(TC_DATUM);
            spr.setTextFont(1);
            spr.drawString("Display time", SCREEN_W / 2, textY - stripY);
        }
    }

    // Row 2 buttons
    for (int i = 0; i < DT_COUNT; i++) {
        Button btn = {DT_X[i], DT_Y, DT_W, DT_H};
        bool sel = (s.displaySecs == (uint16_t)DT_VALUES[i]);
        drawButton(spr, btn, stripY, DT_LABELS[i], sel);
    }

    // --- Row 3 label: "Brightness" at y=170 ---
    {
        int textY = 170;
        if (textY >= stripY && textY < stripBot) {
            spr.setTextColor(CLR_TEXT_SECONDARY);
            spr.setTextDatum(TC_DATUM);
            spr.setTextFont(1);
            spr.drawString("Brightness", SCREEN_W / 2, textY - stripY);
        }
    }

    // Row 3 buttons
    for (int i = 0; i < BR_COUNT; i++) {
        Button btn = {BR_X[i], BR_Y, BR_W, BR_H};
        bool sel = (s.brightness == (uint8_t)BR_VALUES[i]);
        drawButton(spr, btn, stripY, BR_LABELS[i], sel);
    }

    // --- Back button at y=270 ---
    {
        Button btn = {BACK_X, BACK_Y, BACK_W, BACK_H};
        int btnTop    = btn.y;
        int btnBottom = btn.y + btn.h;

        if (btnBottom > stripY && btnTop < stripBot) {
            int drawY = btnTop - stripY;
            int drawH = btn.h;
            if (drawY < 0) { drawH += drawY; drawY = 0; }
            if (drawY + drawH > STRIP_H) { drawH = STRIP_H - drawY; }

            // Fill with background (no fill, just border)
            spr.fillRect(btn.x, drawY, btn.w, drawH, CLR_BG_DARK);

            // Border in CLR_ACCENT
            if (btnTop >= stripY && btnTop < stripBot)
                spr.drawFastHLine(btn.x, btnTop - stripY, btn.w, CLR_ACCENT);
            if (btnBottom - 1 >= stripY && btnBottom - 1 < stripBot)
                spr.drawFastHLine(btn.x, btnBottom - 1 - stripY, btn.w, CLR_ACCENT);
            spr.drawFastVLine(btn.x, drawY, drawH, CLR_ACCENT);
            spr.drawFastVLine(btn.x + btn.w - 1, drawY, drawH, CLR_ACCENT);

            // Label
            int textY = btnTop + btn.h / 2;
            if (textY >= stripY && textY < stripBot) {
                spr.setTextColor(CLR_ACCENT);
                spr.setTextDatum(MC_DATUM);
                spr.setTextFont(1);
                spr.drawString("< BACK", btn.x + btn.w / 2, textY - stripY);
            }
        }
    }
}

// -------------------------------------------------------
bool SettingsScreen::handleTap(TouchPoint pt) {
    OsmosisSettings& s = settingsMgr.settings();
    bool changed = false;

    // Words per day buttons
    for (int i = 0; i < WPD_COUNT; i++) {
        Button btn = {WPD_X[i], WPD_Y, WPD_W, WPD_H};
        if (hitTest(btn, pt)) {
            s.wordsPerDay = (uint8_t)WPD_VALUES[i];
            changed = true;
            return changed;
        }
    }

    // Display time buttons
    for (int i = 0; i < DT_COUNT; i++) {
        Button btn = {DT_X[i], DT_Y, DT_W, DT_H};
        if (hitTest(btn, pt)) {
            s.displaySecs = (uint16_t)DT_VALUES[i];
            changed = true;
            return changed;
        }
    }

    // Brightness buttons
    for (int i = 0; i < BR_COUNT; i++) {
        Button btn = {BR_X[i], BR_Y, BR_W, BR_H};
        if (hitTest(btn, pt)) {
            s.brightness = (uint8_t)BR_VALUES[i];
            display.setBrightnessLevel((uint8_t)BR_VALUES[i]);
            changed = true;
            return changed;
        }
    }

    // Back button
    {
        Button btn = {BACK_X, BACK_Y, BACK_W, BACK_H};
        if (hitTest(btn, pt)) {
            hide();
            settingsMgr.save();
            return false;
        }
    }

    return changed;
}
