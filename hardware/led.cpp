#include "led.h"
#include "../hal/hal.h"

LED::LED(unsigned pin, bool inverted) : pin(pin), inverted(inverted)
{
    
}

void LED::init()
{
	#ifdef sysPinMode
    sysPinMode(pin, PinMode::output);
	#endif
}

#ifdef PWM
void LED::usePWM(PWM& pwm)
{

    if (usingPWM)
        sysDetachPin(pin);
    sysAttachPin(pin, pwm.channel());
    usingPWM = true;
}
#endif

void LED::turnOn()
{
	#ifdef PWM
    if (usingPWM)
    {
        sysDetachPin(pin);
        usingPWM = false;
        init();
    }
	#endif

	#ifdef sysDigitalWrite
    sysDigitalWrite(pin, inverted ? Digital::low : Digital::high);
	#endif
}

void LED::turnOff()
{
	#ifdef PWM
    if (usingPWM)
    {
        sysDetachPin(pin);
        usingPWM = false;
        init();
    }
	#endif

	#ifdef sysDigitalWrite
    sysDigitalWrite(pin, inverted ? Digital::high : Digital::low);
	#endif
}
