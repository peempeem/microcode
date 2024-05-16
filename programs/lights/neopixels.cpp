#include "neopixels.h"

NeoPixels::NeoPixels(TIM_HandleTypeDef* timer, unsigned channel, unsigned ARR, unsigned numPixels) : _timer(timer), _channel(channel), _numPixels(numPixels)
{
    _zero = (ARR + 1) / 3;
    _one = _zero * 2;
    _data = new bgr_t[numPixels];
    _dmaBufSize = numPixels * 24 + 1;
    _dmaBuf = new uint32_t[_dmaBufSize];

    for (unsigned i = 0; i < numPixels; i++)
        _data[i].data = 0;
}

NeoPixels::~NeoPixels()
{
    delete[] _data;
    delete[] _dmaBuf;
}

void NeoPixels::set(unsigned i, uint8_t r, uint8_t g, uint8_t b)
{
    _data[i].color.r = r;
    _data[i].color.g = g;
    _data[i].color.b = b;
}

void NeoPixels::send()
{
    unsigned j = 0;
    for (unsigned i = 0; i < _numPixels; i++)
    {
        for (int bit = 23; bit >= 0; bit--, j++)
            _dmaBuf[j] = ((_data[i].data >> bit) & 0x01) ? _one : _zero;
    }
    _dmaBuf[j] = 0;

    HAL_TIM_PWM_Start_DMA(_timer, _channel, _dmaBuf, _dmaBufSize);
}
