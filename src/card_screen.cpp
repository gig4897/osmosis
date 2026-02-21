#include "card_screen.h"
#include "display_manager.h"
#include "card_manager.h"
#include "image_renderer.h"
#include "vocab_loader.h"
#include "settings_manager.h"
#include "constants.h"
#include <FS.h>
#include <SPIFFS.h>

// Dimmed accent color for glow effect (~40% brightness of CLR_ACCENT)
static const uint16_t CLR_ACCENT_GLOW = 0x1909;

// Smooth font data buffer (loaded from SPIFFS at init)
static uint8_t* fontData26 = nullptr;
static bool smoothFontReady = false;

// Helper: draw a filled rounded rect clipped to the current strip
static void drawRoundedRect(TFT_eSprite& spr, int bx, int by, int bw, int bh,
                            int r, int stripY, uint16_t fillClr, uint16_t borderClr) {
    int bBottom = by + bh;
    int stripBot = stripY + STRIP_H;

    // Nothing to draw if no overlap
    if (bBottom <= stripY || by >= stripBot) return;

    // Draw row by row within this strip
    for (int sy = 0; sy < STRIP_H; sy++) {
        int screenY = stripY + sy;
        if (screenY < by || screenY >= bBottom) continue;

        int ry = screenY - by;       // row within the box (0..bh-1)
        int ryb = bh - 1 - ry;       // distance from bottom

        // Determine horizontal inset from corners
        int inset = 0;
        if (ry < r) {
            // Top corners: circular inset
            int dy = r - ry;
            // inset = r - sqrt(r*r - dy*dy), approximated
            int dx2 = r * r - dy * dy;
            int dx = 0;
            while ((dx + 1) * (dx + 1) <= dx2) dx++;
            inset = r - dx;
        } else if (ryb < r) {
            // Bottom corners
            int dy = r - ryb;
            int dx2 = r * r - dy * dy;
            int dx = 0;
            while ((dx + 1) * (dx + 1) <= dx2) dx++;
            inset = r - dx;
        }

        int x0 = bx + inset;
        int x1 = bx + bw - 1 - inset;
        int lineW = x1 - x0 + 1;
        if (lineW > 0) {
            spr.drawFastHLine(x0, sy, lineW, fillClr);
        }

        // Border: draw left and right edge pixels
        bool isTopOrBottom = (ry == 0 || ry == bh - 1);
        if (!isTopOrBottom && lineW > 0) {
            spr.drawPixel(x0, sy, borderClr);
            spr.drawPixel(x1, sy, borderClr);
        }
        // Top/bottom border: draw the full row in border color
        if (ry == 0 || ry == bh - 1) {
            if (lineW > 0) {
                spr.drawFastHLine(x0, sy, lineW, borderClr);
            }
        }
    }
}

// Load smooth font VLW data from SPIFFS into heap buffer
static uint8_t* loadVlwFromSpiffs(const char* filename) {
    char path[48];
    snprintf(path, sizeof(path), "/%s.vlw", filename);

    fs::File f = SPIFFS.open(path, "r");
    if (!f) {
        Serial.printf("[font] File not found: %s\n", path);
        return nullptr;
    }

    size_t fileSize = f.size();
    uint8_t* buf = (uint8_t*)malloc(fileSize);
    if (!buf) {
        Serial.printf("[font] Failed to allocate %u bytes\n", (uint32_t)fileSize);
        f.close();
        return nullptr;
    }

    size_t bytesRead = f.read(buf, fileSize);
    f.close();

    if (bytesRead != fileSize) {
        Serial.printf("[font] Short read: %u of %u\n", (uint32_t)bytesRead, (uint32_t)fileSize);
        free(buf);
        return nullptr;
    }

    Serial.printf("[font] Loaded %s (%u bytes)\n", path, (uint32_t)fileSize);
    return buf;
}

