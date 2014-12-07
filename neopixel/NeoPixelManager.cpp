/*
 * NeoPixelManager.cpp
 *
 *  Created on: Nov 28, 2014
 *      Author: Konstantin Gredeskoul
 *        Code: https://github.com/kigster
 *
 *  (c) 2014 All rights reserved, MIT License.
 */

#include "NeoPixelManager.h"
#include "NeoPixelEffects.h"
#include <Adafruit_NeoPixel.h>

NeoPixelManager::NeoPixelManager(uint8_t pixels, uint8_t pin) {
    _strip = new Adafruit_NeoPixel(pixels, pin, NEO_GRB + NEO_KHZ800);
    _effects = new NeoPixelEffects(_strip);
}

uint32_t NeoPixelManager::color(uint8_t r, uint8_t g, uint8_t b) {
    return _strip->Color(r,g,b);
}

void NeoPixelManager::shutoff() {
    _strip->clear();
    _strip->show();
}

void NeoPixelManager::begin() {
    _strip->begin();
    _strip->show(); // Initialize all pixels to 'off'
}

void NeoPixelManager::nextEffect() {
    if (_effects != NULL)
        _effects->chooseNewEffect();
}

void NeoPixelManager::refreshEffect() {
    if (_effects != NULL)
        _effects->refreshCurrentEffect();
}

NeoPixelEffects *NeoPixelManager::effects() {
    return _effects;
}
