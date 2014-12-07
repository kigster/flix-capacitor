/*
 * MusicPlayer.cpp
 *
 *  Created on: Nov 23, 2014
 *      Author: Konstantin Gredeskoul
 *        Code: https://github.com/kigster
 *
 *  (c) 2014 All rights reserved, MIT License.
 */

#include "FlixCapacitor.h"

#ifdef ENABLE_AUDIO_SD

#include "MusicPlayer.h"

AudioPlaySdWav playWave;
AudioOutputI2S i2s1;
AudioConnection patchCordRight(playWave, 0, i2s1, 0);
AudioConnection patchCordLeft(playWave, 1, i2s1, 1);
AudioControlSGTL5000 sgtl5000;

MusicPlayer::MusicPlayer() {
    _maxVolume = MUSIC_PLAYER_DEFAULT_MAX_VOLUME;
    _volume = 0.5 * _maxVolume;
    _on = false;
}

MusicPlayer::MusicPlayer(float maxVolume) {
    _maxVolume = maxVolume;
    if (_maxVolume > 1.0 || _maxVolume < 0)
        _maxVolume = MUSIC_PLAYER_DEFAULT_MAX_VOLUME;
    _volume = 0.5 * _maxVolume;
    _on = false;
}

void MusicPlayer::moarBass(bool enabled) {
    enabled ? sgtl5000.enhanceBassEnable() : sgtl5000.enhanceBassDisable();
}

void MusicPlayer::begin() {
    AudioMemory(MUSIC_PLAYER_DEFAULT_MEMORY);
    sgtl5000.enable();
}

void MusicPlayer::turnOn() {
    if (!_on) {
        delay(100);
        sgtl5000.audioPostProcessorEnable();
        delay(100);
        sgtl5000.volume(_volume);
        sgtl5000.enhanceBassEnable();
        sgtl5000.muteHeadphone();
        sgtl5000.unmuteLineout();
        _on = true;
    }
}

void MusicPlayer::turnOff() {
    if (_on) {
        sgtl5000.muteLineout();
    }

    _on = false;
}

bool MusicPlayer::play(const char *filename) {
    turnOn();

    if (!SD.exists((char *) filename)) {
        return false;
    }
    if (playWave.isPlaying())
        playWave.stop();

    playWave.play(filename);
    // A brief delay for the library read WAV info
    delay(10);
    return playWave.isPlaying();
}

bool MusicPlayer::isPlaying() {
    return (playWave.isPlaying());
}

void MusicPlayer::stop() {
    if (playWave.isPlaying()) {
        playWave.stop();
        turnOff();
    }
}


void MusicPlayer::changeBassVolume(bool up) {
    _bassVolume = _bassVolume + (up ? 0.1 : -0.1);
    if (_bassVolume > _maxVolume) _bassVolume = _maxVolume;
    if (_bassVolume < 0) _bassVolume = 0;
    sgtl5000.enhanceBass(_volume, _bassVolume, 0, 1);
    Serial.print("volume is at "); Serial.print(_volume);
    Serial.print(", bassVolume is at "); Serial.println(_bassVolume);
}

void MusicPlayer::setBassVolume(float bassVolume) {
    if (bassVolume > 1.0 || bassVolume < 0)
        return;

    bassVolume = bassVolume * _maxVolume;
    if (abs(bassVolume - _bassVolume) > 0.02) {
        _bassVolume = bassVolume;
        sgtl5000.enhanceBass(_volume, _bassVolume, 0, 2);
    }
}

bool MusicPlayer::setVolume(float volume) {
    if (volume > 1.0 || volume < 0)
        return false;

    volume = volume * _maxVolume;
    if (abs(volume - _volume) > 0.02) {
        _volume = volume;
        sgtl5000.volume(_volume);
        sgtl5000.enhanceBass(_volume, _bassVolume, 0, 3);
        return true;
    } else
        return false;
}

void MusicPlayer::setMaxVolume(float volume) {
    if (volume <= 1.0 && volume >= 0)
        _maxVolume = volume;
}

float MusicPlayer::volume() {
    return _volume;
}

#endif
