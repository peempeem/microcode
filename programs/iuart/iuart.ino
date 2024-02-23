#include "src/microcode/util/log.h"
#include "src/microcode/hardware/suart.h"
#include "src/microcode/util/rate.h"
#include "src/microcode/util/filesys.h"
#include "src/microcode/grid/hub.h"

Rate r(50);
Rate r2(0.1);

GridMessageHub hub0(1);
GridMessageHub hub1(2);
GridMessageHub hub2(2);
GridMessageHub hub3(2);
GridMessageHub hub4(1);

SharedGridBuffer sharedState0("RobotState", 1);
SharedGridBuffer sharedState2("RobotState", 1);
SharedGridBuffer sharedState4("RobotState", 1);

#define linkspeed 10000

void sendPackets(GridMessageHub& h0, GridMessageHub& h1, unsigned b0, unsigned b1)
{
    h0[b0].out.lock();
    while (!h0[b0].out.empty())
    {
        GridPacket& pkt = h0[b0].out.top();
        for (unsigned i = 0; i < pkt.size(); i++)
            h1[b1].in.insert(pkt.raw()[i]);
        h0[b0].out.pop();
    }
    h0[b0].out.unlock();

    h1[b1].out.lock();
    while (!h1[b1].out.empty())
    {  
        GridPacket& pkt = h1[b1].out.top();
        for (unsigned i = 0; i < pkt.size(); i++)
            h0[b0].in.insert(pkt.raw()[i]);
        h1[b1].out.pop();
    }
    h1[b1].out.unlock();
}

void wastePackets(GridMessageHub& h, unsigned b)
{
    while (!h[b].out.empty())
        h[b].out.pop();
}

void updateHub(void* args)
{
    uint32_t notification = 0;
    Rate r(4);

    while (true)
    {
        xTaskNotifyWait(0, UINT32_MAX, &notification, 10 / portTICK_PERIOD_MS);
        hub0.update();
        hub1.update();
        hub2.update();
        hub3.update();
        hub4.update();

        if (r.isReady())
            Log("updateHub") << uxTaskGetStackHighWaterMark(NULL);

        if (notification & BIT(0))
            vTaskDelete(NULL);
    }
}

void transportPackets(void* args)
{
    uint32_t notification = 0;
    Rate r(4);

    while (true)
    {
        xTaskNotifyWait(0, UINT32_MAX, &notification, 10 / portTICK_PERIOD_MS);
        sendPackets(hub0, hub1, 0, 0);
        sendPackets(hub1, hub2, 1, 0);
        sendPackets(hub2, hub3, 1, 0);
        sendPackets(hub3, hub4, 1, 0);

        if (r.isReady())
            Log("sendPackets") << uxTaskGetStackHighWaterMark(NULL);

        if (notification & BIT(0))
            vTaskDelete(NULL);
    }
}

const char* msg0 = "Post from hub0";
const char* msg2 = "Post from hub2 ligma ballszzz :D Post from hub2 ligma ballszzz :D Post from hub2 ligma ballszzz :D Post from hub2 ligma ballszzz :D Post from hub2 ligma ballszzz :D Post from hub2 ligma ballszzz :D Post from hub2 ligma ballszzz :D Post from hub2 ligma ballszzz :D Post from hub2 ligma ballszzz :D Post from hub2 ligma ballszzz :D Post from hub2 ligma ballszzz :D Post from hub2 ligma ballszzz :D Post from hub2 ligma ballszzz :D Post from hub2 ligma ballszzz :D";
const char* msg4 = "Post from hub4";

void setup()
{
    Log("system") << "booting ...";
    suart0.init(2000000);
    Log() >> *suart0.getTXStream();

    filesys.init();
    Log("filesys") << filesys.toString(); 

    hub0.listenFor(sharedState0);
    hub2.listenFor(sharedState2);
    hub4.listenFor(sharedState4);

    hub0.setLinkSpeed(0, linkspeed);
    hub1.setLinkSpeed(0, linkspeed);
    hub1.setLinkSpeed(1, linkspeed);
    hub2.setLinkSpeed(0, linkspeed);
    hub2.setLinkSpeed(1, linkspeed);
    hub3.setLinkSpeed(0, linkspeed);
    hub3.setLinkSpeed(1, linkspeed);
    hub4.setLinkSpeed(0, linkspeed);

    xTaskCreateUniversal(transportPackets, "transport", 4 * 1024, NULL, 10, NULL, tskNO_AFFINITY);
    xTaskCreateUniversal(updateHub, "hubupdate", 4 * 1024, NULL, 5, NULL, tskNO_AFFINITY);
}

