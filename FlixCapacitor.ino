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
#include "FlixCapacitor.h"

#define TFT_DC  20
#define TFT_CS  21
#define SD_CS   10
ILI9341_t3 tft = ILI9341_t3(TFT_CS, TFT_DC);

periodicCall ImageLoad = { 6000, 0, nextImage };
periodicCall TrackSwitch= { 120000, 0, nextTrack};
periodicCall VolumeAdjust = { 50, 0, adjustVolume };
periodicCall RamCheck = { 3000, 0, reportAvailableRAM };

periodicCall timers[] = { ImageLoad, VolumeAdjust, RamCheck, TrackSwitch };
periodicCall *executingTimer;

FileList *photos, *tracks;
char trackPathName[FAT32_FILENAME_LENGTH * 2];
char photoPathName[FAT32_FILENAME_LENGTH * 2];

Sd2Card card;
SdVolume volume;
SdFile root;
File bmpFile;

long lastMillis = 0;
bool ledOn = false;
bool sdCardInitialized = false;
uint8_t pinVolume = 15;
char stringBuffer[20];


void reportAvailableRAM() {
    Serial.print("Free HEAP RAM is: ");
    Serial.print(1.0 * FreeRamTeensy() / 1014);
    Serial.println("Kb");
}


void nextTrack() {
    if (sdCardInitialized) {
        nextFileInList(tracks, trackPathName);
        Serial.print("Starting to play next track "); Serial.println(trackPathName);
        playFile(trackPathName);
    } else {
        Serial.println("SD Card not initialized, can't play track");
        readSDCard();
    }
}

void nextImage() {
    if (sdCardInitialized) {
        nextFileInList(photos, photoPathName);
        Serial.print("Starting to play next photo"); Serial.println(photoPathName);
        bmpDraw(photoPathName, 0, 0);
    } else {
        Serial.println("SD Card not initialized, can't play image");
        readSDCard();
    }
}

void readSDCard() {
    sdCardInitialized = initSDCard();
    if (sdCardInitialized) {
        photos = findFilesMatchingExtension("/PHOTOS", ".BMP");
        tracks = findFilesMatchingExtension("/MUSIC", ".WAV");

        for (int i = 0; i < photos->size; i++) {
            Serial.print("Found image file "); Serial.println(photos->files[i]);
        }
        for (int i = 0; i < tracks->size; i++) {
            Serial.print("Found audio file "); Serial.println(tracks->files[i]);
        }
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

    nextImage();
    nextTrack();

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

