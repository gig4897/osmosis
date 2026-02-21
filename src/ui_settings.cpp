#include "ui_settings.h"
#include "display_manager.h"
#include "settings_manager.h"
#include "wifi_manager.h"
#include "pack_manager.h"
#include "vocab_loader.h"
#include "card_manager.h"
#include "card_screen.h"
#include "constants.h"

SettingsScreen settingsUI;

// --- Words per day row ---
static const int WPD_VALUES[]   = {10, 15, 20};
static const char* WPD_LABELS[] = {"10", "15", "20"};
static const int WPD_COUNT      = 3;
static const int WPD_LABEL_Y    = 30;
static const int WPD_Y          = 44;
static const int WPD_H          = 36;
static const int WPD_W          = 68;
static const int WPD_X[]        = {10, 86, 162};

// --- Display time row ---
static const int DT_VALUES[]    = {30, 60, 120, 300};
static const char* DT_LABELS[]  = {"30s", "1m", "2m", "5m"};
static const int DT_COUNT       = 4;
static const int DT_LABEL_Y     = 86;
static const int DT_Y           = 100;
static const int DT_H           = 36;
static const int DT_W           = 52;
static const int DT_X[]         = {8, 66, 124, 182};

// --- Brightness row ---
static const int BR_VALUES[]    = {0, 1, 2};
static const char* BR_LABELS[]  = {"Low", "Med", "High"};
static const int BR_COUNT       = 3;
static const int BR_LABEL_Y     = 142;
static const int BR_Y           = 156;
static const int BR_H           = 36;
static const int BR_W           = 68;
static const int BR_X[]         = {10, 86, 162};

// --- Phonetic toggle row ---
static const char* PH_LABELS[]  = {"OFF", "ON"};
static const int PH_COUNT       = 2;
static const int PH_LABEL_Y     = 198;
static const int PH_Y           = 212;
static const int PH_H           = 36;
static const int PH_W           = 100;
static const int PH_X[]         = {20, 130};

// --- Language button ---
static const int LANG_X = 20;
static const int LANG_Y = 256;
static const int LANG_W = 200;
static const int LANG_H = 28;

// --- WiFi status row ---
static const int WIFI_Y = 290;

// -------------------------------------------------------
void SettingsScreen::show() {
    _active = true;
    _page = SettingsPage::Main;
    _selectedLang = -1;
}

void SettingsScreen::hide() {
    _active = false;
    _page = SettingsPage::Main;
}

// -------------------------------------------------------
bool SettingsScreen::hitTest(const Button& btn, TouchPoint pt) {
    return pt.x >= btn.x && pt.x < btn.x + btn.w &&
           pt.y >= btn.y && pt.y < btn.y + btn.h;
}

// -------------------------------------------------------
void SettingsScreen::drawButton(TFT_eSprite& spr, const Button& btn,
                                int stripY, const char* label, bool selected) {
    int btnTop    = btn.y;
    int btnBottom = btn.y + btn.h;
    int stripTop  = stripY;
    int stripBot  = stripY + STRIP_H;

    if (btnBottom <= stripTop || btnTop >= stripBot) return;

    int drawY = btnTop - stripY;
    int drawH = btn.h;
    if (drawY < 0) { drawH += drawY; drawY = 0; }
    if (drawY + drawH > STRIP_H) { drawH = STRIP_H - drawY; }

    uint16_t fillClr  = selected ? CLR_BTN_ACTIVE : CLR_BTN_INACTIVE;
    uint16_t textClr  = selected ? CLR_TEXT_PRIMARY : CLR_TEXT_SECONDARY;

    spr.fillRect(btn.x, drawY, btn.w, drawH, fillClr);

    uint16_t borderClr = selected ? CLR_ACCENT : CLR_BTN_INACTIVE;
    if (btnTop >= stripTop && btnTop < stripBot)
        spr.drawFastHLine(btn.x, btnTop - stripY, btn.w, borderClr);
    if (btnBottom - 1 >= stripTop && btnBottom - 1 < stripBot)
        spr.drawFastHLine(btn.x, btnBottom - 1 - stripY, btn.w, borderClr);
    spr.drawFastVLine(btn.x, drawY, drawH, borderClr);
    spr.drawFastVLine(btn.x + btn.w - 1, drawY, drawH, borderClr);

    int textScreenY = btnTop + (btn.h - 16) / 2;
    int y = textScreenY - stripY;
    if (y >= -16 && y < STRIP_H) {
        spr.setTextColor(textClr, fillClr);
        spr.setTextDatum(TC_DATUM);
        spr.drawString(label, btn.x + btn.w / 2, y, 2);
    }
}

