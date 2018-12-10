#include "FlixCapacitor.h"

namespace SetTime {
typedef enum TimeChangeMode_e {
    Default     = (1 << 0),
    Hour        = (1 << 1),
    Minute      = (1 << 2),
    Brightness  = (1 << 3),
    Save        = (1 << 4),
    Last        = (1 << 5),
} TimeMode;
}
;

SetTime::TimeMode mode = SetTime::Default;
uint8_t h, m, brightness;

void nextMode() {
    if (mode == SetTime::Last)
        mode = SetTime::Default;
    else
        mode = (SetTime::TimeMode) ((int) mode << 1);
}

void configureTime() {
    displayModeChange("Change Time?", "UP = Yes, Click = NO");
    delay(500);
    while (true) {
        if (millis() - lastJoystickAction > 100) {
            lastJoystickAction = millis();
            if (joystick.yUp()) {
                delay(500);
                break;
            } else if (joystick.yDown() || joystick.buttonPressed()) {
                delay(500);
                return;
            }
        }
    }

    mode = SetTime::Hour;
    resetScreen();
    h = hour(), m = minute();
    while (mode != SetTime::Default) {
        switch (mode) {
        case SetTime::Hour:
            displayNotice("To set HOUR: Up/Down", "Click > Next");
            delay(500);
            selectNumber(&h, 1, 12, updateTimeCallback);
            break;

        case SetTime::Minute:
            displayNotice("To set MINUTE: Up/Down", "Click > Next");
            delay(500);
            selectNumber(&m, 0, 59, updateTimeCallback);
            break;

        case SetTime::Brightness:
            displayNotice("To set Brightness:", "UP/DOWN, Click > Next");
            delay(500);
            selectNumber(&brightness, 0, 15, updateBrigthnessCallback);
            break;

        case SetTime::Save:
            sprintf(buf, "Set time to %2d:%02d?", h, m);
            displayNotice(buf, "Yes = UP, No = Click...");
            delay(500);
            while(true) {
                if (millis() - lastJoystickAction > 100) {
                    lastJoystickAction = millis();
                    if (joystick.yUp()) {
                        tmElements_t tmTime;
                        breakTime(now(), tmTime);
                        tmTime.Hour = h;
                        tmTime.Minute = m;
                        Serial.println(buf);
                        time_t t = makeTime(tmTime);
                        setTime(t);
                        Teensy3Clock.set(t);
                        break;
                    } else if (joystick.yDown() || joystick.buttonPressed()){
                        delay(500);
                        break;
                    }
                }
            }
            /* no break */
        case SetTime::Last:
            mode = SetTime::Default;
            /* no break */
        case SetTime::Default:
            return;
            break;
        };
        nextMode();
    }
}

void updateTimeCallback() {
    displayTime(h, m);
}

void updateBrigthnessCallback() {
    matrix.setBrightness(brightness);
    Serial.print("Brightness value [0-15] set to ");
    Serial.println(brightness);
}

void selectNumber(uint8_t *current, int min, int max, displayUpdateCallback callback) {
    while(true) {
        if (millis() - lastJoystickAction > 100) {
            lastJoystickAction = millis();
            if (joystick.buttonPressed()) {
                delay(500);
                break;
            } else if (joystick.yUp()) {
                *current = *current + 1;
                if (*current > max) *current = max;
            } else if (joystick.yDown()) {
                *current = abs(*current - 1);
                if (*current < min) *current = min;
            }
            if (callback != NULL)
                callback();
        }
        delay(50);
    }
    return;
}
