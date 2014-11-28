/*
 * FlixCapacitor
 *
 *  Created on: Nov 13, 2014
 *      Author: Konstantin Gredeskoul
 *        Code: https://github.com/kigster
 *
 *  (c) 2014 All rights reserved, MIT License
 *
 */

#include <SPI.h>
#include <SD.h>
#include <ILI9341_t3.h>
#include "FlixCapacitor.h"
#include "Joystick.h"

#ifdef MUSIC_ENABLED
#include "MusicPlayer.h"
#endif

#define JOYSTICK_BLOCKTIME_AFTER_ACTION_MS 500

#define TFT_DC  20
#define TFT_CS  21
#define SD_CS   10
ILI9341_t3 tft = ILI9341_t3(TFT_CS, TFT_DC);

periodicCall ImageTimer    = {   6003, autoPlayPhotos,         true};
periodicCall TrackTimer    = {  10001, autoPlayMusic,          false};
periodicCall VolumeTimer   = {     48, adjustVolume,           true };
periodicCall RamCheckTimer = {  10002, reportAvailableRAM,     true };
periodicCall JoystickTimer = {    202, readJoystick,           true };

periodicCall *timers[] = { &ImageTimer, &VolumeTimer, &RamCheckTimer, &TrackTimer, &JoystickTimer };
periodicCall *executingTimer;

FileList *photos, *tracks;

char trackPathName[FAT32_FILENAME_LENGTH * 2];
char photoPathName[FAT32_FILENAME_LENGTH * 2];

long lastMillis = 0, lastJoystickAction = 0;
bool ledOn = false;
bool sdCardInitialized = false;
char stringBuffer[30];

uint8_t pinVolume = 15;
uint8_t pinX = A2;
uint8_t pinY = A3;
uint8_t pinButton = 8;

Joystick joystick(pinX, pinY, pinButton);


#ifdef MUSIC_ENABLED
MusicPlayer player(0.7);
#endif


void readJoystick() {
    if (millis() - lastJoystickAction > JOYSTICK_BLOCKTIME_AFTER_ACTION_MS) {
        long actionStart = millis();
        bool action = true;
        bool pressed = joystick.buttonPressed();
        if (pressed) {
            if (player.isPlaying()) {
                player.stop();
            } else {
                playTrack(CURRENT);
            }
        } else if (joystick.readY() > 0.98) {
            playTrack(NEXT);
        } else if (joystick.readY() < 0.02) {
            playTrack(PREVIOUS);
        } else if (joystick.readX() > 0.98) {
            ImageTimer.lastCallMs = millis() + 3 * ImageTimer.frequencyMs;
            playImage(NEXT);
        } else if (joystick.readX() < 0.02) {
            ImageTimer.lastCallMs = millis() + 3 * ImageTimer.frequencyMs;
            playImage(PREVIOUS);
        } else {
            action = false;
        }

        if (action) {
            Serial.print("Joystick Action Detected: X = ");
            Serial.print(joystick.readX());
            Serial.print(", Y = ");
            Serial.print(joystick.readY());
            Serial.print(", button is ");
            pressed ? Serial.println("PRESSED") : Serial.println("Not Pressed");
            lastJoystickAction = actionStart;
        }
    }
}

void reportAvailableRAM() {
    Serial.print("Free HEAP RAM is: ");
    Serial.print(1.0 * FreeRamTeensy() / 1014);
    Serial.println("Kb");
}

void autoPlayPhotos() {
    playImage(NEXT);
}

void autoPlayMusic() {
    if (!player.isPlaying())
        playTrack(NEXT);
}

void displayMessage(char *title, char *message, uint8_t x, uint8_t y, uint32_t color) {
    displayMessageWindow(x, y, 0x0245);
    displayMessageWithShadow(title, message, x, y, color, 0);
}

void displayMessageWithShadow(char *title, char *message, uint8_t x, uint8_t y, uint32_t color, uint8_t shift) {
    if (shift == 0) {
        // call ourselves recursively to draw a tiny shadow
        displayMessageWithShadow(title, message, x, y, ILI9341_BLACK, 2);
    }
    tft.setCursor(x + shift, y + shift);
    tft.setTextSize(2);
    tft.setTextColor(color);
    tft.print(title);
    tft.setCursor(x + shift, y + 20 + shift);
    tft.print(message);
}

void displayMessageWindow(uint8_t x, uint8_t y, uint32_t color) {
    for (int i = 0; i < 2; i++) {
        tft.drawRoundRect(x - 10 + i , y - 10 + i, tft.width() - 2 * (x - 10), tft.height() - 2 * (y - 10), 5, ILI9341_BLACK);
    }
    tft.fillRoundRect(x - 10, y - 10, tft.width() - 2 * (x - 10), tft.height() - 2 * (y - 10), 5, color);
}

void playTrack(direction direction) {
    if (sdCardInitialized) {
        nextFileInList(tracks, trackPathName, direction);
        Serial.print("Starting to play next track "); Serial.print(trackPathName);
    #ifdef MUSIC_ENABLED
        if (player.play(trackPathName)) {
            Serial.println(", started OK!");
            displayMessage((char *)"Next Track:", trackPathName, 20, 100, ILI9341_WHITE);
            delay(500);
        } else {
            Serial.println(", failed to start");
        }
    #endif
    } else {
        Serial.println("SD Card not initialized, can't play track");
        readSDCard();
    }
}

void playImage(direction direction) {
    if (sdCardInitialized) {
        nextFileInList(photos, photoPathName, direction);
        Serial.print("Starting to play next photo"); Serial.println(photoPathName);
        bmpDraw(photoPathName, 0, 0);
    } else {
        Serial.println("SD Card not initialized, can't play image");
        readSDCard();
    }
}

bool readSDCard() {
    sdCardInitialized = initSDCard();
    if (sdCardInitialized) {
        photos = findFilesMatchingExtension((char *)"/PHOTOS", (char *)".BMP");
        tracks = findFilesMatchingExtension((char *)"/MUSIC", (char *)".WAV");
        for (int i = 0; i < photos->size; i++) {
            Serial.print("Found image file "); Serial.println(photos->files[i]);
        }
        for (int i = 0; i < tracks->size; i++) {
            Serial.print("Found audio file "); Serial.println(tracks->files[i]);
        }
    }
    return sdCardInitialized;
}

void adjustVolume() {
    #ifdef MUSIC_ENABLED
    float vol = abs(1.0 - 1.0 * analogRead(pinVolume) / 1024.0);
    if (player.setVolume(vol)) {
        Serial.print("Changed volume to ");
        Serial.println(player.volume());
    }
    #endif
}

/*__________________________________________________________________________________________*/

void setup() {
    SPI.setMOSI(7);
    SPI.setSCK(14);
    pinMode(10, OUTPUT);
    pinMode(pinVolume, INPUT);
    delay(10);
    Serial.begin(9600);

    delay(3000);
    sdCardInitialized = readSDCard();

    tft.begin();
    tft.setRotation(3);
    resetScreen();

#ifdef MUSIC_ENABLED
    player.begin();
    player.moarBass(true);
#endif
    joystick.begin();

    playImage(CURRENT);
    adjustVolume();
}

void loop() {
    for (uint8_t i = 0; i < sizeof(timers) / sizeof(periodicCall *); i++ ) {
        periodicCall *t = timers[i];

        if (!t->active) continue;

        uint32_t now = millis();
        if (now > t->lastCallMs && (now - t->lastCallMs) > t->frequencyMs) {
            executingTimer = t;
            t->callback();
            executingTimer = NULL;
            t->lastCallMs = now;
        }
    }
    delay(10);
}

