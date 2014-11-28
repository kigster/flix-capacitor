/*
 * TimeManager.h
 *
 *  Created on: Nov 28, 2014
 *      Author: Konstantin Gredeskoul
 *        Code: https://github.com/kigster
 *
 *  (c) 2014 All rights reserved, MIT License.
 */

#ifndef TEENSYTIMEMANAGER_H_
#define TEENSYTIMEMANAGER_H_

#include <Time.h>
#include <Wire.h>
#include <DS1307RTC.h>

class TeensyTimeManager {
public:
    TeensyTimeManager();
    bool setTimeToCompileTime();

private:
    tmElements_t tm;
    char *monthNames[12];
    bool getTime(const char *str);
    bool getDate(const char *str);
};

#endif /* TIMEMANAGER_H_ */
