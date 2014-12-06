/*
 * Joystick.h
 *
 *  Created on: Nov 23, 2014
 *      Author: Konstantin Gredeskoul
 *        Code: https://github.com/kigster
 *
 *  (c) 2014 All rights reserved, MIT License.
 */

#ifndef JOYSTICK_H_
#define JOYSTICK_H_
#include <Arduino.h>

#define JOYSTICK_X_BOUND 0.5
#define JOYSTICK_Y_BOUND 0.5

#define JOYSTICK_THRESHOLD 0.15

class Joystick {
public:
    Joystick(uint8_t _pinX, uint8_t _pinY, uint8_t _pinButton);
    void begin();
    float readX();
    float readY();
    bool xForward();
    bool xBack();
    bool yUp();
    bool yDown();
    bool buttonPressed();

private:
    uint8_t pinX, pinY, pinButton;
};

#endif /* JOYSTICK_H_ */
