
#include "FlixCapacitor.h"

#if defined(ENABLE_TFT) && defined(ENABLE_AUDIO_SD)

#define BUFFPIXEL 32
#include <ILI9341_t3.h>

File bmpFile;
void resetScreen() {
    tft.fillScreen(tft.color565(0x20, 0x20, 0x20));

    tft.setTextColor(ILI9341_WHITE);
    tft.setTextSize(1);

    tft.setCursor(1, 20);
    tft.setTextWrap(true);
}

void bmpDraw(char *filename, uint8_t x, uint16_t y) {
    int bmpWidth, bmpHeight;   // W+H in pixels
    uint8_t bmpDepth;              // Bit depth (currently must be 24)
    uint32_t bmpImageoffset;        // Start of image data in file
    uint32_t rowSize;               // Not always = bmpWidth; may have padding
    uint8_t sdbuffer[3 * BUFFPIXEL]; // pixel buffer (R+G+B per pixel)
    uint8_t buffidx = sizeof(sdbuffer); // Current position in sdbuffer
    boolean goodBmp = false;       // Set to true on valid header parse
    boolean flip = true;        // BMP is stored bottom-to-top
    int w, h, row, col;
    uint8_t r, g, b;
    uint32_t pos = 0, startTime = millis();

    uint16_t awColors[320];  // hold colors for one row at a time...

    if ((x >= tft.width()) || (y >= tft.height()))
        return;

    Serial.println();
    Serial.print(F("Loading image ["));
    Serial.print(filename);
    Serial.print("]... ");
    bmpFile = SD.open(filename);
    // Open requested file on SD card
    if (!bmpFile) {
        Serial.println(F("File not found"));
        return;
    }

    // Parse BMP header
    if (read16(bmpFile) == 0x4D42) { // BMP signature
#ifdef DEBUG_IMAGES
        Serial.print(F("File size: "));
        Serial.println(read32(bmpFile));
#else
        read32(bmpFile);
#endif
        (void) read32(bmpFile); // Read & ignore creator bytes
        bmpImageoffset = read32(bmpFile); // Start of image data
#ifdef DEBUG_IMAGES
        Serial.print(F("Image Offset: "));
        Serial.println(bmpImageoffset, DEC);
        // Read DIB header
        Serial.print(F("Header size: "));
        Serial.println(read32(bmpFile));
#else
        read32(bmpFile);
#endif
        bmpWidth = read32(bmpFile);
        bmpHeight = read32(bmpFile);
        if (read16(bmpFile) == 1) { // # planes -- must be '1'
            bmpDepth = read16(bmpFile); // bits per pixel
#ifdef DEBUG_IMAGES
            Serial.print(F("Bit Depth: "));
            Serial.println(bmpDepth);
#endif
            if ((bmpDepth == 24) && (read32(bmpFile) == 0)) { // 0 = uncompressed

                goodBmp = true; // Supported BMP format -- proceed!
#ifdef DEBUG_IMAGES

                Serial.print(F("Image size: "));
                Serial.print(bmpWidth);
                Serial.print('x');
                Serial.println(bmpHeight);
#endif

                // BMP rows are padded (if needed) to 4-byte boundary
                rowSize = (bmpWidth * 3 + 3) & ~3;

                // If bmpHeight is negative, image is in top-down order.
                // This is not canon but has been observed in the wild.
                if (bmpHeight < 0) {
                    bmpHeight = -bmpHeight;
                    flip = false;
                }

                // Crop area to be loaded
                w = bmpWidth;
                h = bmpHeight;
                if ((x + w - 1) >= tft.width())
                    w = tft.width() - x;
                if ((y + h - 1) >= tft.height())
                    h = tft.height() - y;

                for (row = 0; row < h; row++) { // For each scanline...

                    // Seek to start of scan line.  It might seem labor-
                    // intensive to be doing this on every line, but this
                    // method covers a lot of gritty details like cropping
                    // and scanline padding.  Also, the seek only takes
                    // place if the file position actually needs to change
                    // (avoids a lot of cluster math in SD library).
                    if (flip) // Bitmap is stored bottom-to-top order (normal BMP)
                        pos = bmpImageoffset + (bmpHeight - 1 - row) * rowSize;
                    else
                        // Bitmap is stored top-to-bottom
                        pos = bmpImageoffset + row * rowSize;
                    if (bmpFile.position() != pos) { // Need seek?
                        bmpFile.seek(pos);
                        buffidx = sizeof(sdbuffer); // Force buffer reload
                    }

                    for (col = 0; col < w; col++) { // For each pixel...
                        // Time to read more pixel data?

                        if (buffidx >= sizeof(sdbuffer)) { // Indeed
                            bmpFile.read(sdbuffer, sizeof(sdbuffer));
                            buffidx = 0; // Set index to beginning
                        }

                        // Convert pixel from BMP to TFT format, push to display
                        b = sdbuffer[buffidx++];
                        g = sdbuffer[buffidx++];
                        r = sdbuffer[buffidx++];
                        awColors[col] = tft.color565(r, g, b);
                    } // end pixel
                    tft.writeRect(0, row, w, 1, awColors);
                } // end scanline
                Serial.print(F(", loaded in ["));
                Serial.print(millis() - startTime);
                Serial.println("] ms");
            } // end goodBmp
        }
    }

    bmpFile.close();
    if (!goodBmp)
        Serial.println(F("BMP format not recognized."));
}

uint16_t read16(File &f) {
    uint16_t result;
    ((uint8_t *) &result)[0] = f.read(); // LSB
    ((uint8_t *) &result)[1] = f.read(); // MSB
    return result;
}

uint32_t read32(File &f) {
    uint32_t result;
    ((uint8_t *) &result)[0] = f.read(); // LSB
    ((uint8_t *) &result)[1] = f.read();
    ((uint8_t *) &result)[2] = f.read();
    ((uint8_t *) &result)[3] = f.read(); // MSB
    return result;
}

#endif // ENABLE_TFT
