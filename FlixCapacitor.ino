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
#include <Sparkfun7SD_Serial.h>
#include <ILI9341_t3.h>


#include "FlixCapacitor.h"
#include "neopixel/NeoPixelManager.h"
#include "Joystick.h"
#include "print_helpers.h"

// uncomment, then clean project, build and upload it to set the time to compile time.
// #define SET_TIME_TO_COMPILE
#ifdef SET_TIME_TO_COMPILE
#include "TeensyTimeManager.h"
#endif

#ifdef ENABLE_CLOCK
#include <Time.h>
#endif

#ifdef ENABLE_AUDIO_SD
#include "MusicPlayer.h"
#endif

#define JOYSTICK_BLOCKTIME_AFTER_ACTION_MS 500

#define TFT_DC  20
#define TFT_CS  21
#define SD_CS   10

#if defined(ENABLE_TFT) && defined(ENABLE_AUDIO_SD)

ILI9341_t3 tft = ILI9341_t3(TFT_CS, TFT_DC);
#endif

periodicCall StatusTimer   = {  10002, status,                 true };
periodicCall JoystickTimer = {    202, readJoystick,           true };

#if defined(ENABLE_TFT) && defined(ENABLE_AUDIO_SD)
periodicCall ImageTimer    = {   6003, autoPlayPhotos,         true};
#endif
#ifdef ENABLE_AUDIO_SD
periodicCall TrackTimer    = {  10001, autoPlayMusic,          false};
periodicCall VolumeTimer   = {     48, adjustVolume,           true };
#endif

#ifdef ENABLE_CLOCK
periodicCall ClockTimer    = {   1000, showClock,              true };
#endif

#ifdef ENABLE_NEOPIXEL
periodicCall NeoPixelShow  = {     10, neoPixelShow,           true };
periodicCall NeoPixelNext  = {   5000, neoPixelNext,           true };
#endif

#ifdef ENABLE_7SD
periodicCall Reset7SD      = {  30000, resetSerialDisplay,     true };
#endif


// don't forget to add a new timer to the array below!
periodicCall *timers[] = {
          &StatusTimer
		, &JoystickTimer
#if defined(ENABLE_TFT) && defined(ENABLE_AUDIO_SD)

		, &ImageTimer
#endif
#ifdef ENABLE_AUDIO_SD
        , &VolumeTimer
        , &TrackTimer
#endif
#ifdef ENABLE_CLOCK
        , &ClockTimer
#endif
#ifdef ENABLE_NEOPIXEL
        , &NeoPixelShow
        , &NeoPixelNext
#endif
#ifdef ENABLE_7SD
		, &Reset7SD
#endif
};

periodicCall *executingTimer;

FileList *photos, *tracks;

char trackPathName[FAT32_FILENAME_LENGTH * 2];
char photoPathName[FAT32_FILENAME_LENGTH * 2];

long lastMillis = 0, lastJoystickAction = 0;
bool ledOn = false, colonOn = false;
bool sdCardInitialized = false;
char stringBuffer[30];

uint8_t pinVolume = 15;
uint8_t pinX = A2;
uint8_t pinY = A3;
uint8_t pinButton = 8;
uint8_t pinNeoPixels = 2;
uint8_t pinSerialDisplay = 4;
uint8_t pinSerialDisplayReset = 3;

char buf[80];

Joystick joystick(pinX, pinY, pinButton);

#ifdef ENABLE_NEOPIXEL
NeoPixelManager neoPixelManager(4, pinNeoPixels);
#endif

#ifdef ENABLE_AUDIO_SD
MusicPlayer player(0.7);
#endif

#ifdef ENABLE_7SD
Sparkfun7SD_Serial display(pinSerialDisplay);

void resetSerialDisplay(){
    digitalWrite(pinSerialDisplayReset, LOW); // keep high.  GND resets the display.
    delay(10);
    digitalWrite(pinSerialDisplayReset, HIGH); // keep high.  GND resets the display.
    delay(10);
}
#endif


uint32_t FreeRamTeensy() { // for Teensy 3.0
    uint32_t stackTop;
    uint32_t heapTop;

    // current position of the stack.
    stackTop = (uint32_t) &stackTop;

    // current position of heap.
    void* hTop = malloc(1);
    heapTop = (uint32_t) hTop;
    free(hTop);

    // The difference is the free, available ram.
    return stackTop - heapTop;
}

void readJoystick() {
    if (millis() - lastJoystickAction > JOYSTICK_BLOCKTIME_AFTER_ACTION_MS) {
        long actionStart = millis();
		bool action = true;
		bool pressed = joystick.buttonPressed();
		if (pressed) {
#ifdef ENABLE_AUDIO_SD
			if (player.isPlaying()) {
				player.stop();
			} else {
				playTrack(CURRENT);
			}
#endif
		} else if (joystick.readY() > 0.98) {
#ifdef ENABLE_AUDIO_SD
			playTrack(NEXT);
#endif
		} else if (joystick.readY() < 0.02) {
#ifdef ENABLE_AUDIO_SD
			playTrack(PREVIOUS);
#endif
		} else if (joystick.readX() > 0.98) {
#if defined(ENABLE_TFT) && defined(ENABLE_AUDIO_SD)

			ImageTimer.lastCallMs = millis() + 3 * ImageTimer.frequencyMs;
			playImage(NEXT);
#endif
		} else if (joystick.readX() < 0.02) {
#if defined(ENABLE_TFT) && defined(ENABLE_AUDIO_SD)

			ImageTimer.lastCallMs = millis() + 3 * ImageTimer.frequencyMs;
			playImage(PREVIOUS);
#endif
		} else {
			action = false;
		}

        if (action) {
            //printv("Joystick Action Detected: X = ", joystick.readX());
            Serial.print(F("Joystick Action Detected: X = "));
            Serial.print(joystick.readX());
            Serial.print(F(", Y = "));
            Serial.print(joystick.readY());
            Serial.print(F(", button is "));
            pressed ? Serial.println("PRESSED") : Serial.println("Not Pressed");
            lastJoystickAction = actionStart;
        }
    }
}

