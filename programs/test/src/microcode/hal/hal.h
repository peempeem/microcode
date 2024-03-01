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

static uint64_t sysTime()
{
    return esp_timer_get_time();
}

static void sysSleep(unsigned ms)
{
    vTaskDelay(ms / portTICK_PERIOD_MS);
}

static float sysMemUsage()
{
    return 1 - (ESP.getFreeHeap() / (float) ESP.getHeapSize());
}

static float sysMemUsageExternal()
{
    multi_heap_info_t info;
    heap_caps_get_info(&info, MALLOC_CAP_SPIRAM);
    return 1 - info.total_free_bytes / (float) (info.total_free_bytes + info.total_allocated_bytes);
}

static unsigned sysAnalogRead(unsigned pin)
{
    return analogReadCalibrationValues[analogRead(pin)];
}
