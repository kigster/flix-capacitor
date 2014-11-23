
#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>

AudioPlaySdWav playWave;
AudioOutputI2S i2s1;
AudioConnection patchCordRight(playWave, 0, i2s1, 0);
AudioConnection patchCordLeft(playWave, 1, i2s1, 1);
AudioControlSGTL5000 sgtl5000;

static const char *soundFiles[] = { "CHUGDUB.WAV", "KEMFR10.WAV", "MINTY.WAV" };
int soundFilesCount = 3;
int currentSoundFile = -1;
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

void playNextFile() {
    currentSoundFile++;
    currentSoundFile = currentSoundFile % soundFilesCount;
    Serial.print("current sound file index is ");
    Serial.println(currentSoundFile);
    playFile(soundFiles[currentSoundFile]);
}

void playFile(const char *filename) {
    stop();

    char filePath[30];
    sprintf(filePath, "MUSIC/%s", filename);

    if (SD.exists(filePath)) {
        Serial.print("Playing existing file: ");
        Serial.print(filePath);
        Serial.print(" at volume ");
        Serial.println(currentVolume);
    } else {
        Serial.print("File not found: ");
        Serial.println(filePath);
        return;
    }

    // Start playing the file.  This sketch continues to
    // run while the file plays.
    playWave.play(filePath);
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
