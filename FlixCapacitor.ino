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
#include "neopixel/NeoPixelManager.h"
#include "Joystick.h"
#include "print_helpers.h"


#ifdef ENABLE_7SD
#include <Adafruit_LEDBackpack.h>
#include <Adafruit_GFX.h>
Adafruit_7segment matrix = Adafruit_7segment();
#endif

namespace State {
    typedef enum SystemMode_e {
        Clock    = (1 << 0),
        LightsOn = (1 << 1),
        PhotosOn = (1 << 2),
        SetTime  = (1 << 3),
        Last     = (1 << 4)
    } SystemMode;
};

State::SystemMode mode = State::Clock;

// uncomment, then clean project, build and upload it to set the time to compile time.
//#define SET_TIME_TO_COMPILE
#ifdef SET_TIME_TO_COMPILE
#include "TeensyTimeManager.h"
#endif

#ifdef ENABLE_CLOCK
#include <Time.h>
#endif

#define JOYSTICK_BLOCKTIME_AFTER_ACTION_MS 500

#define TFT_DC  20
#define TFT_CS  21
#define SD_CS   10

#ifdef ENABLE_AUDIO_SD
#include "MusicPlayer.h"
#include "FileSystem.h"
FileSystem fileSystem((uint8_t) SD_CS);
#endif

time_t startTime;

#if defined(ENABLE_TFT) && defined(ENABLE_AUDIO_SD)
ILI9341_t3 tft = ILI9341_t3(TFT_CS, TFT_DC);
#endif

periodicCall ScreenReset   = {   2000, screenReset,            false };
periodicCall StatusTimer   = {  10002, status,                 true };
periodicCall JoystickTimer = {     20, readJoystick,           true };

#if defined(ENABLE_TFT) && defined(ENABLE_AUDIO_SD)
periodicCall ImageTimer    = {   6003, autoPlayPhotos,         false };
#endif
#ifdef ENABLE_AUDIO_SD
periodicCall VolumeTimer   = {     48, adjustVolume,           true };
#endif

#ifdef ENABLE_CLOCK
periodicCall ClockTimer    = {   1000, showClock,              true };
#endif

#ifdef ENABLE_NEOPIXEL
periodicCall NeoPixelShow  = {      5, neoPixelShow,           false };
periodicCall NeoPixelNext  = {   5000, neoPixelNext,           true };
#endif

periodicCall ShutOffTimer  ={ 3600000, shutOff,                true };


// don't forget to add a new timer to the array below!
periodicCall *timers[] = {
          &ScreenReset
        , &StatusTimer
		, &JoystickTimer
		, &ShutOffTimer
#if defined(ENABLE_TFT) && defined(ENABLE_AUDIO_SD)
		, &ImageTimer
#endif
#ifdef ENABLE_AUDIO_SD
        , &VolumeTimer
#endif
#ifdef ENABLE_CLOCK
        , &ClockTimer
#endif
#ifdef ENABLE_NEOPIXEL
        , &NeoPixelShow
        , &NeoPixelNext
#endif
};

periodicCall *executingTimer;

long lastMillis = 0, lastJoystickAction = 0, lastVolumeDisplayRedraw = 0;
bool ledOn = false, colonOn = false;
char stringBuffer[30];

uint8_t pinVolume = 15;
uint8_t pinX = A2;
uint8_t pinY = A3;
uint8_t pinButton = 8;
uint8_t pinNeoPixels = 2;
uint8_t pinSerialDisplay = 4;
uint8_t pinSerialDisplayReset = 3;

char buf[256];

Joystick joystick(pinX, pinY, pinButton);

#ifdef ENABLE_NEOPIXEL
NeoPixelManager neoPixelManager(4, pinNeoPixels);
#endif

#ifdef ENABLE_AUDIO_SD
MusicPlayer player(0.7);
FileList *tracks;
char trackPathName[FAT32_FILENAME_LENGTH * 2 + 2];
#endif

#ifdef ENABLE_TFT
char photoPathName[FAT32_FILENAME_LENGTH * 2 + 2];
FileList *photos;
#endif

void screenReset() {
    ScreenReset.active = false;
    resetScreen();
}
void activateTimer(periodicCall *timer, bool callNow) {
    timer->active = true;
    timer->lastCallMs = callNow ? 0 : millis() + timer->frequencyMs;
}
void deactivateTimer(periodicCall *timer) {
    timer->active = false;
}

void shutOff() {
    ImageTimer.active = false;
    NeoPixelShow.active = false;
    NeoPixelNext.active = false;

    displayModeChange((char *) "Stopping", (char *) "Auto Play...");

    if (player.isPlaying())
        player.stop();

    neoPixelManager.shutoff();

    activateTimer(&ScreenReset, false);
}