namespace cardScreen {

void init() {
    reloadFont();
}

void reloadFont() {
    // Free previous font
    if (fontData26) {
        free(fontData26);
        fontData26 = nullptr;
        smoothFontReady = false;
    }
    // Load the pack's font file
    fontData26 = loadVlwFromSpiffs("font");  // loads /font.vlw
    if (fontData26) {
        smoothFontReady = true;
        Serial.println("[font] Smooth font ready");
    } else {
        Serial.println("[font] WARNING: Smooth font not loaded");
    }
}

void freeFont() {
    if (fontData26) {
        free(fontData26);
        fontData26 = nullptr;
        smoothFontReady = false;
        Serial.println("[font] Freed font buffer");
    }
}

void render() {
    const WordEntry& word = cardMgr.currentWord();
    const bool showPhonetic = settingsMgr.settings().showPhonetic && strlen(word.phonetic) > 0;

    // Build the "Card X / Y" string once
    char counterBuf[24];
    snprintf(counterBuf, sizeof(counterBuf), "Card %d / %d",
             cardMgr.currentCardIndex(), cardMgr.totalCardsToday());

    // Dynamic layout: when phonetic is on, make box taller and shift image + box up
    const int phoneticShift = showPhonetic ? 16 : 0;
    const int imgY   = IMG_Y - phoneticShift;
    const int boxY   = WORD_BOX_Y - phoneticShift;
    const int boxH   = showPhonetic ? (WORD_BOX_H + 16) : WORD_BOX_H;  // 70 vs 54

    for (int strip = 0; strip < NUM_STRIPS; strip++) {
        int stripY = strip * STRIP_H;
        TFT_eSprite& spr = display.getStrip();

        // Ensure smooth font is NOT active for built-in font rendering
        // (smooth font overrides all drawString calls when loaded)
        if (spr.fontLoaded) spr.unloadFont();

        // 1. Fill entire strip with background
        spr.fillSprite(CLR_BG_DARK);

        // 2. Header band (y 0..HEADER_H-1)
        if (stripY < HEADER_H) {
            int hStart = 0;
            int hEnd = HEADER_H - stripY;
            if (hEnd > STRIP_H) hEnd = STRIP_H;
            if (hStart < hEnd) {
                spr.fillRect(0, hStart, SCREEN_W, hEnd - hStart, CLR_HEADER_BG);
            }
        }

        // "OSMOSIS" title at y=4, Font 2 (16px), with glow effect
        {
            int textScreenY = 4;
            int y = textScreenY - stripY;
            if (y >= -18 && y < STRIP_H) {
                spr.setTextDatum(TC_DATUM);
                // Glow: draw in dim accent offset by 1px in multiple directions
                spr.setTextColor(CLR_ACCENT_GLOW, CLR_HEADER_BG);
                spr.drawString("OSMOSIS", SCREEN_W / 2 - 1, y, 2);
                spr.drawString("OSMOSIS", SCREEN_W / 2 + 1, y, 2);
                spr.drawString("OSMOSIS", SCREEN_W / 2, y - 1, 2);
                spr.drawString("OSMOSIS", SCREEN_W / 2, y + 1, 2);
                // Main text on top
                spr.setTextColor(CLR_ACCENT, CLR_HEADER_BG);
                spr.drawString("OSMOSIS", SCREEN_W / 2, y, 2);
            }
        }

        // Dynamic language subtitle at y=20, Font 2 (16px)
        {
            int textScreenY = 20;
            int y = textScreenY - stripY;
            if (y >= -16 && y < STRIP_H) {
                char subtitle[32];
                const char* langName = vocabLoader::isLoaded()
                    ? vocabLoader::packInfo().languageDisplay : "No Pack";
                snprintf(subtitle, sizeof(subtitle), "~ %s ~", langName);
                spr.setTextDatum(TC_DATUM);
                spr.setTextColor(CLR_TEXT_SECONDARY, CLR_HEADER_BG);
                spr.drawString(subtitle, SCREEN_W / 2, y, 2);
            }
        }

        // 3. Image area (shifts up when phonetic is on)
        if (stripY + STRIP_H > imgY && stripY < imgY + IMG_DISPLAY_H) {
            imageRenderer::drawPreloaded(IMG_X, imgY, stripY);
        }

        // 4. Word box with rounded corners (taller when phonetic is on)
        drawRoundedRect(spr, WORD_BOX_X, boxY, WORD_BOX_W, boxH,
                        WORD_BOX_R, stripY, CLR_WORD_BOX_BG, CLR_WORD_BOX_BORDER);

        // 5. Foreign word — use smooth font for proper accented character support
        {
            int textScreenY = boxY + 6;
            int y = textScreenY - stripY;
            if (y >= -28 && y < STRIP_H) {
                if (smoothFontReady) {
                    spr.loadFont(fontData26);
                }
                spr.setTextDatum(TC_DATUM);
                spr.setTextColor(CLR_TEXT_PRIMARY, CLR_WORD_BOX_BG);
                if (smoothFontReady) {
                    spr.drawString(word.word, SCREEN_W / 2, y);  // smooth font (no font number)
                } else {
                    spr.drawString(word.word, SCREEN_W / 2, y, 4);  // fallback to Font 4
                }
                if (spr.fontLoaded) spr.unloadFont();
            }
        }

        // 6. Phonetic pronunciation (between foreign and english when enabled)
        if (showPhonetic) {
            int textScreenY = boxY + 32;
            int y = textScreenY - stripY;
            if (y >= -16 && y < STRIP_H) {
                spr.setTextDatum(TC_DATUM);
                spr.setTextColor(CLR_PHONETIC, CLR_WORD_BOX_BG);
                spr.drawString(word.phonetic, SCREEN_W / 2, y, 2);
            }
        }

        // 7. English word — Font 2 (16px), high contrast
        {
            int textScreenY = showPhonetic ? (boxY + 50) : (boxY + 34);
            int y = textScreenY - stripY;
            if (y >= -16 && y < STRIP_H) {
                spr.setTextDatum(TC_DATUM);
                spr.setTextColor(CLR_ENGLISH, CLR_WORD_BOX_BG);
                spr.drawString(word.english, SCREEN_W / 2, y, 2);
            }
        }

        // 8. Card counter
        {
            int y = COUNTER_Y - stripY;
            if (y >= -16 && y < STRIP_H) {
                spr.setTextDatum(TC_DATUM);
                spr.setTextColor(CLR_TEXT_DIM, CLR_BG_DARK);
                spr.drawString(counterBuf, SCREEN_W / 2, y, 2);
            }
        }

        // Push the completed strip to the display
        display.pushStrip(stripY);
    }
}

}  // namespace cardScreen