// -------------------------------------------------------
void SettingsScreen::draw(TFT_eSprite& spr, int stripY) {
    switch (_page) {
        case SettingsPage::Main:
            drawMainPage(spr, stripY);
            break;
        case SettingsPage::LanguageBrowser:
            drawLanguageBrowser(spr, stripY);
            break;
        case SettingsPage::DownloadProgress:
            drawDownloadProgress(spr, stripY);
            break;
    }
}

// -------------------------------------------------------
void SettingsScreen::drawMainPage(TFT_eSprite& spr, int stripY) {
    spr.fillSprite(CLR_BG_DARK);
    const OsmosisSettings& s = settingsMgr.settings();

    // Title
    {
        int y = 6 - stripY;
        if (y >= -26 && y < STRIP_H) {
            spr.setTextColor(CLR_ACCENT, CLR_BG_DARK);
            spr.setTextDatum(TC_DATUM);
            spr.drawString("SETTINGS", SCREEN_W / 2, y, 4);
        }
    }

    // Row 1: Words per day
    {
        int y = WPD_LABEL_Y - stripY;
        if (y >= -16 && y < STRIP_H) {
            spr.setTextColor(CLR_TEXT_SECONDARY, CLR_BG_DARK);
            spr.setTextDatum(TC_DATUM);
            spr.drawString("Words per day", SCREEN_W / 2, y, 2);
        }
    }
    for (int i = 0; i < WPD_COUNT; i++) {
        Button btn = {WPD_X[i], WPD_Y, WPD_W, WPD_H};
        drawButton(spr, btn, stripY, WPD_LABELS[i], s.wordsPerDay == WPD_VALUES[i]);
    }

    // Row 2: Display time
    {
        int y = DT_LABEL_Y - stripY;
        if (y >= -16 && y < STRIP_H) {
            spr.setTextColor(CLR_TEXT_SECONDARY, CLR_BG_DARK);
            spr.setTextDatum(TC_DATUM);
            spr.drawString("Display time", SCREEN_W / 2, y, 2);
        }
    }
    for (int i = 0; i < DT_COUNT; i++) {
        Button btn = {DT_X[i], DT_Y, DT_W, DT_H};
        drawButton(spr, btn, stripY, DT_LABELS[i], s.displaySecs == (uint16_t)DT_VALUES[i]);
    }

    // Row 3: Brightness
    {
        int y = BR_LABEL_Y - stripY;
        if (y >= -16 && y < STRIP_H) {
            spr.setTextColor(CLR_TEXT_SECONDARY, CLR_BG_DARK);
            spr.setTextDatum(TC_DATUM);
            spr.drawString("Brightness", SCREEN_W / 2, y, 2);
        }
    }
    for (int i = 0; i < BR_COUNT; i++) {
        Button btn = {BR_X[i], BR_Y, BR_W, BR_H};
        drawButton(spr, btn, stripY, BR_LABELS[i], s.brightness == (uint8_t)BR_VALUES[i]);
    }

    // Row 4: Phonetic toggle
    {
        int y = PH_LABEL_Y - stripY;
        if (y >= -16 && y < STRIP_H) {
            spr.setTextColor(CLR_TEXT_SECONDARY, CLR_BG_DARK);
            spr.setTextDatum(TC_DATUM);
            spr.drawString("Phonetic", SCREEN_W / 2, y, 2);
        }
    }
    for (int i = 0; i < PH_COUNT; i++) {
        Button btn = {PH_X[i], PH_Y, PH_W, PH_H};
        bool sel = (i == 0 && !s.showPhonetic) || (i == 1 && s.showPhonetic);
        drawButton(spr, btn, stripY, PH_LABELS[i], sel);
    }

    // Row 5: Language button
    {
        Button btn = {LANG_X, LANG_Y, LANG_W, LANG_H};
        char label[32];
        if (strlen(s.installedLang) > 0) {
            snprintf(label, sizeof(label), "Lang: %s", s.installedLang);
        } else {
            strlcpy(label, "Browse Languages", sizeof(label));
        }
        drawButton(spr, btn, stripY, label, false);
    }

    // Row 6: WiFi status text
    {
        int y = WIFI_Y - stripY;
        if (y >= -16 && y < STRIP_H) {
            spr.setTextDatum(TC_DATUM);
            const char* status;
            switch (wifiMgr::state()) {
                case WiFiState::Connected:      status = "WiFi: Connected"; break;
                case WiFiState::Connecting:      status = "WiFi: Connecting..."; break;
                case WiFiState::CaptivePortalActive: status = "WiFi: Portal Active"; break;
                case WiFiState::Disconnected:   status = "WiFi: Disconnected"; break;
                default:                        status = "WiFi: Not Setup"; break;
            }
            spr.setTextColor(CLR_TEXT_DIM, CLR_BG_DARK);
            spr.drawString(status, SCREEN_W / 2, y, 1);
        }
    }
}

