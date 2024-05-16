#include "src/microcode/util/log.h"
#include "src/microcode/hardware/suart.h"
#include "src/microcode/util/rate.h"
#include "src/microcode/util/filesys.h"
#include "src/microcode/grid/hub.h"
#include "src/microcode/usb/usbcom.h"

Rate r(1);
Rate r2(100);
Rate r3(1);

GridMessageHub base(1);
GridMessageHub hubs[] = 
{
    GridMessageHub(2),
    GridMessageHub(1)
};

SharedGridBuffer sharedStates[] = 
{
    SharedGridBuffer("RobotState", (int) GridMessageHub::Priority::USER),
    SharedGridBuffer("RobotState", (int) GridMessageHub::Priority::USER)
};

#define linkspeed 10000

ByteStream base0RX_hub00TX;
ByteStream base0TX_hub00RX;
ByteStream hub01RX_hub10TX;
ByteStream hub01TX_hub10RX;

struct RunTopArgs
{
    GridMessageHub* hub;
    unsigned port;

    RunTopArgs(GridMessageHub& hub, unsigned port) : hub(&hub), port(port) {}
};

void runTop(void* args)
{
    RunTopArgs* rta = (RunTopArgs*) args;
    uint32_t notification;

    while (true)
    {
        rta->hub->updateTop(rta->port);
        xTaskNotifyWait(0, UINT32_MAX, &notification, 1);
    }
}

struct RunBottomArgs
{
    GridMessageHub* hub;
    TaskHandle_t handle0;
    TaskHandle_t handle1;

    RunBottomArgs(GridMessageHub& hub, TaskHandle_t handle0) 
        : hub(&hub)
    {
        this->handle0 = handle0;
        this->handle1 = NULL;
    }

    RunBottomArgs(GridMessageHub& hub, TaskHandle_t handle0, TaskHandle_t handle1) 
        : hub(&hub)
    {
        this->handle0 = handle0;
        this->handle1 = handle1;
    }
};

void runBottom(void* args)
{
    RunBottomArgs* rba = (RunBottomArgs*) args;
    uint32_t notification;

    while (true)
    {
        rba->hub->updateBottom();
        xTaskNotify(rba->handle0, 0, eNoAction);
        if (rba->handle1)
            xTaskNotify(rba->handle1, 0, eNoAction);
        xTaskNotifyWait(0, UINT32_MAX, &notification, 1);
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
    hubs[0].setLinkSpeed(0, linkspeed);
    hubs[0].setLinkSpeed(1, linkspeed);
    hubs[1].setLinkSpeed(0, linkspeed);

    base.setName("base");
    hubs[0].setName("module0");
    hubs[1].setName("module1");

    base.setInputStream(0, base0RX_hub00TX);
    base.setOutputStream(0, base0TX_hub00RX);
    hubs[0].setInputStream(0, base0TX_hub00RX);
    hubs[0].setOutputStream(0, base0RX_hub00TX);
    hubs[0].setInputStream(1, hub01RX_hub10TX);
    hubs[0].setOutputStream(1, hub01TX_hub10RX);
    hubs[1].setInputStream(0, hub01TX_hub10RX);
    hubs[1].setOutputStream(0, hub01RX_hub10TX);

    TaskHandle_t base0Handle;
    xTaskCreateUniversal(
        runTop, 
        "runtop", 
        1024 * 5,
        new RunTopArgs(base, 0),
        10,
        &base0Handle,
        tskNO_AFFINITY);
    
    TaskHandle_t hub00Handle;
    xTaskCreateUniversal(
        runTop, 
        "runtop", 
        1024 * 5,
        new RunTopArgs(hubs[0], 0),
        10,
        &hub00Handle,
        tskNO_AFFINITY);

    TaskHandle_t hub01Handle;
    xTaskCreateUniversal(
        runTop, 
        "runtop", 
        1024 * 5,
        new RunTopArgs(hubs[0], 1),
        10,
        &hub01Handle,
        tskNO_AFFINITY);
    
    TaskHandle_t hub10Handle;
    xTaskCreateUniversal(
        runTop, 
        "runtop", 
        1024 * 5,
        new RunTopArgs(hubs[1], 0),
        10,
        &hub10Handle,
        tskNO_AFFINITY);
    
    xTaskCreateUniversal(
        runBottom, 
        "runbot", 
        1024 * 5,
        new RunBottomArgs(base, base0Handle),
        5,
        NULL,
        tskNO_AFFINITY);
    
    xTaskCreateUniversal(
        runBottom, 
        "runbot", 
        1024 * 5,
        new RunBottomArgs(hubs[0], hub00Handle, hub01Handle),
        5,
        NULL,
        tskNO_AFFINITY);

    xTaskCreateUniversal(
        runBottom, 
        "runbot", 
        1024 * 5,
        new RunBottomArgs(hubs[1], hub10Handle),
        5,
        NULL,
        tskNO_AFFINITY);
}

void loop()
{ 
    if (r2.isReady())
    {
        for (unsigned i = 0; i < sizeof(hubs) / sizeof(GridMessageHub); ++i)
        {
            std::stringstream ss;
            ss << sysTime() << " hub" << i;
            std::string str = ss.str();
            SharedBuffer buf = SharedBuffer((const uint8_t*) &str[0], str.size() + 1);
            buf.data()[buf.size() - 1] = 0;
            sharedStates[i].lock();
            sharedStates[i].touch(hubs[i].id()) = buf;
            sharedStates[i].unlock();
        }
    }

    for (unsigned i = 0; i < sizeof(hubs) / sizeof(GridMessageHub); ++i)
    {
        sharedStates[i].lock();
        for (auto it = sharedStates[i].data.begin(); it != sharedStates[i].data.end(); ++it)
        {
            if (sharedStates[i].canRead(it.key()))
                Log("sharedstate") << "hub " << i << "id: " << hubs[i].id() << " _id: " << it.key() << " arrive: " << it->arrival << " msg: " << (char*) it->buf.data(); 
        }
        sharedStates[i].unlock();
    }

    ByteStream* rx = suart0.getRXStream();
    uint8_t buf[256];
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
        GridGraph g;
        base.getGraph(g);
        Log("base") << nodes.toString();
        Log("base") << g.toString();
        Log("MemoryUsage") << sysMemUsage();
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
