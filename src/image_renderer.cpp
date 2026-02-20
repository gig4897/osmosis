#include "image_renderer.h"
#include "display_manager.h"
#include "constants.h"
#include <FS.h>
#include <SPIFFS.h>

// Static heap-allocated buffer for decompressed 120x120 RGB565 image
static uint16_t* imageBuffer = nullptr;
static bool imageLoaded = false;

// ORLE header: 4 magic + 2 width + 2 height + 4 compressed size = 12 bytes (big-endian)
static const uint32_t ORLE_MAGIC = 0x4F524C45;  // "ORLE"
static const size_t ORLE_HEADER_SIZE = 12;

static uint16_t readU16BE(const uint8_t* p) {
    return (uint16_t)(p[0] << 8) | p[1];
}

static uint32_t readU32BE(const uint8_t* p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8)  | (uint32_t)p[3];
}

static bool rleDecompress(const uint8_t* compressed, uint32_t compressedSize,
                          uint16_t* output, uint32_t pixelCount) {
    uint32_t srcPos = 0;
    uint32_t dstPos = 0;

    while (srcPos < compressedSize && dstPos < pixelCount) {
        uint8_t header = compressed[srcPos++];

        if (header & 0x80) {
            // Run-length encoded: (count & 0x7F + 1) identical pixels
            uint16_t count = (header & 0x7F) + 1;
            if (srcPos + 2 > compressedSize) return false;
            uint16_t pixel = readU16BE(&compressed[srcPos]);
            srcPos += 2;

            for (uint16_t i = 0; i < count && dstPos < pixelCount; i++) {
                output[dstPos++] = pixel;
            }
        } else {
            // Literal run: (count + 1) literal pixels follow
            uint16_t count = header + 1;
            if (srcPos + (uint32_t)count * 2 > compressedSize) return false;

            for (uint16_t i = 0; i < count && dstPos < pixelCount; i++) {
                output[dstPos++] = readU16BE(&compressed[srcPos]);
                srcPos += 2;
            }
        }
    }

    // Fill remaining pixels with black (transparent) if decompression ended early
    while (dstPos < pixelCount) {
        output[dstPos++] = 0x0000;
    }

    return true;
}

namespace imageRenderer {

bool preloadImage(const char* filename) {
    imageLoaded = false;

    // Allocate image buffer if not already done
    if (!imageBuffer) {
        imageBuffer = (uint16_t*)malloc(IMG_W * IMG_H * sizeof(uint16_t));
        if (!imageBuffer) {
            Serial.println("[img] Failed to allocate image buffer");
            return false;
        }
    }

    // Clear buffer to black (transparent)
    memset(imageBuffer, 0, IMG_W * IMG_H * sizeof(uint16_t));

    // Build path: /{filename}.bin
    char path[64];
    snprintf(path, sizeof(path), "/%s.bin", filename);

    fs::File f = SPIFFS.open(path, "r");
    if (!f) {
        Serial.printf("[img] File not found: %s\n", path);
        return false;
    }

    // Read ORLE header (12 bytes)
    uint8_t headerBuf[ORLE_HEADER_SIZE];
    if (f.read(headerBuf, ORLE_HEADER_SIZE) != ORLE_HEADER_SIZE) {
        Serial.println("[img] Failed to read ORLE header");
        f.close();
        return false;
    }

    uint32_t magic = readU32BE(&headerBuf[0]);
    uint16_t width = readU16BE(&headerBuf[4]);
    uint16_t height = readU16BE(&headerBuf[6]);
    uint32_t compressedSize = readU32BE(&headerBuf[8]);

    if (magic != ORLE_MAGIC) {
        Serial.printf("[img] Invalid magic: 0x%08X\n", magic);
        f.close();
        return false;
    }

    if (width != IMG_W || height != IMG_H) {
        Serial.printf("[img] Unexpected dimensions: %dx%d (expected %dx%d)\n",
                      width, height, IMG_W, IMG_H);
        f.close();
        return false;
    }

    // Allocate temporary buffer for compressed data
    uint8_t* compressed = (uint8_t*)malloc(compressedSize);
    if (!compressed) {
        Serial.printf("[img] Failed to allocate %u bytes for compressed data\n", compressedSize);
        f.close();
        return false;
    }

    // Read compressed data
    size_t bytesRead = f.read(compressed, compressedSize);
    f.close();

    if (bytesRead != compressedSize) {
        Serial.printf("[img] Short read: %u of %u bytes\n", (uint32_t)bytesRead, compressedSize);
        free(compressed);
        return false;
    }

    // RLE decompress into image buffer
    bool ok = rleDecompress(compressed, compressedSize, imageBuffer, IMG_W * IMG_H);
    free(compressed);

    if (!ok) {
        Serial.println("[img] RLE decompression failed");
        return false;
    }

    imageLoaded = true;
    Serial.printf("[img] Loaded %s (%ux%u, %u bytes compressed)\n",
                  path, width, height, compressedSize);
    return true;
}

void drawPreloaded(int x, int y, int stripY) {
    if (!imageLoaded || !imageBuffer) return;

    TFT_eSprite& strip = display.getStrip();

    // Determine which rows of the image overlap with the current strip
    // Strip covers screen rows [stripY .. stripY + STRIP_H - 1]
    // Image occupies screen rows [y .. y + IMG_H - 1]
    int imgRowStart = stripY - y;        // First image row that falls in this strip
    int imgRowEnd = imgRowStart + STRIP_H;  // One past last image row

    // Clamp to valid image row range
    if (imgRowStart < 0) imgRowStart = 0;
    if (imgRowEnd > IMG_H) imgRowEnd = IMG_H;

    // Nothing to draw if no overlap
    if (imgRowStart >= imgRowEnd || imgRowStart >= IMG_H || imgRowEnd <= 0) return;

    for (int imgRow = imgRowStart; imgRow < imgRowEnd; imgRow++) {
        int screenY = y + imgRow;           // Screen Y of this image row
        int spriteRow = screenY - stripY;   // Row within the strip sprite

        if (spriteRow < 0 || spriteRow >= STRIP_H) continue;

        const uint16_t* srcRow = &imageBuffer[imgRow * IMG_W];
        for (int col = 0; col < IMG_W; col++) {
            uint16_t pixel = srcRow[col];
            if (pixel != 0x0000) {  // Skip black pixels (transparent)
                strip.drawPixel(x + col, spriteRow, pixel);
            }
        }
    }
}

}  // namespace imageRenderer