// -------------------------------------------------------
void SettingsScreen::drawLanguageBrowser(TFT_eSprite& spr, int stripY) {
    spr.fillSprite(CLR_BG_DARK);

    // Title
    {
        int y = 6 - stripY;
        if (y >= -26 && y < STRIP_H) {
            spr.setTextColor(CLR_ACCENT, CLR_BG_DARK);
            spr.setTextDatum(TC_DATUM);
            spr.drawString("LANGUAGES", SCREEN_W / 2, y, 4);
        }
    }

    uint8_t count = packMgr::languageCount();

    if (count == 0) {
        // No catalog loaded
        int y = 100 - stripY;
        if (y >= -16 && y < STRIP_H) {
            spr.setTextColor(CLR_TEXT_SECONDARY, CLR_BG_DARK);
            spr.setTextDatum(TC_DATUM);
            spr.drawString("Fetching catalog...", SCREEN_W / 2, y, 2);
        }
        return;
    }

    if (_selectedLang < 0) {
        // Show language list
        for (uint8_t i = 0; i < count && i < 6; i++) {
            int btnY = 40 + i * 40;
            Button btn = {20, btnY, 200, 34};
            drawButton(spr, btn, stripY, packMgr::language(i).name, false);
        }

        // Back button
        Button back = {60, 290, 120, 26};
        drawButton(spr, back, stripY, "< BACK", false);
    } else {
        // Show tiers for selected language
        {
            int y = 36 - stripY;
            if (y >= -16 && y < STRIP_H) {
                spr.setTextColor(CLR_TEXT_SECONDARY, CLR_BG_DARK);
                spr.setTextDatum(TC_DATUM);
                spr.drawString(packMgr::language(_selectedLang).name, SCREEN_W / 2, y, 2);
            }
        }

        uint8_t tCount = packMgr::tierCount(_selectedLang);
        for (uint8_t i = 0; i < tCount && i < 4; i++) {
            int btnY = 60 + i * 50;
            Button btn = {20, btnY, 200, 40};
            char label[32];
            const CatalogTier& t = packMgr::tier(_selectedLang, i);
            snprintf(label, sizeof(label), "%s (%u words)", t.name, t.words);
            drawButton(spr, btn, stripY, label, false);
        }

        // Back to language list
        Button back = {60, 280, 120, 26};
        drawButton(spr, back, stripY, "< BACK", false);
    }
}

// -------------------------------------------------------
void SettingsScreen::drawDownloadProgress(TFT_eSprite& spr, int stripY) {
    spr.fillSprite(CLR_BG_DARK);

    // Title
    {
        int y = 60 - stripY;
        if (y >= -26 && y < STRIP_H) {
            spr.setTextColor(CLR_ACCENT, CLR_BG_DARK);
            spr.setTextDatum(TC_DATUM);
            spr.drawString("Downloading", SCREEN_W / 2, y, 4);
        }
    }

    uint8_t pct = packMgr::progressPercent();

    // Progress bar outline (200x20, centered)
    {
        int barX = 20;
        int barY = 140;
        int barW = 200;
        int barH = 20;
        int barBottom = barY + barH;

        if (barBottom > stripY && barY < stripY + STRIP_H) {
            int drawY = barY - stripY;
            int drawH = barH;
            if (drawY < 0) { drawH += drawY; drawY = 0; }
            if (drawY + drawH > STRIP_H) { drawH = STRIP_H - drawY; }

            // Background
            spr.fillRect(barX, drawY, barW, drawH, CLR_BTN_INACTIVE);

            // Fill
            int fillW = (int)((uint32_t)barW * pct / 100);
            if (fillW > 0) {
                spr.fillRect(barX, drawY, fillW, drawH, CLR_ACCENT);
            }

            // Border
            if (barY >= stripY && barY < stripY + STRIP_H)
                spr.drawFastHLine(barX, barY - stripY, barW, CLR_TEXT_SECONDARY);
            if (barBottom - 1 >= stripY && barBottom - 1 < stripY + STRIP_H)
                spr.drawFastHLine(barX, barBottom - 1 - stripY, barW, CLR_TEXT_SECONDARY);
            spr.drawFastVLine(barX, drawY, drawH, CLR_TEXT_SECONDARY);
            spr.drawFastVLine(barX + barW - 1, drawY, drawH, CLR_TEXT_SECONDARY);
        }
    }

    // Percentage text
    {
        int y = 170 - stripY;
        if (y >= -16 && y < STRIP_H) {
            char buf[8];
            snprintf(buf, sizeof(buf), "%u%%", pct);
            spr.setTextColor(CLR_TEXT_PRIMARY, CLR_BG_DARK);
            spr.setTextDatum(TC_DATUM);
            spr.drawString(buf, SCREEN_W / 2, y, 4);
        }
    }

    // Status message
    {
        int y = 210 - stripY;
        if (y >= -16 && y < STRIP_H) {
            spr.setTextColor(CLR_TEXT_SECONDARY, CLR_BG_DARK);
            spr.setTextDatum(TC_DATUM);
            spr.drawString(packMgr::statusText(), SCREEN_W / 2, y, 2);
        }
    }
}

