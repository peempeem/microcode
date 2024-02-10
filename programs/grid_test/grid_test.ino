#include "src/microcode/hal/hal.h"
#include "src/microcode/grid/hub.h"
#include "src/microcode/util/rate.h"

void setup()
{
    Serial.begin(115200);
    sysAttachDebug(&Serial);
}

GridMessageHub hub(1, 11);

void loop()
{
    hub.update();
}
