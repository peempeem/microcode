#include "src/microcode/util/log.h"
#include "src/microcode/util/rate.h"
#include "src/microcode/hardware/suart.h"
#include "src/microcode/usb/usbcom.h"
#include "src/microcode/grid/hub.h"

#define SIMULATE false

#define BAUD (unsigned) 2e6
#define RX1 35
#define TX1 32
const char* moduleBaseName = "module";
const unsigned moduleBaseLen = strlen(moduleBaseName);

struct Eular 
{
  float yaw;
  float pitch;
  float roll;
};

PACK(struct Extras
{
    uint8_t stage;
    uint16_t id;
    uint32_t ping;
    float memUsage;
    float memUsageExternal;
});

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

PACK(struct SetDrivers
{
    uint8_t stage;
    StageState::Drivers drivers;
});

PACK(struct SetBuf
{
    uint8_t stage;
    uint8_t data[];
});

USBMessageBroker usb;
GridMessageHub hub(1);
SharedGridBuffer robotState("RobotState", (int) GridMessageHub::Priority::USER);
SharedGridBuffer robotControl("RobotControl", (int) GridMessageHub::Priority::USER);
SharedGridBuffer robotLighting("RobotLighting", (int) GridMessageHub::Priority::USER);

Hash<unsigned> toID;
Hash<unsigned> fromID;
Rate graphSend(1);

#if SIMULATE
GridMessageHub tempHub(1);
SharedGridBuffer tempRobotState("RobotState", (int) GridMessageHub::Priority::USER);
SharedGridBuffer tempRobotControl("RobotControl", (int) GridMessageHub::Priority::USER);
ByteStream tempRXTX;
ByteStream tempTXRX;
Rate tempPost(10);
StageState tempSS;
#endif

void setup()
{
    suart0.init(BAUD, 3, 1, false);
    Log() >> *suart0.getTXStream();
    usb.attachStreams(suart0.getTXStream(), suart0.getRXStream());
    
    #if SIMULATE
    hub.setInputStream(0, tempRXTX);
    hub.setOutputStream(0, tempTXRX);
    tempHub.setLinkSpeed(0, BAUD);
    tempHub.setName("module0");
    tempHub.listenFor(tempRobotState);
    tempHub.listenFor(tempRobotControl);
    tempHub.setInputStream(0, tempTXRX);
    tempHub.setOutputStream(0, tempRXTX);
    #else
    suart1.init(BAUD, RX1, TX1, false);
    hub.setInputStream(0, *suart1.getRXStream());
    hub.setOutputStream(0, *suart1.getTXStream());
    #endif

    hub.setLinkSpeed(0, BAUD);
    hub.setName("base");
    hub.listenFor(robotState);
    hub.listenFor(robotControl);
    hub.listenFor(robotLighting);
}

Rate updateRate(50);

void loop()
{
    suart0.read();
    usb.update();
    suart0.write();

    #if !SIMULATE
    suart1.read();
    #endif
    hub.updateTop(0);
    hub.updateBottom();
    #if !SIMULATE
    suart1.write();
    #endif

    #if SIMULATE
    tempHub.updateTop(0);
    tempHub.updateBottom();
    #endif

    auto nodes = hub.getTableNodes();
    for (auto it = nodes.begin(); it != nodes.end(); ++it)
    {
        if (it->name.size() <= moduleBaseLen 
            || memcmp(&it->name[0], moduleBaseName, moduleBaseLen))
            continue;
        unsigned stage = std::stoi(it->name.substr(moduleBaseLen));
        toID[stage] = it.key();
        fromID[it.key()] = stage;
    }

    #if SIMULATE
    if (tempPost.isReady())
    {
        tempRobotState.lock();
        SharedBuffer& buf = tempRobotState.touch(tempHub.id());
        buf.resize(sizeof(StageState));
        tempSS.timestamp = sysTime();
        *((StageState*) buf.data()) = tempSS;
        tempRobotState.unlock();
    }

    tempRobotControl.lock();
    if (tempRobotControl.canRead(tempHub.id()))
        tempSS.drivers = *((StageState::Drivers*) tempRobotControl.data[tempHub.id()].buf.data());
    tempRobotControl.unlock();
    #endif

    for (auto it = fromID.begin(); it != fromID.end(); ++it)
    {
        if (!nodes.contains(it.key()))
        {
            unsigned stage = *it;
            fromID.remove(it);
            toID.remove(stage);
        }
    }
    fromID.shrink();

    while (!usb.messages.empty())
    {
        auto& msg = usb.messages.top();
        switch (msg.get().header.type)
        {
            case 0:
            {
                SetDrivers* sd = (SetDrivers*) msg.get().data;
                unsigned* id = toID.at(sd->stage);
                if (id)
                {
                    robotControl.lock();
                    SharedBuffer& buf = robotControl.touch(*id);
                    buf.resize(sizeof(StageState::Drivers));
                    *((StageState::Drivers*) buf.data()) = sd->drivers;
                    robotControl.unlock();
                    break;
                }
            }

            case 5:
            {
                SetBuf* sb = (SetBuf*) msg.get().data;
                unsigned* id = toID.at(sb->stage);
                if (id)
                {
                    robotLighting.lock();
                    SharedBuffer& buf = robotLighting.touch(*id);
                    buf.resize(msg.get().header.len - sizeof(SetBuf));
                    memcpy(buf.data(), sb->data, msg.get().header.len - sizeof(SetBuf));
                    robotLighting.unlock();
                }
                break;
            }
        }
        usb.messages.pop();
    }

    if (updateRate.isReady())
    {
        robotControl.lock();
        for (auto it = robotControl.data.begin(); it != robotControl.data.end(); ++it)
            robotControl.touch(it.key());
        robotControl.unlock();

        robotLighting.lock();
        for (auto it = robotLighting.data.begin(); it != robotLighting.data.end(); ++it)
            robotLighting.touch(it.key());
        robotLighting.unlock();
    }
    
    robotState.lock();
    for (auto it = robotState.data.begin(); it != robotState.data.end(); ++it)
    {
        if (robotState.canRead(it.key()))
        {
            unsigned* stage = fromID.at(it.key());
            NetworkTable::Node* node = nodes.at(it.key());
            if (!stage || !node)
                continue;
            
            uint8_t* buf = new uint8_t[sizeof(Extras) + it->buf.size()];
            ((Extras*) buf)->stage = *stage;
            ((Extras*) buf)->id = it.key();
            ((Extras*) buf)->ping = node->ping;
            ((Extras*) buf)->memUsage = node->data.memUsage;
            ((Extras*) buf)->memUsageExternal = node->data.memUsageExternal;
            memcpy(&buf[sizeof(Extras)], it->buf.data(), it->buf.size());
            usb.send(1, 0, buf, sizeof(Extras) + it->buf.size());
            delete[] buf;
        }
    }
    robotState.unlock();


    // if (graphSend.isReady())
    // {
    //     GridGraph g;
    //     hub.getGraph(g);
    //     std::string s = g.toString();
    //     usb.send(2, 1, (uint8_t*) &s[0], s.size());
    //     NetworkTable::Nodes nodes = hub.getTableNodes();
    //     s = nodes.toString();
    //     usb.send(2, 1, (uint8_t*) &s[0], s.size());
    // }
}
