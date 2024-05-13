#include "led.h"
#include "../hal/hal.h"

LED::LED(unsigned pin, bool inverted) : pin(pin), inverted(inverted)
{
    
}

void LED::init()
{
    pinMode(pin, OUTPUT);
}

void LED::usePWM(PWM& pwm)
{
    if (usingPWM)
        ledcDetachPin(pin);
    ledcAttachPin(pin, pwm.channel());
    usingPWM = true;
}

void LED::turnOn()
{
    if (usingPWM)
    {
        ledcDetachPin(pin);
        usingPWM = false;
        init();
    }

    digitalWrite(pin, inverted ? LOW : HIGH);
}

void LED::turnOff()
{
    if (usingPWM)
    {
        ledcDetachPin(pin);
        usingPWM = false;
        init();
    }

    digitalWrite(pin, inverted ? HIGH : LOW);
}
