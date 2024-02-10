#pragma once

#include <stdint.h>
#include <stm32g4xx_hal.h>

class NeoPixels
{
	public:
		NeoPixels(TIM_HandleTypeDef* timer, unsigned channel, unsigned ARR, unsigned numPixels);
		~NeoPixels();

		void set(unsigned i, uint8_t r, uint8_t g, uint8_t b);

		void send();

	private:
		typedef union PixelBGR
		{
			struct
			{
				uint8_t b;
				uint8_t r;
				uint8_t g;
			} color;
			uint32_t data;
		} bgr_t;

		TIM_HandleTypeDef* _timer;
		unsigned _channel;
		unsigned _numPixels;
		bgr_t* _data;
		uint32_t _one;
		uint32_t _zero;
		unsigned _dmaBufSize;
		uint32_t* _dmaBuf;
};