void status() {
    printv(F("Free HEAP RAM (Kb): "), 1.0 * FreeRamTeensy() / 1014);
    sprintf(buf, "%d:%02d:%02d, %d/%d/%d",hour(), minute(), second(), month(), day(), year());
    printv(F("Current time is: "), buf);
}

#ifdef ENABLE_AUDIO_SD
void autoPlayMusic() {
    if (!player.isPlaying())
        playTrack(NEXT);
}

void playTrack(direction direction) {
    playTrack(direction, 0);
}

void playTrack(direction direction, int attempts) {
    if (sdCardInitialized) {
        nextFileInList(tracks, trackPathName, direction);
        Serial.print(F("Starting to play next track ")); Serial.print(trackPathName);
    #ifdef ENABLE_AUDIO_SD
        if (player.play(trackPathName)) {
            Serial.println(F(", started OK!"));
            displayMessage((char *)"Next Track:", trackPathName, 20, 100, ILI9341_WHITE);
            delay(500);
        } else {
            if (attempts == 0) {
               Serial.println(F(", failed to start, trying one more time..."));
               displayMessage((char *)"Can't open file :(", trackPathName, 20, 100, neoPixelManager.color(127, 10, 10));
               playTrack(direction, attempts + 1);
            } else {
                Serial.println(F(", failed to start"));
            }
        }
    #endif
    } else {
        Serial.println(F("SD Card not initialized, can't play track"));
        readSDCard();
    }
}

bool readSDCard() {
    sdCardInitialized = initSDCard();
    if (sdCardInitialized) {
        photos = findFilesMatchingExtension((char *)"/PHOTOS", (char *)".BMP");
        tracks = findFilesMatchingExtension((char *)"/MUSIC", (char *)".WAV");
        for (int i = 0; i < photos->size; i++) {
            Serial.print(F("Found image file ")); Serial.println(photos->files[i]);
        }
        for (int i = 0; i < tracks->size; i++) {
            Serial.print(F("Found audio file ")); Serial.println(tracks->files[i]);
        }
    }
    return sdCardInitialized;
}

void adjustVolume() {
    float vol = abs(1.0 - 1.0 * analogRead(pinVolume) / 1024.0);
    if (player.setVolume(vol)) {
        Serial.print(F("Changed volume to "));
        Serial.println(player.volume());
    }
}
#endif


#if defined(ENABLE_TFT) && defined(ENABLE_AUDIO_SD)
void autoPlayPhotos() {
    playImage(NEXT);
}

void displayMessage(char *title, char *message, uint8_t x, uint8_t y, uint32_t color) {
    displayMessageWindow(x, y, neoPixelManager.color(20, 20, 80));
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


void playImage(direction direction) {
    if (sdCardInitialized) {
        nextFileInList(photos, photoPathName, direction);
        Serial.print(F("Starting to play next photo")); Serial.println(photoPathName);
        bmpDraw(photoPathName, 0, 0);
    } else {
        Serial.println(F("SD Card not initialized, can't play image"));
        readSDCard();
    }
}

#endif

#ifdef ENABLE_CLOCK
time_t getTeensy3Time() {
    return Teensy3Clock.get();
}

void showClock() {
    colonOn = !colonOn;
#ifdef ENABLE_7SD
    display.printTime(hour(), minute(), colonOn);
#else
    Serial.printf("Time now is %d:%02d:%02d on %02d/%02d/%04d\n", hour(), minute(), second(), day(), month(), year() );
#endif
}

#endif

#ifdef ENABLE_NEOPIXEL
void neoPixelShow() {
    neoPixelManager.refreshEffect();
}
void neoPixelNext() {
    neoPixelManager.nextEffect();
    printv("Next effect # is ",  neoPixelManager.effects()->currentEffectIndex());
}
#endif
/*__________________________________________________________________________________________*/

void setup() {

    Serial.begin(9600);
    joystick.begin();

    delay(3000);

#ifdef ENABLE_AUIDIO_SD
    SPI.setMOSI(7);
    SPI.setSCK(14);
    pinMode(10, OUTPUT);
    pinMode(pinVolume, INPUT);
    sdCardInitialized = readSDCard();
    if (!sdCardInitialized) {
    	ImageTimer.active = false;
    	TrackTimer.active = false;
    }
#endif

#if defined(ENABLE_TFT) && defined(ENABLE_AUDIO_SD)

    tft.begin();
    tft.setRotation(3);
    playImage(CURRENT);
    resetScreen();
#endif

#ifdef ENABLE_AUDIO_SD
    player.begin();
    player.moarBass(true);
    adjustVolume();
#endif

#ifdef ENABLE_7SD
    pinMode(pinSerialDisplayReset, OUTPUT);
    display.begin();
    resetSerialDisplay();
#endif

#ifdef ENABLE_CLOCK
#ifdef SET_TIME_TO_COMPILE
    TeensyTimeManager *timeManager = new TeensyTimeManager();
    bool result = timeManager->setTimeToCompileTime();
    Serial.println(result ? "Successful Time Set" : "Could not set time");
    setSyncProvider(getTeensy3Time);
    if (timeStatus() != timeSet) {
        Serial.println("Unable to sync with the RTC");
    } else {
        Serial.println("RTC has set the system time");
    }
#else
    setSyncProvider(getTeensy3Time);
#endif
#endif

#ifdef ENABLE_NEOPIXEL
    neoPixelManager.begin();
#endif

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

