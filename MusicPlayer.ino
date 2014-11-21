// Simple WAV file player example
//
// Requires the audio shield:
//   http://www.pjrc.com/store/teensy3_audio.html
//
// Data files to put on your SD card can be downloaded here:
//   http://www.pjrc.com/teensy/td_libs_AudioDataFiles.html
//
// This example code is in the public domain.

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>

// GUItool: begin automatically generated code
AudioPlaySdWav           playWav1;       //xy=154,78
AudioOutputI2S           i2s1;           //xy=334,89
AudioConnection          patchCord1(playWav1, 0, i2s1, 0);
AudioConnection          patchCord2(playWav1, 1, i2s1, 1);
AudioControlSGTL5000     sgtl5000_1;     //xy=240,153
// GUItool: end automatically generated code

static const char *soundFiles[] = { "CHUGDUB.WAV", "KEMFR10.WAV", "MINTY.WAV" };
int soundFilesCount = sizeof(soundFiles) / sizeof(char *);
int currentSoundFile = -1;
long startPlay = 0;
float currentVolume = 0.5;
void audioSetup() {
  // Audio connections require memory to work.  For more
  // detailed information, see the MemoryAndCpuUsage example
  AudioMemory(5);

  sgtl5000_1.enable();
  sgtl5000_1.volume(currentVolume);
}

void playFile(const char *filename)
{
  char filePath[30];
  sprintf(filePath, "MUSIC/%s", filename);
  if (SD.exists(filePath)) {
      Serial.print("Playing existing file: ");
      Serial.print(filePath);
      Serial.print(" at volume ");
      Serial.print(currentVolume);
  } else {
      Serial.print("file not found: ");
      Serial.println(filePath);
      return;
  }

  // Start playing the file.  This sketch continues to
  // run while the file plays.
  playWav1.play(filePath);
  startPlay = millis();
  // A brief delay for the library read WAV info
  delay(5);

  // Simply wait for the file to finish playing.
  while (playWav1.isPlaying() || (millis() - startPlay > 10000)) {
    // uncomment these lines if you audio shield
    // has the optional volume pot soldered
    float vol = analogRead(15);
    if (abs(vol / 1024 - currentVolume) > 0.05) {
        sgtl5000_1.volume(vol);
        if ((millis() - startPlay) % 10 == 0) {
            Serial.print("setting volume to ");
            Serial.println(currentVolume);
        }
    }

    delay(21);
  }
  playWav1.stop();
}

void playNextFile() {
    currentSoundFile ++;
    currentSoundFile = currentSoundFile % 3;
    Serial.print("current sound file index is ");
    Serial.println(currentSoundFile);
    playFile(soundFiles[currentSoundFile]);
}
