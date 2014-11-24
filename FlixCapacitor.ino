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
#include "Joystick.h"

#ifdef MUSIC_ENABLED
#include "MusicPlayer.h"
#endif

#define TFT_DC  20
#define TFT_CS  21
#define SD_CS   10
ILI9341_t3 tft = ILI9341_t3(TFT_CS, TFT_DC);

periodicCall ImageTimer    = {   6000, autoPlayPhotos,         true};
periodicCall TrackTimer    = { 120000, autoPlayMusic,          false};
periodicCall VolumeTimer   = {     50, adjustVolume,           true };
periodicCall RamCheckTimer = {  10000, reportAvailableRAM,     true };
periodicCall JoystickTimer = {    200, readJoystick,           true };

periodicCall *timers[] = { &ImageTimer, &VolumeTimer, &RamCheckTimer, &TrackTimer, &JoystickTimer };
periodicCall *executingTimer;

FileList *photos, *tracks;

char trackPathName[FAT32_FILENAME_LENGTH * 2];
char photoPathName[FAT32_FILENAME_LENGTH * 2];

long lastMillis = 0, lastJoystickAction = 0, lastJoystickPrint = 0;
bool ledOn = false;
bool sdCardInitialized = false;
char stringBuffer[30];

uint8_t pinVolume = 15;
uint8_t pinX = A2;
uint8_t pinY = A3;
uint8_t pinButton = 8;
Joystick joystick(pinX, pinY, pinButton);


#ifdef MUSIC_ENABLED
MusicPlayer player;
#endif

void readJoystick() {
    bool pressed = joystick.buttonPressed();

    if (millis() - lastJoystickPrint > 500) {
        lastJoystickPrint = millis();
        Serial.print("Joystick: X = ");
        Serial.print(joystick.readX());
        Serial.print(", Y = ");
        Serial.print(joystick.readY());
        Serial.print(", button is ");
        pressed ? Serial.println("PRESSED") : Serial.println("Not Pressed");
    }

    if (millis() - lastJoystickAction > 250) {
        if (pressed) {
            lastJoystickAction = millis();
            if (player.isPlaying()) {
                player.stop();
            } else {
                playTrack(CURRENT);
            }
        } else if (joystick.readY() > 0.8) {
            lastJoystickAction = millis();
            playTrack(NEXT);
        } else if (joystick.readY() < 0.47) {
            lastJoystickAction = millis();
            playTrack(PREVIOUS);
        } else if (joystick.readX() > 0.95) {
            lastJoystickAction = millis();
            ImageTimer.lastCallMs = millis() + 3 * ImageTimer.frequencyMs;
            playImage(NEXT);
        } else if (joystick.readX() < 0.05) {
            lastJoystickAction = millis();
            ImageTimer.lastCallMs = millis() + 3 * ImageTimer.frequencyMs;
            playImage(PREVIOUS);
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
    playTrack(NEXT);
}

void displayMessage(char *title, char *message, uint8_t x, uint8_t y, uint32_t color, uint8_t shift) {
    tft.setCursor(x + shift, y + shift);
    tft.setTextSize(2);
    tft.setTextColor(color);
    for (int i = 0; i < 3; i++) {
        tft.drawRoundRect(x - 10 + shift + i, y - 10 + shift + i, tft.width() - 2 * (x - 10), tft.height() - 2 * (y - 10), 5, color);
    }
    tft.print(title);
    tft.setCursor(x + shift, y + 20 + shift);
    tft.print(message);
}

void playTrack(direction direction) {
    if (sdCardInitialized) {
        nextFileInList(tracks, trackPathName, direction);
        Serial.print("Starting to play next track "); Serial.print(trackPathName);
    #ifdef MUSIC_ENABLED
        if (player.play(trackPathName)) {
            Serial.println(", started OK!");
            displayMessage((char *)"Next Track:", trackPathName, 20, 100, ILI9341_BLACK, 2);
            displayMessage((char *)"Next Track:", trackPathName, 20, 100, ILI9341_WHITE, 0);
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
    float vol = abs(1.0 - 1.0 * analogRead(pinVolume) / 1024.0);
    #ifdef MUSIC_ENABLED
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

