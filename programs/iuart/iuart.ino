#include "src/microcode/util/log.h"
#include "src/microcode/hardware/suart.h"
#include "src/microcode/util/rate.h"
#include "src/microcode/util/filesys.h"
#include "src/microcode/grid/hub.h"
#include "src/microcode/usb/usbcom.h"

Rate r(1);
Rate r2(0.1);
Rate r3(1);

GridMessageHub base(1);
GridMessageHub hubs[] = 
{
    GridMessageHub(2),
    GridMessageHub(2),
    GridMessageHub(2),
    GridMessageHub(2),
    GridMessageHub(1)
};

SharedGridBuffer sharedStates[] = 
{
    SharedGridBuffer("RobotState", (int) GridMessageHub::Priority::USER),
    SharedGridBuffer("RobotState", (int) GridMessageHub::Priority::USER),
    SharedGridBuffer("RobotState", (int) GridMessageHub::Priority::USER),
    SharedGridBuffer("RobotState", (int) GridMessageHub::Priority::USER),
    SharedGridBuffer("RobotState", (int) GridMessageHub::Priority::USER),
};

USBMessageBroker usb;

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

    while (true)
    {
        xTaskNotifyWait(0, UINT32_MAX, &notification, 2 / portTICK_PERIOD_MS);
        base.update();
        for (unsigned i = 0; i < sizeof(hubs) / sizeof(GridMessageHub); i++)
            hubs[i].update();

        if (notification & BIT(0))
            vTaskDelete(NULL);
    }
}

void transportPackets(void* args)
{
    uint32_t notification = 0;

    while (true)
    {
        xTaskNotifyWait(0, UINT32_MAX, &notification, 2 / portTICK_PERIOD_MS);
        sendPackets(base, hubs[0], 0, 1);
        for (unsigned i = 0; i < (sizeof(hubs) / sizeof(GridMessageHub)) - 2; ++i)
            sendPackets(hubs[i], hubs[i + 1], 0, 1);
        sendPackets(hubs[3], hubs[4], 0, 0);

        if (notification & BIT(0))
            vTaskDelete(NULL);
    }
}

void updateUSB(void* args)
{
    uint32_t notification = 0;

    while (true)
    {
        xTaskNotifyWait(0, UINT32_MAX, &notification, 2 / portTICK_PERIOD_MS);
        suart0.read();
        usb.update();
        suart0.write();

        if (notification & BIT(0))
            vTaskDelete(NULL);
    }
}

void setup()
{
    Log("system") << "booting ...";
    suart0.init(2000000, 3, 1);
    Log() >> *suart0.getTXStream();

    filesys.init();
    Log("filesys") << filesys.toString(); 

    for (unsigned i = 0; i < sizeof(hubs) / sizeof(GridMessageHub); ++i)
        hubs[i].listenFor(sharedStates[i]);

    base.setLinkSpeed(0, linkspeed);
    for (unsigned i = 0; i < sizeof(hubs) / sizeof(GridMessageHub) - 1; ++i)
    {
        hubs[i].setLinkSpeed(0, linkspeed);
        hubs[i].setLinkSpeed(1, linkspeed);
    }
    hubs[4].setLinkSpeed(0, linkspeed);

    base.setName("base");
    hubs[0].setName("module0");
    hubs[1].setName("module1");
    hubs[2].setName("module2");
    hubs[3].setName("module3");
    hubs[4].setName("module4");

    usb.attachStreams(suart0.getTXStream(), suart0.getRXStream());

    xTaskCreateUniversal(transportPackets, "transport", 4 * 1024, NULL, 11, NULL, tskNO_AFFINITY);
    xTaskCreateUniversal(updateHub, "updatehub", 4 * 1024, NULL, 10, NULL, tskNO_AFFINITY);
    //xTaskCreateUniversal(updateUSB, "updateUSB", 4 * 1024, NULL, 12, NULL, tskNO_AFFINITY);
}

void loop()
{
    // if (r.isReady())
    // {
    //     //Log("MemoryUsage") << sysMemUsage();

    //     for (unsigned i = 0; i < sizeof(hubs) / sizeof(GridMessageHub) - 1; ++i)
    //     {
    //         std::stringstream ss;
    //         ss << sysTime() << " hub" << i;
    //         std::string str = ss.str();
    //         SharedBuffer buf = SharedBuffer((const uint8_t*) &str[0], str.size() + 1);
    //         buf.data()[buf.size() - 1] = 0;
    //         sharedStates[i].lock();
    //         sharedStates[i].touch(hubs[i].id()) = buf;
    //         sharedStates[i].unlock();
    //     }
    // }

    // for (unsigned i = 0; i < sizeof(hubs) / sizeof(GridMessageHub) - 1; ++i)
    // {
    //     sharedStates[i].lock();
    //     for (auto it = sharedStates[i].data.begin(); it != sharedStates[i].data.end(); ++it)
    //     {
    //         if (sharedStates[i].canRead(it.key()))
    //             Log("sharedstate") << "hub " << i << "id: " << hubs[i].id() << " _id: " << it.key() << " arrive: " << it->arrival << " msg: " << (char*) it->buf.data(); 
    //     }
    //     sharedStates[i].unlock();
    // }

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

    if (r.isReady())
    {
        NetworkTable::Nodes nodes = base.getTableNodes();
        Log("base") << nodes.toString();
    }

    // if (r2.isReady())
    // {
    //     NetworkTable::Nodes nodes = base.getTableNodes();
    //     GridGraph graph;
    //     base.getGraph(graph);
        // Log("base") << nodes.toString();
        // Log("base") << graph.toString();
        // Log("base") << "bytes: " << base.totalBytes();


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
    //}
}
