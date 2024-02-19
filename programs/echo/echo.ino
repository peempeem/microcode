#include "src/microcode/util/rate.h"

Rate r(1);

void setup()
{
    Serial.begin(115200);
    Serial1.begin(500000, SERIAL_8N1, 34, 33);
    Serial2.begin(500000, SERIAL_8N1, 35, 32);
    Serial.println("hello");
}

void loop()
{
    if (r.isReady())
    {
        Serial1.write('b');
        Serial2.write('b');
    }
        
    // if (Serial1.available())
    // {
    //     char c = Serial1.read();
    //     if (c == 'b')
    //     {
    //         Serial.println("serial1 got b");
    //         Serial1.write('a');
    //     }
    //     else if (c == 'a')
    //     {
    //         Serial.println("serial1 got a");
    //     }
    // }

    if (Serial2.available())
    {
        char c = Serial2.read();
        if (c == 'b')
        {
            Serial.println("serial2 got b");
            Serial2.write('a');
        }
        else if (c == 'a')
        {
            Serial.println("serial2 got a");
        }
    }
}