void nextState() {
    // disable any previous screen reset timers
    deactivateTimer(&ScreenReset);

    mode = (State::SystemMode) ((int) mode << 1);
    switch(mode) {
    case State::LightsOn:
        if (!NeoPixelShow.active) {
            neoPixelManager.begin();
            NeoPixelShow.active = true;
            NeoPixelNext.active = true;
            displayModeChange((char *) "Coming up next:", (char *)"Pretty blinkies!");
            activateTimer(&ScreenReset, false);
        }
        break;
    case State::PhotosOn:
        if (!ImageTimer.active) {
            ImageTimer.active = true;
            displayModeChange((char *)"Coming up next:", (char *)"Photo slideshow!");
            delay(500);
        }
        break;

    case State::SetTime:
        configureTime();
        break;
    case State::Last:
        mode = State::Clock;
        /* no break */
    case State::Clock:
        shutOff();
    }
}

void readJoystick() {
    if (millis() - lastJoystickAction > JOYSTICK_BLOCKTIME_AFTER_ACTION_MS) {
        long actionStart = millis();
		bool action = true;
		bool pressed = joystick.buttonPressed();
		if (pressed) {
		    nextState();
		} else if (joystick.yUp()) {
            #ifdef ENABLE_AUDIO_SD
			playTrack(NEXT);
		    Serial.println(">> UP");
            #endif
		} else if (joystick.yDown()) {
            #ifdef ENABLE_AUDIO_SD
            Serial.println(">> DOWN");
			if (player.isPlaying()) player.stop();
            #endif
		} else if (joystick.xForward()) {
            #if defined(ENABLE_TFT) && defined(ENABLE_AUDIO_SD)
			ImageTimer.lastCallMs = millis() + 3 * ImageTimer.frequencyMs;
			playImage(NEXT);
			Serial.println(">> FORWARD");
            #endif
		} else if (joystick.xBack()) {
            #if defined(ENABLE_TFT) && defined(ENABLE_AUDIO_SD)
			ImageTimer.lastCallMs = millis() + 3 * ImageTimer.frequencyMs;
			playImage(PREVIOUS);
			Serial.println(">> BACK");
            #endif
		} else {
			action = false;
		}

        if (action) {
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
    long uptime = now() - startTime;
    sprintf(buf, "[%d:%02d:%02d, %d/%d/%d] Uptime: %d days, %d hours, %d minutes | Memory Free: %5.2fKb | State:%d Lights:%s Photos:%s Music:%s Effect:%d",
            hour(), minute(), second(), month(), day(), year(),
            uptime  / (60 * 60 * 24),
            uptime / (60 * 60),
            uptime / 60,
            (double) 1.0 * FreeRamTeensy() / 1014.0,
            (int) mode,
            NeoPixelShow.active ? "on" : "off",
            ImageTimer.active ? "on" : "off",
            player.isPlaying() ? "on" : "off",
            (int) neoPixelManager.effects()->currentEffectIndex());
    Serial.println(buf);
}

#ifdef ENABLE_AUDIO_SD
void playTrack(direction direction) {
    playTrack(direction, 0);
}

void playRandomTrack() {
    if (fileSystem.hasInitialized() && tracks != NULL && !player.isPlaying()) {
        if (fileSystem.randomFileInList(tracks, trackPathName)) {
            Serial.print(F("Starting to play a random track ")); Serial.print(trackPathName);
            if (player.play(trackPathName)) {
                Serial.println(F(", started OK!"));
                displayNotice("Next Track:", trackPathName);
                activateTimer(&ScreenReset, false);
                delay(500);
            }
        }
    }
}

void playTrack(direction direction, int attempts) {
    if (fileSystem.hasInitialized() && tracks != NULL) {
        fileSystem.nextFileInList(tracks, trackPathName, direction);
        Serial.print(F("Starting to play next track ")); Serial.print(trackPathName);
        if (player.play(trackPathName)) {
            Serial.println(F(", started OK!"));
            displayNotice("Next Track:", trackPathName);
            activateTimer(&ScreenReset, false);
            delay(500);
        } else {
            if (attempts == 0) {
               Serial.println(F(", failed to start, trying one more time..."));
               displayError("Can't open file :(", trackPathName);
               activateTimer(&ScreenReset, false);
               playTrack(direction, attempts + 1);
            } else {
                Serial.println(F(", failed to start"));
            }
        }
    } else {
        Serial.println(F("SD Card not initialized, can't play track"));
        displayError("Unable to read SD Card", trackPathName);
        activateTimer(&ScreenReset, false);
    }
}

bool readSDCard() {
    if (!fileSystem.hasInitialized())
    	fileSystem.initSDCard();

    if (fileSystem.hasInitialized()) {
        if (photos == NULL) {
            photos = fileSystem.findFilesMatchingExtension((char *)"/PHOTOS", (char *)".BMP");
            for (int i = 0; i < photos->size; i++) {
                Serial.print(F("Found image file ")); Serial.println(photos->files[i]);
            }
        }
        delay(50);
        if (tracks == NULL) {
            tracks = fileSystem.findFilesMatchingExtension((char *)"/MUSIC", (char *)".WAV");
            for (int i = 0; i < tracks->size; i++) {
                Serial.print(F("Found audio file ")); Serial.println(tracks->files[i]);
            }
        }
    }
    return fileSystem.hasInitialized();
}

void adjustVolume() {
    float vol = abs(1.0 - 1.0 * analogRead(pinVolume) / 1024.0);
    if (player.setVolume(vol)) {
        Serial.print(F("Changed volume to "));
        Serial.println(player.volume());
        if (player.isPlaying() && lastVolumeDisplayRedraw - millis() > 500) {
            sprintf(buf, "%.0f%%", 100.0 * player.volume() / player.maxVolume());
            displayError("Volume:", buf);
            if (!ImageTimer.active)
                activateTimer(&ScreenReset, false);
            lastVolumeDisplayRedraw = millis();
        }
    }
}
#endif


#if defined(ENABLE_TFT) && defined(ENABLE_AUDIO_SD)
void autoPlayPhotos() {
    playImage(NEXT);
}

void displayNotice(char * title, char * message) {
    displayMessage(title, message, 20, 100, ILI9341_WHITE, tft.color565(0,  110,  160));
}
void displayModeChange(char * title, char * message) {
    displayMessage(title, message, 20, 100, ILI9341_WHITE, tft.color565(20, 80,  160));
}
void displayError(char * title, char * message) {
    displayMessage(title, message, 20, 100, ILI9341_WHITE, tft.color565(160, 60,  10));
}
void displayMessage(char *title, char *message, uint8_t x, uint8_t y, uint32_t color, uint32_t bgColor) {
    displayMessageWindow(x, y, bgColor);
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
    deactivateTimer(&ScreenReset);
    if (fileSystem.hasInitialized() && photos != NULL) {
        fileSystem.nextFileInList(photos, photoPathName, direction);
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

void displayTime(uint8_t hour, uint8_t minute) {
    colonOn = !colonOn;
#ifdef ENABLE_7SD
    int h = hour % 12;
    if (h == 0) h = 12;
    int m = minute;
    matrix.clear();
    if (h > 9)
        matrix.writeDigitNum(0, h / 10, false);
    matrix.writeDigitNum(1, h % 10, false);
    matrix.drawColon(colonOn);
    matrix.writeDigitNum(3, m / 10, false);
    matrix.writeDigitNum(4, m % 10, false);
    matrix.writeDisplay();
#else
    Serial.printf("Time now is %d:%02d:%02d on %02d/%02d/%04d\n", hour(), minute(), second(), day(), month(), year() );
#endif
}

void showClock() {
    displayTime(hour(), minute());
}

#endif

#ifdef ENABLE_NEOPIXEL
void neoPixelShow() {
    neoPixelManager.refreshEffect();
}
void neoPixelNext() {
    neoPixelManager.nextEffect();
}
#endif
/*__________________________________________________________________________________________*/

void setup() {

    Serial.begin(9600);
    joystick.begin();

    delay(3000);

#ifdef ENABLE_AUDIO_SD
    SPI.setMOSI(7);
    SPI.setSCK(14);
    pinMode(10, OUTPUT);
    pinMode(pinVolume, INPUT);
    photos = NULL;
    tracks = NULL;
    fileSystem.initSDCard();
    if (!fileSystem.hasInitialized()) {
    	ImageTimer.active = false;
    } else {
        readSDCard();
    }
#endif

#if defined(ENABLE_TFT) && defined(ENABLE_AUDIO_SD)
    tft.begin();
    tft.setRotation(3);
    resetScreen();
#endif

#ifdef ENABLE_AUDIO_SD
    player.begin();
    player.moarBass(true);
    adjustVolume();
#endif

#ifdef ENABLE_7SD
    pinMode(pinSerialDisplayReset, OUTPUT);
    matrix.begin(0x70);
    matrix.setBrightness(2);
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

    Serial.println("Setup finished, going into loop().");
    startTime = now();
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
    delay(2);
}
