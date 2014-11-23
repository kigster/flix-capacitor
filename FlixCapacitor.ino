/*
 * FlixCapacitor
 *
 *  Created on: Nov 13, 2014
 *      Author: Konstantin Gredeskoul
 *        Code: https://github.com/kigster
 *
 *  (c) 2014 All rights reserved, MIT License.
 */

#include <SPI.h>
#include <SD.h>
#include <ILI9341_t3.h>

#define TFT_DC  20
#define TFT_CS  21
#define SD_CS   10
ILI9341_t3 tft = ILI9341_t3(TFT_CS, TFT_DC);

typedef void (*timerCallback)(void);

typedef struct periodicCallStruct {
    uint16_t frequencyMs;
    uint32_t lastCallMs;
    timerCallback callback;
} periodicCall;

periodicCall ImageLoad = { 6000, 0, nextImage };
periodicCall VolumeAdjust = { 50, 0, adjustVolume };
periodicCall RamCheck = { 3000, 0, reportAvailableRAM };
periodicCall TrackSwitch= { 15000, 0, playNextFile};

periodicCall timers[] = { ImageLoad, VolumeAdjust, RamCheck, TrackSwitch };
periodicCall *executingTimer;

static const char *images[] = { "IMG_1940.bmp", "IMG_2196.bmp", "IMG_2271.bmp", "IMG_2297.bmp", "IMG_2475.bmp", "IMG_2483.bmp" };
uint8_t currentImageIndex = -1, totalImages = 6;
long lastMillis = 0;

uint8_t pinVolume = 15;
char stringBuffer[20];
Sd2Card card;
SdVolume volume;
SdFile root;
File bmpFile;
bool ledOn = false;
bool sdCardInitialized = false;

void reportAvailableRAM() {
    Serial.print("Free HEAP RAM is: ");
    Serial.print(1.0 * FreeRamTeensy() / 1014);
    Serial.println("Kb");
}

void nextImage() {
    char imageName[20];

    if (sdCardInitialized) {
        currentImageIndex++;
        currentImageIndex = currentImageIndex % totalImages;
        sprintf(imageName, "PHOTOS/%s", images[currentImageIndex]);
        Serial.print("Showing image ");
        Serial.println(imageName);
        bmpDraw(imageName, 0, 0);
    } else {
        Serial.println("SD Card not initialized, can't play image");
    }
}

/*__________________________________________________________________________________________*/

void setup() {
    SPI.setMOSI(7);
    SPI.setSCK(14);
    pinMode(10, OUTPUT);
    pinMode(pinVolume, INPUT);
    delay(10);
    Serial.begin(9600);

    audioSetup();

    tft.begin();
    tft.setRotation(3);
    resetScreen();

    delay(3000);
    sdCardInitialized = initSDCard();
    adjustVolume();
}

void loop() {
    for (uint8_t i = 0; i < sizeof(timers) / sizeof(periodicCall); i++ ) {
        periodicCall *t = &timers[i];
        if (t->frequencyMs == 0) {
            continue;
        }
        uint32_t now = millis();
        if (now - t->lastCallMs > t->frequencyMs) {
            executingTimer = t;
            t->callback();
            executingTimer = NULL;
            t->lastCallMs = now;
        }
    }
    delay(10);
}

