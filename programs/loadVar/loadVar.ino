#include "src/microcode/util/pvar.h"
#include "src/microcode/hardware/suart.h"
#include "src/microcode/util/log.h"

struct WiFiData
{
    char ssid[64];
    char host[64];
    char pass[64];
};

PVar<WiFiData> wdata("/wifi");

char* ssid = "softwater";
char* pass = "softwater";
char* host = "module2";

void setup()
{
    Log("init") << "Begin initialization...";
    suart0.init(115200);
    Log() >> *suart0.getTXStream();
    filesys.init();
    strcpy(wdata.data.ssid, ssid);
    strcpy(wdata.data.pass, pass);
    strcpy(wdata.data.host, host);
    wdata.save();
    filesys.remove("/hostname");
    Log("init") << "done";
}

void loop()
{

}
