#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>

AudioPlaySdWav playWave;
AudioOutputI2S i2s1;
AudioConnection patchCordRight(playWave, 0, i2s1, 0);
AudioConnection patchCordLeft(playWave, 1, i2s1, 1);
AudioControlSGTL5000 sgtl5000;

long startPlay = 0;
float currentVolume = 0.25;
float maxVolume = 0.6;

void audioSetup() {
    // Audio connections require memory to work.  For more
    // detailed information, see the MemoryAndCpuUsage example
    AudioMemory(20);

    sgtl5000.enable();
    sgtl5000.volume(currentVolume);
}

void playFile(const char *filename) {
    stop();

    if (SD.exists((char *) filename)) {
        Serial.print("Playing existing file: ");
        Serial.print(filename);
        Serial.print(" at volume ");
        Serial.println(currentVolume);
    } else {
        Serial.print("File not found: ");
        Serial.println(filename);
        return;
    }

    // Start playing the file.  This sketch continues to
    // run while the file plays.
    playWave.play(filename);
    startPlay = millis();
    // A brief delay for the library read WAV info
    delay(5);
}

void stop() {
    if (playWave.isPlaying()) {
        playWave.stop();
        long playedFor = (millis() - startPlay) / 1000;
        Serial.print("track was playing for (sec): ");
        Serial.println(playedFor);
    }
}

void adjustVolume() {
    float vol = abs(maxVolume - 1.0 * analogRead(pinVolume) / 1024.0 * maxVolume);
    if (abs(vol - currentVolume) > 0.02) {
        currentVolume = vol;
        sgtl5000.volume(currentVolume);
        Serial.print("Changing volume to [0-100]: ");
        Serial.println(currentVolume * 100 / maxVolume);
    }
}
