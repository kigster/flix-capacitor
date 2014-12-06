/*
 * Joystick.cpp
 *
 *  Created on: Nov 23, 2014
 *      Author: Konstantin Gredeskoul
 *        Code: https://github.com/kigster
 *
 *  (c) 2014 All rights reserved, MIT License.
 */

#include "Joystick.h"

Joystick::Joystick(uint8_t _pinX, uint8_t _pinY, uint8_t _pinButton) {
    pinX = _pinX;
    pinY = _pinY;
    pinButton = _pinButton;
}

void Joystick::begin() {
    pinMode(pinX, INPUT);
    pinMode(pinY, INPUT);
    pinMode(pinButton, INPUT);

    digitalWrite(pinButton, HIGH);
}


bool Joystick::xForward() {
    return (1 - readX() < JOYSTICK_THRESHOLD) ? true : false;
}
bool Joystick::xBack() {
    return (readX() - JOYSTICK_X_BOUND < JOYSTICK_THRESHOLD) ? true : false;
}
bool Joystick::yUp() {
    return (1 - readY() < JOYSTICK_THRESHOLD) ? true : false;

}
bool Joystick::yDown() {
    return (readY() - JOYSTICK_Y_BOUND < JOYSTICK_THRESHOLD) ? true : false;

}
float Joystick::readX() {
    return 1.0 * analogRead(pinX) / 1024.0;
}
float Joystick::readY() {
    return 1.0 * analogRead(pinY) / 1024.0;
}
bool Joystick::buttonPressed(){
    return digitalRead(pinButton) == LOW;
}
