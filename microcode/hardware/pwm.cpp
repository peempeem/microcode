#include "pwm.h"
#include "../hal/hal.h"

PWM::PWM() : defaultConstructor(true)
{

}

PWM::PWM(unsigned channel, unsigned frequency, unsigned resolution) : _channel(channel), _frequency(frequency), _resolution(resolution), defaultConstructor(false)
{

}

void PWM::init()
{
    if (!defaultConstructor)
        ledcSetup(_channel, _frequency, _resolution);
}

void PWM::write(float duty)
{
    if (duty < 0)
        duty = 0;
    else if (duty > 1)
        duty = 1;
    
    ledcWrite(_channel, (1 << (_resolution - 1)) * duty);
}

unsigned PWM::channel()
{
    return _channel;
}

unsigned PWM::frequency()
{
    return _frequency;
}

unsigned PWM::resolution()
{
    return _resolution;
}
