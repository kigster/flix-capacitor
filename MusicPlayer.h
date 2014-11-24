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
    bool play(const char *filename);
    void stop();
    bool isPlaying();
    /* always between 0 and 1.0, it's automatically adjusted for maxVolume */
    bool setVolume(float volume);
    float volume();
    /* also between 0 and 1.0, it defines what 1.0 means for volume */
    void setMaxVolume(float maxVolume);
private:
    void init();
    float _volume = 0.25;
    float _maxVolume = 0.6;

};

#endif /* MUSICPLAYER_H_ */