// -------------------------------------------------------
bool SettingsScreen::handleTap(TouchPoint pt) {
    switch (_page) {
        case SettingsPage::Main:
            return handleMainTap(pt);
        case SettingsPage::LanguageBrowser:
            return handleBrowserTap(pt);
        case SettingsPage::DownloadProgress:
            return false;  // No touch during download
    }
    return false;
}

// -------------------------------------------------------
bool SettingsScreen::handleMainTap(TouchPoint pt) {
    OsmosisSettings& s = settingsMgr.settings();

    // Words per day
    for (int i = 0; i < WPD_COUNT; i++) {
        Button btn = {WPD_X[i], WPD_Y, WPD_W, WPD_H};
        if (hitTest(btn, pt)) {
            s.wordsPerDay = (uint8_t)WPD_VALUES[i];
            return true;
        }
    }

    // Display time
    for (int i = 0; i < DT_COUNT; i++) {
        Button btn = {DT_X[i], DT_Y, DT_W, DT_H};
        if (hitTest(btn, pt)) {
            s.displaySecs = (uint16_t)DT_VALUES[i];
            return true;
        }
    }

    // Brightness
    for (int i = 0; i < BR_COUNT; i++) {
        Button btn = {BR_X[i], BR_Y, BR_W, BR_H};
        if (hitTest(btn, pt)) {
            s.brightness = (uint8_t)BR_VALUES[i];
            display.setBrightnessLevel((uint8_t)BR_VALUES[i]);
            return true;
        }
    }

    // Phonetic toggle
    for (int i = 0; i < PH_COUNT; i++) {
        Button btn = {PH_X[i], PH_Y, PH_W, PH_H};
        if (hitTest(btn, pt)) {
            s.showPhonetic = (i == 1);
            return true;
        }
    }

    // Language button — open browser
    {
        Button btn = {LANG_X, LANG_Y, LANG_W, LANG_H};
        if (hitTest(btn, pt)) {
            if (wifiMgr::isConnected()) {
                _page = SettingsPage::LanguageBrowser;
                _selectedLang = -1;
                // Fetch catalog if not already loaded
                if (packMgr::languageCount() == 0) {
                    packMgr::fetchCatalog();
                }
            } else {
                // Start captive portal if WiFi not connected
                wifiMgr::startCaptivePortal();
            }
            return true;
        }
    }

    return false;
}

// -------------------------------------------------------
bool SettingsScreen::handleBrowserTap(TouchPoint pt) {
    uint8_t count = packMgr::languageCount();

    if (_selectedLang < 0) {
        // Language list
        for (uint8_t i = 0; i < count && i < 6; i++) {
            int btnY = 40 + i * 40;
            Button btn = {20, btnY, 200, 34};
            if (hitTest(btn, pt)) {
                _selectedLang = i;
                return true;
            }
        }

        // Back button
        Button back = {60, 290, 120, 26};
        if (hitTest(back, pt)) {
            _page = SettingsPage::Main;
            return true;
        }
    } else {
        // Tier list
        uint8_t tCount = packMgr::tierCount(_selectedLang);
        for (uint8_t i = 0; i < tCount && i < 4; i++) {
            int btnY = 60 + i * 50;
            Button btn = {20, btnY, 200, 40};
            if (hitTest(btn, pt)) {
                // Start download
                _page = SettingsPage::DownloadProgress;
                packMgr::startDownload(_selectedLang, i);

                // After download completes, reload vocab and font
                if (packMgr::state() == PackDownloadState::Complete) {
                    vocabLoader::load();
                    cardScreen::reloadFont();
                    cardMgr.init();
                    _page = SettingsPage::Main;
                    hide();
                }
                return true;
            }
        }

        // Back to language list
        Button back = {60, 280, 120, 26};
        if (hitTest(back, pt)) {
            _selectedLang = -1;
            return true;
        }
    }

    return false;
}