void loop()
{
    if (r.isReady())
    {
        Log("MemoryUsage") << sysMemUsage();

        {
            sharedState0.lock();
            SharedBuffer& buf = sharedState0.touch(hub0.id());
            buf = SharedBuffer((const uint8_t*) msg0, strlen(msg0) + 1);
            buf.data()[buf.size() - 1] = 0;
            sharedState0.unlock();
        }

        {
            sharedState2.lock();
            SharedBuffer& buf = sharedState2.touch(hub2.id());
            buf = SharedBuffer((const uint8_t*) msg2, strlen(msg2) + 1);
            buf.data()[buf.size() - 1] = 0;
            sharedState2.unlock();
        }
        
        {
            sharedState4.lock();
            SharedBuffer& buf = sharedState4.touch(hub4.id());
            buf = SharedBuffer((const uint8_t*) msg4, strlen(msg4) + 1);
            buf.data()[buf.size() - 1] = 0;
            sharedState4.unlock();
        }
    }

    sharedState0.lock();
    for (auto it = sharedState0.data.begin(); it != sharedState0.data.end(); ++it)
    {
        if (sharedState0.canRead(it.key()))
        {
            Log("sharedstate0") << "id: " << hub0.id() << " _id: " << it.key() << " arrive: " << it->arrival << " msg: " << (char*) it->buf.data(); 
        }
            
    }
    sharedState0.unlock();

    sharedState2.lock();
    for (auto it = sharedState2.data.begin(); it != sharedState2.data.end(); ++it)
    {
        if (sharedState2.canRead(it.key()))
        {
            Log("sharedstate2") << "id: " << hub2.id() << " _id: " << it.key() << " arrive: " << it->arrival << " msg: " << (char*) it->buf.data(); 
        }
           
    }
    sharedState2.unlock();

    sharedState4.lock();
    for (auto it = sharedState4.data.begin(); it != sharedState4.data.end(); ++it)
    {
        if (sharedState4.canRead(it.key()))
        {
            Log("sharedstate4") << "id: " << hub4.id() << " _id: " << it.key() << " arrive: " << it->arrival << " msg: " << (char*) it->buf.data();
        }
    }
    sharedState4.unlock();

    ByteStream* rx = suart0.getRXStream();
    uint8_t buf[128];
    unsigned pulled;
    do
    {
        pulled = rx->get(buf, sizeof(buf));
        for (unsigned i = 0; i < pulled; ++i)
        {
            if (buf[i] == (uint8_t) 't')
                ESP.restart();
        }
    } while (pulled);

    if (r2.isReady())
    {
        hub0._lock.lock();
        Log("hub0") << hub0._table.toString();
        Log("hub0") << hub0._graph.toString();
        Log("hub0") << "bytes: " << hub0.totalBytes();
        hub0._lock.unlock();

        // std::vector<std::vector<uint16_t>> paths(hub0._table.nodes.size() * hub0._table.nodes.size());
        // uint64_t start1, dt1, start2, dt2;
        // unsigned i;

        // start1 = sysTime();
        // i = 0;
        // for (auto it = hub0._table.nodes.begin(); it != hub0._table.nodes.end(); ++it)
        // {
        //     for (auto ti = hub0._table.nodes.begin(); ti != hub0._table.nodes.end(); ++ti)
        //         hub0._graph.path(paths[i++], it.key(), ti.key());
        // }
        // dt1 = sysTime() - start1;

        // start2 = sysTime();
        // i = 0;
        // for (auto it = hub0._table.nodes.begin(); it != hub0._table.nodes.end(); ++it)
        // {
        //     for (auto ti = hub0._table.nodes.begin(); ti != hub0._table.nodes.end(); ++ti)
        //         hub0._graph.path(paths[i++], it.key(), ti.key());
        // }
        // dt2 = sysTime() - start1;

        // Log("djikstra") << dt1;
        // Log("BFS") << dt2;
    }
}
