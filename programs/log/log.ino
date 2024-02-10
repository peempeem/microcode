#include "src/microcode/hal/hal.h"
#include "src/microcode/util/log.h"
#include "src/microcode/util/bytestream.h"
#include "src/microcode/util/rate.h"

ByteStream stream;

void setup()
{
    Log("before init") << "hello before init";
    Serial.begin(115200);
    debugHandler.attachByteStream(stream);
    Log("after") << "LETGO!?";
}

Rate r(1);

void loop()
{
    while (!stream.isEmpty())
        Serial.write(stream.get());
    
    if (r.isReady())
    {
        Log("bruh") << "WAT THE BURG";
        Serial.println(sysMemUsage());
    }
        
}
