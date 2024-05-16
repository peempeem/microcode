#include "lights.h"
#include "./src/microcode/hal/hal.h"
#include "./src/microcode/util/timer.h"
#include "neopixels.h"

extern volatile uint32_t UptimeMillis;
extern TIM_HandleTypeDef htim1;

uint32_t sysMillis()
{
	return UptimeMillis;
}

uint32_t sysMicros()
{
	uint32_t ms;
	uint32_t st;

	do
	{
		ms = UptimeMillis;
		st = SysTick->VAL;

	} while (ms != UptimeMillis);

	return ms * 1000 - st / ((SysTick->LOAD + 1) / 1000);
}

void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim)
{
  HAL_TIM_PWM_Stop_DMA(htim, TIM_CHANNEL_1);
}

void stm32Hook(void* args)
{
	Rate r(2.4);
	Rate g(1.6);
	Rate b(0.8);
    NeoPixels pixels(&htim1, 0, 110, 21);

    while (true)
    {
        for (unsigned i = 0; i < 21; i++)
        {
            float rf = r.getStageCos(i / 21.0f) * 255;
            pixels.set(
                i, 
                r.getStageCos(i / 21.0f) * 255, 
                g.getStageCos(i / 21.0f) * 255,
                b.getStageCos(i / 21.0f) * 255);
        }
        
        pixels.send();
        sysSleep(16);
    }
}
