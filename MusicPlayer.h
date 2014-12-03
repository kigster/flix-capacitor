/*
 * MusicPlayer
 *
 * This class is based on the WavFilePlayer from the Audio Library
 * by Paul Stoffregen, paul@pjrc.com
 *
 * It requires the audio shield: http://www.pjrc.com/store/teensy3_audio.html
 *
 * It encapsulates simple functions needed to control playback of a file
 * and volume externally.
 *
 *  Created on: Nov 23, 2014
 *      Author: Konstantin Gredeskoul
 *        Code: https://github.com/kigster
 *
 *  (c) 2014 All rights reserved, MIT License.
 */
#include "FlixCapacitor.h"

#ifdef ENABLE_AUDIO_SD

#ifndef MUSICPLAYER_H_
#define MUSICPLAYER_H_
#include <Audio.h>

#define MUSIC_PLAYER_DEFAULT_MAX_VOLUME 0.6
#define MUSIC_PLAYER_DEFAULT_MEMORY 20

class MusicPlayer {
public:
    MusicPlayer();
    MusicPlayer(float maxVolume);

    void begin();

    void turnOn();
    void turnOff();

    bool play(const char *filename);
    void stop();
    bool isPlaying();
    /* always between 0 and 1.0, it's automatically adjusted for maxVolume */
    float volume();
    bool setVolume(float volume);
    void setMaxVolume(float maxVolume);
    void setBassVolume(float volume);
    void moarBass(bool enabled);
    void changeBassVolume(bool up);
private:
    float _volume = 0.25;
    float _bassVolume = 0;
    float _maxVolume = 0.6;
    bool _on;

};

#endif /* MUSICPLAYER_H_ */

#endif /* ENABLE_AUDIO)_SD */
