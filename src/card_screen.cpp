#include "card_screen.h"
#include "display_manager.h"
#include "card_manager.h"
#include "image_renderer.h"
#include "constants.h"

void cardScreen::render() {
    const WordEntry& word = cardMgr.currentWord();

    // Build the "Card X / Y" string once
    char counterBuf[24];
    snprintf(counterBuf, sizeof(counterBuf), "Card %d / %d",
             cardMgr.currentCardIndex(), cardMgr.totalCardsToday());

    for (int strip = 0; strip < NUM_STRIPS; strip++) {
        int stripY = strip * STRIP_H;
        TFT_eSprite& spr = display.getStrip();

        // 1. Fill entire strip with background
        spr.fillSprite(CLR_BG_DARK);

        // 2. Header band (y 0-29)
        if (stripY < HEADER_H) {
            // Draw header background for the portion of this strip that overlaps
            int hStart = 0;
            int hEnd = HEADER_H - stripY;
            if (hEnd > STRIP_H) hEnd = STRIP_H;
            if (hStart < hEnd) {
                spr.fillRect(0, hStart, SCREEN_W, hEnd - hStart, CLR_HEADER_BG);
            }

            // "OSMOSIS" title at screen y=4, Font 4, centered
            {
                int textScreenY = 4;
                if (textScreenY >= stripY && textScreenY < stripY + STRIP_H) {
                    spr.setTextDatum(TC_DATUM);
                    spr.setTextColor(CLR_ACCENT, CLR_HEADER_BG);
                    spr.drawString("OSMOSIS", SCREEN_W / 2, textScreenY - stripY, 4);
                }
            }

            // "~ Spanish ~" subtitle at screen y=20, Font 1
            {
                int textScreenY = 20;
                if (textScreenY >= stripY && textScreenY < stripY + STRIP_H) {
                    spr.setTextDatum(TC_DATUM);
                    spr.setTextColor(CLR_TEXT_SECONDARY, CLR_HEADER_BG);
                    spr.drawString("~ Spanish ~", SCREEN_W / 2, textScreenY - stripY, 1);
                }
            }
        }

        // 3. Image area (y 45-164)
        if (stripY + STRIP_H > IMG_Y && stripY < IMG_Y + IMG_H) {
            imageRenderer::drawPreloaded(IMG_X, IMG_Y, stripY);
        }

        // 4. Word box (y 180-249)
        {
            int boxTop = WORD_BOX_Y;
            int boxBottom = WORD_BOX_Y + WORD_BOX_H;
            int boxLeft = WORD_BOX_X;
            int boxRight = WORD_BOX_X + WORD_BOX_W - 1;

            if (stripY + STRIP_H > boxTop && stripY < boxBottom) {
                // Fill box background (interior)
                int fillTop = boxTop + 1;
                int fillBottom = boxBottom - 1;
                int rStart = fillTop - stripY;
                int rEnd = fillBottom - stripY;
                if (rStart < 0) rStart = 0;
                if (rEnd > STRIP_H) rEnd = STRIP_H;
                if (rStart < rEnd) {
                    spr.fillRect(boxLeft + 1, rStart, WORD_BOX_W - 2, rEnd - rStart, CLR_WORD_BOX_BG);
                }

                // Top border
                if (boxTop >= stripY && boxTop < stripY + STRIP_H) {
                    int localY = boxTop - stripY;
                    spr.drawFastHLine(boxLeft, localY, WORD_BOX_W, CLR_WORD_BOX_BORDER);
                }

                // Bottom border
                int bottomLineY = boxBottom - 1;
                if (bottomLineY >= stripY && bottomLineY < stripY + STRIP_H) {
                    int localY = bottomLineY - stripY;
                    spr.drawFastHLine(boxLeft, localY, WORD_BOX_W, CLR_WORD_BOX_BORDER);
                }

                // Left border
                {
                    int vStart = boxTop - stripY;
                    int vEnd = boxBottom - stripY;
                    if (vStart < 0) vStart = 0;
                    if (vEnd > STRIP_H) vEnd = STRIP_H;
                    if (vStart < vEnd) {
                        spr.drawFastVLine(boxLeft, vStart, vEnd - vStart, CLR_WORD_BOX_BORDER);
                    }
                }

                // Right border
                {
                    int vStart = boxTop - stripY;
                    int vEnd = boxBottom - stripY;
                    if (vStart < 0) vStart = 0;
                    if (vEnd > STRIP_H) vEnd = STRIP_H;
                    if (vStart < vEnd) {
                        spr.drawFastVLine(boxRight, vStart, vEnd - vStart, CLR_WORD_BOX_BORDER);
                    }
                }

                // Spanish word in white, Font 6, at screen y = WORD_BOX_Y + 12
                {
                    int textScreenY = WORD_BOX_Y + 12;
                    if (textScreenY >= stripY && textScreenY < stripY + STRIP_H) {
                        spr.setTextDatum(TC_DATUM);
                        spr.setTextColor(CLR_TEXT_PRIMARY, CLR_WORD_BOX_BG);
                        spr.drawString(word.spanish, SCREEN_W / 2, textScreenY - stripY, 6);
                    }
                }

                // English word in secondary color, Font 2, at screen y = WORD_BOX_Y + 46
                {
                    int textScreenY = WORD_BOX_Y + 46;
                    if (textScreenY >= stripY && textScreenY < stripY + STRIP_H) {
                        spr.setTextDatum(TC_DATUM);
                        spr.setTextColor(CLR_TEXT_SECONDARY, CLR_WORD_BOX_BG);
                        spr.drawString(word.english, SCREEN_W / 2, textScreenY - stripY, 2);
                    }
                }
            }
        }

        // 5. Card counter at screen y = COUNTER_Y, Font 1
        {
            if (COUNTER_Y >= stripY && COUNTER_Y < stripY + STRIP_H) {
                spr.setTextDatum(TC_DATUM);
                spr.setTextColor(CLR_TEXT_DIM, CLR_BG_DARK);
                spr.drawString(counterBuf, SCREEN_W / 2, COUNTER_Y - stripY, 1);
            }
        }

        // Push the completed strip to the display
        display.pushStrip(stripY);
    }
}
