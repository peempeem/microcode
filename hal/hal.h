#pragma once

#ifdef __GNUC__
#define PACK( __Declaration__ ) __Declaration__ __attribute__((__packed__))
#endif

#ifdef _MSC_VER
#define PACK( __Declaration__ ) __pragma( pack(push, 1) ) __Declaration__ __pragma( pack(pop))
#endif

#include <stdint.h>
#include <FreeRTOS.h>

static uint64_t sysMillis();
extern uint64_t sysMicros() __attribute__((__weak__));
static void sysSleep(unsigned ms);

const static float fPI = 3.14159265358979323846264338327950288419716939937510582097494459f;
static float fsin(float value);
static float fcos(float value);

#if __has_include(<Arduino.h>)

#include <Arduino.h>

static void sysBeginDebug(unsigned baudrate);

template <class T> static void sysPrint(T data, bool nl=false);
template <class T> static void sysPrintHex(T data, bool nl);
static void sysWrite(uint8_t* data, unsigned len);

static void sysPinMode(unsigned pin, unsigned mode);
static unsigned sysDigitalRead(unsigned pin);
static void sysDigitalWrite(unsigned pin, unsigned state);
static unsigned sysAnalogRead(unsigned pin);

static void sysSerialInit(unsigned port, unsigned baudrate, int rx=-1, int tx=-1);
static bool sysSerialAvailable(unsigned port);
static uint8_t sysSerialRead(unsigned port);
static void sysSerialWrite(unsigned port, uint8_t* data, unsigned len);

#endif

#if __has_include(<esp.h>)

static void sysInit();
static float sysTemp();
static float sysMemUsage();

static unsigned sysAnalogRead(unsigned pin);

static void sysConfigurePWM(unsigned channel, unsigned frequency, unsigned resolution);
static void sysAttachPin(unsigned pin, unsigned channel);
static void sysDetachPin(unsigned pin);
static void sysPWMWrite(unsigned channel, unsigned duty);

#endif

#include "hal.hpp"
