#pragma once

#ifdef __GNUC__
#define PACK( __Declaration__ ) __Declaration__ __attribute__((__packed__))
#endif

#ifdef _MSC_VER
#define PACK( __Declaration__ ) __pragma( pack(push, 1) ) __Declaration__ __pragma( pack(pop))
#endif

#include "calibration.h"
#include <Arduino.h>
#include <esp.h>

extern "C"
{
    uint8_t temperature_sens_read();
}

uint8_t temprature_sens_read();

static uint64_t sysTime()
{
    return esp_timer_get_time();
}

static void sysSleep(unsigned ms)
{
    vTaskDelay(ms / portTICK_PERIOD_MS);
}

static float sysTemp()
{
    return (temprature_sens_read() - 32) / 1.8f;
}

static float sysMemUsage()
{
    return 1 - (ESP.getFreeHeap() / (float) ESP.getHeapSize());
}

static unsigned sysAnalogRead(unsigned pin)
{
    return analogReadCalibrationValues[analogRead(pin)];
}
