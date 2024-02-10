#include "src/microcode/util/log.h"
#include "src/microcode/hardware/suart.h"
#include "src/microcode/util/rate.h"
#include "src/microcode/util/filesys.h"
#include "src/microcode/grid/hub.h"

Rate r(4);
Rate r2(2);

GridMessageHub hub0(1);
GridMessageHub hub1(2);
GridMessageHub hub2(2);
GridMessageHub hub3(2);
GridMessageHub hub4(2);
GridMessageHub hub5(1);
// GridMessageHub hub6(2);
// GridMessageHub hub7(2);
// GridMessageHub hub8(2);
// GridMessageHub hub9(2);
// GridMessageHub hub10(1);

#define linkspeed 10000

void setup()
{
    Log("system") << "booting ...";
    suart0.init(115200);
    Log() >> *suart0.getTXStream();

    filesys.init();
    filesys.logMap(filesys.map("/"));

    hub0.setLinkSpeed(0, linkspeed);
    hub1.setLinkSpeed(0, linkspeed);
    hub1.setLinkSpeed(1, linkspeed);
    hub2.setLinkSpeed(0, linkspeed);
    hub2.setLinkSpeed(1, linkspeed);
    hub3.setLinkSpeed(0, linkspeed);
    hub3.setLinkSpeed(1, linkspeed);
    hub4.setLinkSpeed(0, linkspeed);
    hub4.setLinkSpeed(1, linkspeed);
    hub5.setLinkSpeed(0, linkspeed);
    // hub5.setLinkSpeed(1, linkspeed);
    // hub6.setLinkSpeed(0, linkspeed);
    // hub6.setLinkSpeed(1, linkspeed);
    // hub7.setLinkSpeed(0, linkspeed);
    // hub7.setLinkSpeed(1, linkspeed);
    // hub8.setLinkSpeed(0, linkspeed);
    // hub8.setLinkSpeed(1, linkspeed);
    // hub9.setLinkSpeed(0, linkspeed);
    // hub9.setLinkSpeed(1, linkspeed);
    // hub10.setLinkSpeed(0, linkspeed);
}

void sendPackets(GridMessageHub& h0, GridMessageHub& h1, unsigned b0, unsigned b1)
{
    while (!h0[b0].out.empty())
    {
        h1[b1].in.packets.push(h0[b0].out.top());
        h0[b0].out.pop();
    }

    while (!h1[b1].out.empty())
    {
        h0[b0].in.packets.push(h1[b1].out.top());
        h1[b1].out.pop();
    }
}

void loop()
{
    if (r.isReady())
        Log("MemoryUsage") << sysMemUsage();

    suart0.read();
    ByteStream* rx = suart0.getRXStream();
    rx->lock();
    while (!rx->isEmptyUnlocked())
    {
        if (rx->getUnlocked() == (uint8_t) 't')
            esp_restart();
    }
    rx->unlock();

    // 0[0] <-> 1[0]
    sendPackets(hub0, hub1, 0, 0);

    // 1[1] <-> 2[0]
    sendPackets(hub1, hub2, 1, 0);

    // 2[1] <-> 3[0]
    sendPackets(hub2, hub3, 1, 0);

    // 3[1] <-> 4[0]
    sendPackets(hub3, hub4, 1, 0);

    // 4[1] <-> 5[0]
    sendPackets(hub4, hub5, 1, 0);

    // // 5[1] <-> 6[0]
    // sendPackets(hub5, hub6, 1, 0);

    // // 6[1] <-> 7[0]
    // sendPackets(hub6, hub7, 1, 0);

    // // 7[1] <-> 8[0]
    // sendPackets(hub7, hub8, 1, 0);

    // // 8[1] <-> 9[0]
    // sendPackets(hub8, hub9, 1, 0);

    // // 9[1] <-> 10[0]
    // sendPackets(hub9, hub10, 1, 0);

    uint64_t updateStart = sysTime();
    hub0.update();
    uint64_t updateDT = sysTime() - updateStart;
    hub1.update();
    hub2.update();
    hub3.update();
    hub4.update();
    hub5.update();
    // hub6.update();
    // hub7.update();
    // hub8.update();
    // hub9.update();
    // hub10.update();

    if (r2.isReady())
    {
        Log("hub0") << hub0._table.toString();
        Log("hub0") << "\n" << hub0._graph.toString();
        Log("hub0") << "bytes: " << hub0.totalBytes();
        Log("hub0") << "dt: " << updateDT;


        std::vector<std::vector<uint16_t>> paths(hub0._table.nodes.size() * hub0._table.nodes.size());
        uint64_t start1, dt1, start2, dt2;
        unsigned i;

        start1 = sysTime();
        i = 0;
        for (auto it = hub0._table.nodes.begin(); it != hub0._table.nodes.end(); ++it)
        {
            for (auto ti = hub0._table.nodes.begin(); ti != hub0._table.nodes.end(); ++ti)
                hub0._graph.path(paths[i++], it.key(), ti.key());
        }
        dt1 = sysTime() - start1;

        // i = 0;
        // for (auto it = hub0._table.nodes.begin(); it != hub0._table.nodes.end(); ++it)
        // {
        //     for (auto ti = hub0._table.nodes.begin(); ti != hub0._table.nodes.end(); ++ti)
        //         Log("hub0") << "Path " << it.key() << " -> " << ti.key() << " " << hub0._graph.pathToString(paths[i++]);
        // }

        start2 = sysTime();
        i = 0;
        for (auto it = hub0._table.nodes.begin(); it != hub0._table.nodes.end(); ++it)
        {
            for (auto ti = hub0._table.nodes.begin(); ti != hub0._table.nodes.end(); ++ti)
                hub0._graph.path(paths[i++], it.key(), ti.key());
        }
        dt2 = sysTime() - start1;

        // i = 0;
        // for (auto it = hub0._table.nodes.begin(); it != hub0._table.nodes.end(); ++it)
        // {
        //     for (auto ti = hub0._table.nodes.begin(); ti != hub0._table.nodes.end(); ++ti)
        //         Log("hub0") << "Path " << it.key() << " -> " << ti.key() << " " << hub0._graph.pathToString(paths[i++]);
        // }

        Log("djikstra") << dt1;
        Log("BFS") << dt2;
    }
}
