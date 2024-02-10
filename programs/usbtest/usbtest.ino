#include "src/microcode/hal/hal.h"
#include "src/microcode/usb/usbcom.h"
#include "src/microcode/util/rate.h"

USBMessageBroker usb(0);

PACK(struct StageState
{
    uint64_t timestamp;
    union Initialization
    {
        struct Fields
        {
            uint8_t filesys : 1;
            uint8_t wifi    : 1;
            uint8_t mcp     : 1;
            uint8_t adc     : 1;
            uint8_t imu     : 1;
            uint8_t comm0   : 1;
            uint8_t comm1   : 1;
            uint8_t debug   : 1;
        } single;
        uint8_t all;
    } init;

    union Drivers
    {
        struct Fields
        {
            uint8_t s0 : 1;
            uint8_t s1 : 1;
            uint8_t s2 : 1;
            uint8_t s3 : 1;
            uint8_t s4 : 1;
            uint8_t s5 : 1;
            uint8_t m0 : 1;
            uint8_t m1 : 1;
        } single;
        uint8_t all;
    } drivers;
    
    float pressure[4];
    float ypr[3];
    float power;
}) ss;

void setup()
{
    usb.begin(500000);
}

Rate sendStateRate(100);

Rate solenoids[6] = { Rate(0.2), Rate(0.4), Rate(0.6), Rate(0.8), Rate(1.2), Rate(1.4) };
Rate motors[2] = { Rate(0.5), Rate(1.5) };
Rate pressures[4] = { Rate(0.25), Rate(0.75), Rate(1.25), Rate(1.75) };
Rate ypr[3] = { Rate(0.5), Rate(1), Rate(1.5) };

PACK(struct SetDevice
{
    uint8_t driver;
    uint8_t on;
});

void loop()
{
    while (usb.messages.size())
    {
        USBMessageBroker::Message& msg = usb.messages.front();
        switch (msg.type())
        {
            case 1:
            {
                SetDevice* setdevcmd = (SetDevice*) msg.data();
                if (setdevcmd->on)
                    ss.drivers.all |= 1 << setdevcmd->driver;
                else
                    ss.drivers.all &= ~(1 << setdevcmd->driver);
                break;
            }
        }
        usb.messages.pop();
    }
    
    // if (solenoids[0].isReady())
    //     ss.s0 = ~ss.s0;
    
    // if (solenoids[1].isReady())
    //     ss.s1 = ~ss.s1;
    
    // if (solenoids[2].isReady())
    //     ss.s2 = ~ss.s2;
    
    // if (solenoids[3].isReady())
    //     ss.s3 = ~ss.s3;
    
    // if (solenoids[4].isReady())
    //     ss.s4 = ~ss.s4;
    
    // if (solenoids[5].isReady())
    //     ss.s5 = ~ss.s5;
    
    // if (solenoids[6].isReady())
    //     ss.s6 = ~ss.s6;
    
    // if (solenoids[7].isReady())
    //     ss.s7 = ~ss.s7;
    
    for (unsigned i = 0; i < 4; i++)
        ss.pressure[i] = (pressures[i].getStageCos()) * 10 + 100;
    
    for (unsigned i = 0; i < 3; i++)
        ss.ypr[i] = ypr[i].getStageCos() * 180 - 90;
    
    if (sendStateRate.isReady())
    {
        ss.timestamp = sysTime();

        ss.power = 1.5;
        for (unsigned i = 0; i < 6; i++)
        {
            if (0x01 & (ss.drivers.all >> i))
                ss.power += 12 * 0.2;
        }

        for (unsigned i = 0; i < 2; i++)
        {
            if (0x01 & (ss.drivers.all >> (i + 6)))
                ss.power += 12 * 0.3;
        }

        usb.send(0, 0, (uint8_t*) &ss, sizeof(ss));
    }
    
    usb.update();
}
