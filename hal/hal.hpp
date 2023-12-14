#include "hal.h"

#if __has_include(<task.h>)
#include <task.h>
#endif

static uint64_t sysMillis()
{
    return sysMicros() / 1000ULL;
}

static void sysSleep(unsigned ms)
{
    vTaskDelay(ms / portTICK_PERIOD_MS);
}

#if !__has_include(<Arduino.h>)
#include <math.h>

static float fsin(float value)
{
    return sinf(value);
}

static float fcos(float value)
{
    return cosf(value);
}

#endif

#if __has_include(<Arduino.h>)

#if !__has_include(<esp.h>)
uint64_t sysMicros()
{
	return millis() * 1000 + micros() % 1000;
}
#endif

static void sysBeginDebug(unsigned baudrate)
{
    Serial.begin(baudrate);
}

template <class T> static void sysPrint(T data, bool nl)
{
    if (!Serial)
        return;

    if (nl)
        Serial.println(data);
    else
        Serial.print(data);
}

template <class T> static void sysPrintHex(T data, bool nl)
{
    if (!Serial)
        return;

    if (nl)
        Serial.println(data, HEX);
    else
        Serial.print(data, HEX);
}

static void sysWrite(uint8_t* data, unsigned len)
{
    if (Serial)
        Serial.write(data, len);
}

static void sysPinMode(unsigned pin, unsigned mode)
{
    pinMode(pin, mode);
}

static unsigned sysDigitalRead(unsigned pin)
{
    return digitalRead(pin);
}

static void sysDigitalWrite(unsigned pin, unsigned state)
{
    digitalWrite(pin, state);
}

#if !__has_include(<esp.h>)
static unsigned sysAnalogRead(unsigned pin)
{
    return analogRead(pin);
}
#endif

static void sysSerialInit(unsigned port, unsigned baudrate, int rx, int tx)
{
    switch (port)
    {
        case 0:
            Serial.begin(baudrate, SERIAL_8N1, rx, tx);
            break;
        
        case 1:
            Serial1.begin(baudrate, SERIAL_8N1, rx, tx);
            break;
        
        case 2:
            Serial2.begin(baudrate, SERIAL_8N1, rx, tx);
            break;
    }
}

static bool sysSerialAvailable(unsigned port)
{
    HardwareSerial* serial;
    switch (port)
    {
        case 0:
            serial = &Serial;
            break;
        
        case 1:
            serial = &Serial1;
            break;
        
        case 2:
            serial = &Serial2;
            break;
        
        default:
            return false;
    }
    return serial->available();
}

static uint8_t sysSerialRead(unsigned port)
{
    HardwareSerial* serial;
    switch (port)
    {
        case 0:
            serial = &Serial;
            break;
        
        case 1:
            serial = &Serial1;
            break;
        
        case 2:
            serial = &Serial2;
            break;
        
        default:
            return 0;
    }
    return serial->read();
}

static void sysSerialWrite(unsigned port, uint8_t* data, unsigned len)
{
    HardwareSerial* serial;
    switch (port)
    {
        case 0:
            serial = &Serial;
            break;
        
        case 1:
            serial = &Serial1;
            break;
        
        case 2:
            serial = &Serial2;
            break;
        
        default:
            return;
    }
    serial->write(data, len);
}

static float fsin(float value)
{
    return sinf(value);
}

static float fcos(float value)
{
    return cosf(value);
}

#endif

#if __has_include(<esp.h>)
#include "calibration.h"

extern "C"
{
    uint32_t esp_get_free_heap_size();
    uint8_t temperature_sens_read();
}

uint32_t esp_get_free_heap_size();
uint8_t temprature_sens_read();

uint64_t sysMicros()
{
	return esp_timer_get_time();
}

static void sysInit()
{
	sysMemUsage();
}

static float sysTemp()
{
	return (temprature_sens_read() - 32) / 1.8f;
}

static float sysMemUsage()
{
	static unsigned maxMemory = 0;

	unsigned free = esp_get_free_heap_size();
	if (free > maxMemory)
		maxMemory = free;
	return (maxMemory - free) / (float) maxMemory;
}

static unsigned sysAnalogRead(unsigned pin)
{
	return analogReadCalibrationValues[analogRead(pin)];
}

static void sysConfigurePWM(unsigned channel, unsigned frequency, unsigned resolution)
{
    ledcSetup(channel, frequency, resolution);
}

static void sysAttachPin(unsigned pin, unsigned channel)
{
    ledcAttachPin(pin, channel);
}

static void sysDetachPin(unsigned pin)
{
    ledcDetachPin(pin);
}

static void sysPWMWrite(unsigned channel, unsigned duty)
{
    ledcWrite(channel, duty);
}

#endif

