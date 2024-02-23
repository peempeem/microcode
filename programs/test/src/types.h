#pragma once

#include "microcode/hal/hal.h"
#include "microcode/hardware/suart.h"

struct Eular 
{
  float yaw;
  float pitch;
  float roll;
};

PACK(struct StageState
{
    uint64_t timestamp;
    struct Initialization
    {
        uint8_t debug   : 1;
        uint8_t filesys : 1;
        uint8_t wifi    : 1;
        uint8_t mcp     : 1;
        uint8_t adc     : 1;
        uint8_t imu     : 1;
        uint8_t comm0   : 1;
        uint8_t comm1   : 1;
    } init;

    struct Drivers
    {
        uint8_t s0 : 1;
        uint8_t s1 : 1;
        uint8_t s2 : 1;
        uint8_t s3 : 1;
        uint8_t s4 : 1;
        uint8_t s5 : 1;
        uint8_t m0 : 1;
        uint8_t m1 : 1;
    } drivers;

    struct Sensors
    {
        float pressure[4];
        Eular ypr;
    } sensors;
});

PACK(struct SetDevice
{
    uint8_t driver;
    uint8_t on;
});

struct WiFiData
{
    char ssid[64];
    char host[64];
    char pass[64];
};
