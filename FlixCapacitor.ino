/*
 * KemBox
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
#include <SimpleTimer.h>


#define TFT_DC  20
#define TFT_CS  21
#define SD_CS   10
ILI9341_t3 tft = ILI9341_t3(TFT_CS, TFT_DC);

//Sparkfun7SD_Serial display7s(8);
//TeensyRTC rtc;
SimpleTimer timer(1);

static const char *images[] = { "IMG_1940.bmp", "IMG_2196.bmp", "IMG_2271.bmp", "IMG_2297.bmp", "IMG_2475.bmp", "IMG_2483.bmp" };
uint8_t currentImageIndex = -1, totalImages = sizeof(images) / sizeof(char *);
char imageName[40];
long lastMillis = 0;

uint8_t pinLED = 13;
char stringBuffer[20];
Sd2Card card;
SdVolume volume;
SdFile root;
File bmpFile;
bool ledOn = false;
bool sdCardInitialized = false;

void toggleLED(int timerId) {
    ledOn = !ledOn;
    digitalWrite(pinLED, ledOn ? HIGH : LOW);
}

void nextImage(int timerId) {
    if (sdCardInitialized) {
        currentImageIndex++;
        currentImageIndex = currentImageIndex % totalImages;
        sprintf(imageName, "PHOTOS/%s",images[currentImageIndex] );
        showBitMaps(imageName);
    }
}

void nextAudioFile(int timerId) {
    playNextFile();
}

void showBitMaps(char * filename) {
    bmpDrawWriteRect(filename, 0, 0);
}

/*__________________________________________________________________________________________*/

void setup() {
    SPI.setMOSI(7);
    SPI.setSCK(14);
    pinMode(10, OUTPUT);
    delay(10);
    Serial.begin(9600);

    audioSetup();

    tft.begin();
    tft.setRotation(3);
    resetScreen();

    delay(10);

    sdCardInitialized = initSDCard();
//    timer.setInterval(5000, nextImage);
    timer.setInterval(1000, toggleLED);
    timer.setInterval(20000, nextAudioFile);
//    nextImage(0);
    nextAudioFile(0);
}

void loop() {
    delay(10);
    timer.run();
}

