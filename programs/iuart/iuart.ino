#include "src/microcode/util/log.h"
#include "src/microcode/hardware/suart.h"
#include "src/microcode/util/rate.h"
#include "src/microcode/util/filesys.h"
#include "src/microcode/grid/hub.h"

Rate r(4);
Rate r2(0.1);

GridMessageHub hub0(1);
GridMessageHub hub1(2);
GridMessageHub hub2(1);

SharedGridBuffer sharedState0("RobotState");
SharedGridBuffer sharedState2("RobotState");

#define linkspeed 10000

void setup()
{
    Log("system") << "booting ...";
    suart0.init(115200);
    Log() >> *suart0.getTXStream();

    filesys.init();
    Log("filesys") << filesys.toString(); 

    hub0.listenFor(sharedState0);
    hub2.listenFor(sharedState2);

    hub0.setLinkSpeed(0, linkspeed);
    hub1.setLinkSpeed(0, linkspeed);
    hub1.setLinkSpeed(1, linkspeed);
    hub2.setLinkSpeed(0, linkspeed);
}

void sendPackets(GridMessageHub& h0, GridMessageHub& h1, unsigned b0, unsigned b1)
{
    while (!h0[b0].out.empty())
    {
        GridPacket& pkt = h0[b0].out.top();
        for (unsigned i = 0; i < pkt.size(); i++)
        {
            h1[b1].in.insert(pkt.raw()[i]);
        }
        h0[b0].out.pop();
    }

    while (!h1[b1].out.empty())
    {
        GridPacket& pkt = h1[b1].out.top();
        for (unsigned i = 0; i < pkt.size(); i++)
        {
            h0[b0].in.insert(pkt.raw()[i]);
        }
        h1[b1].out.pop();
    }
}

void wastePackets(GridMessageHub& h, unsigned b)
{
    while (!h[b].out.empty())
        h[b].out.pop();
}

const char* msg0 = "Post from hub0";
const char* msg2 = "Post from hub2";

void loop()
{
    if (r.isReady())
    {
        Log("MemoryUsage") << sysMemUsage();

        sharedState0.lock();
        for (auto it = sharedState0.data.begin(); it != sharedState0.data.end(); ++it)
            Log("sharedstate0:") << "id: " << it.key() << " arrive: " << it->arrival << " msg: " << (char*) it->buf.data(); 
        sharedState0.unlock();

        sharedState0.lock();
        Log("sharedState0") << "size: " << sharedState0.data.size();
        sharedState0.unlock();
        sharedState2.lock();
        Log("sharedState2") << "size: " << sharedState2.data.size();
        sharedState2.unlock();

        {
            SharedBuffer& buf = sharedState0.touch(hub0.id());
            buf = SharedBuffer((const uint8_t*) msg0, strlen(msg0) + 1);
            buf.data()[buf.size() - 1] = 0;
        }
        
        {
            SharedBuffer& buf = sharedState2.touch(hub2.id());
            buf = SharedBuffer((const uint8_t*) msg2, strlen(msg2) + 1);
            buf.data()[buf.size() - 1] = 0;
        }
    }

    suart0.read();
    ByteStream* rx = suart0.getRXStream();
    rx->lock();
    while (!rx->isEmptyUnlocked())
    {
        if (rx->getUnlocked() == (uint8_t) 't')
            esp_restart();
    }
    rx->unlock();

    sendPackets(hub0, hub1, 0, 0);
    sendPackets(hub1, hub2, 1, 0);

    uint64_t updateStart = sysTime();
    hub0.update();
    uint64_t updateDT = sysTime() - updateStart;
    hub1.update();
    hub2.update();

    if (r2.isReady())
    {
        Log("hub0") << hub0._table.toString();
        Log("hub0") << hub0._graph.toString();
        Log("hub0") << "bytes: " << hub0.totalBytes();
        Log("hub0") << "dt: " << updateDT;

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
