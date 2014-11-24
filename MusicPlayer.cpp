/*
 * MusicPlayer.cpp
 *
 *  Created on: Nov 23, 2014
 *      Author: Konstantin Gredeskoul
 *        Code: https://github.com/kigster
 *
 *  (c) 2014 All rights reserved, MIT License.
 */

#include "MusicPlayer.h"

AudioPlaySdWav playWave;
AudioOutputI2S i2s1;
AudioConnection patchCordRight(playWave, 0, i2s1, 0);
AudioConnection patchCordLeft(playWave, 1, i2s1, 1);
AudioControlSGTL5000 sgtl5000;

MusicPlayer::MusicPlayer() {
    _maxVolume = MUSIC_PLAYER_DEFAULT_MAX_VOLUME;
    _volume = 0.5 * _maxVolume;
    init();
}

MusicPlayer::MusicPlayer(float maxVolume) {
    _maxVolume = maxVolume;
    if (_maxVolume > 1.0 || _maxVolume < 0)
        _maxVolume = MUSIC_PLAYER_DEFAULT_MAX_VOLUME;
    _volume = 0.5 * _maxVolume;
    init();
}

void MusicPlayer::init() {
}

void MusicPlayer::begin() {
    AudioMemory(MUSIC_PLAYER_DEFAULT_MEMORY);

    sgtl5000.enable();
    sgtl5000.volume(_volume);
}

bool MusicPlayer::play(const char *filename) {
    stop();

    if (!SD.exists((char *) filename)) {
        return false;
    }

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
    }
}

bool MusicPlayer::setVolume(float volume) {
    if (volume > 1.0 || volume < 0)
        return false;

    volume = volume * _maxVolume;
    if (abs(volume - _volume) > 0.02) {
        _volume = volume;
        sgtl5000.volume(_volume);
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
