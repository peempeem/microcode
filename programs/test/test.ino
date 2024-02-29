#include "src/microcode/util/log.h"
#include "src/microcode/MCP23S08/mcp23s08.h"
#include "src/microcode/ADS1120/ads1120.h"
#include "src/microcode/util/filesys.h"
#include "src/microcode/util/pvar.h"
#include "src/tasks.h"
#include "src/func.h"

#include <Adafruit_BNO08x.h>
#include <SPI.h>
#include <WiFi.h>
#include <ESPmDNS.h>

Rate                debugMem(4);
Rate                hubPost(1);
Rate                robotStatePost(100);

SPIClass            spiBus(VSPI);
MCP23S08            mcp(spiBus, MCP_CS, MCP_RST);
ADS1120             adc(spiBus, ADC_CS, ADC_DRDY);
Adafruit_BNO08x     imu(IMU_RST);

PVar<WiFiData>      wifiData("/wifi");

GridMessageHub      hub(2);
SharedGridBuffer    robotState("RobotState", 1);

unsigned            adcReadPin = 0;
sh2_SensorValue_t   sensorValue;
StageState          ss;

void setup()
{
    Log("init") << "Begin initialization...";

    if (suart0.init(1000000))
    {
        Log() >> *suart0.getTXStream();
        ss.init.debug = 1;
    }
    else
        Log().disable();
    
    {
        Log log("init");
        log << "File System";
        if (!filesys.init() && !filesys.init())
        {
            log.failed();
            while (1);
        }
        log.success();

        Log("init") << filesys.toString();
        ss.init.filesys = 1;
    }

    pinMode(MCP_CS, OUTPUT);
    pinMode(ADC_CS, OUTPUT);
    pinMode(IMU_CS, OUTPUT);
    digitalWrite(MCP_CS, HIGH);
    digitalWrite(ADC_CS, HIGH);
    digitalWrite(IMU_CS, HIGH);
    spiBus.begin();

    for (unsigned i = 0; i < 10; ++i) 
    {
        Log log("init");
        log << "MCP GPIO Extender Attempt " << i;
        if (!mcp.begin())
            log.failed();
        else
        {
            for (unsigned i = 0; i < 8; i++)
                mcp.setMode(i, OUTPUT);
            log.success();
            ss.init.mcp = 1;
            break;
        }
    }
    
    for (unsigned i = 0; i < 10; ++i)
    {
        Log log("init");
        log << "ADS1120 ADC Attempt " << i;
        if (!adc.begin())
            log.failed();
        else
        {
            adc.setGain(ADS1120::Gain1);
            adc.setDataRate(ADS1120::T2000);
            adc.setVoltageRef(ADS1120::External_REFP0_REFN0); 
            adc.setMux(ADS1120::AIN0_AVSS);
            adc.setConversionMode(ADS1120::Continuous);
            log.success();
            ss.init.adc = 1;
            break;
        }
    }
    
    for (unsigned i = 0; i < 10; ++i)
    {
        Log log("init");
        log << "BNO085 IMU Attempt " << i;
        if (!imu.begin_SPI(IMU_CS, IMU_INT, &spiBus))
            log.failed();
        else
        {
            log.success();
            ss.init.imu = 1;
            break;
        }
    }

    //if (strcmp(wifiData.data.host, "module3") == 0)
    {
        Log log("init");
        log << "Comm 0";
        if (!suart1.init(COMM_BAUD, RX0, TX0, false))
            log.failed();
        else
        {
            log.success();
            ss.init.comm0 = 1;

            xTaskCreateUniversal(
                transferPackets, 
                "pktTrans0", 
                4 * 1024, 
                (void*) new TransferPacketArgs(&hub, 0, &suart1), 
                15, 
                NULL, 
                tskNO_AFFINITY);
        }
    }

    //if (strcmp(wifiData.data.host, "module2") == 0)
    {
        Log log("init");
        log << "Comm 1";
        if (!suart2.init(COMM_BAUD, RX1, TX1, false))
            log.failed();
        else
        {
            log.success();
            ss.init.comm0 = 1;

            xTaskCreateUniversal(
                transferPackets, 
                "pktTrans1", 
                4 * 1024, 
                (void*) new TransferPacketArgs(&hub, 1, &suart2), 
                15, 
                NULL, 
                tskNO_AFFINITY);
        }
    }

    hub.setLinkSpeed(0, COMM_BAUD);
    hub.setLinkSpeed(1, COMM_BAUD);
    hub.listenFor(robotState);
    xTaskCreateUniversal(
        updateHub, 
        "updateHub", 
        6 * 1024, 
        (void*) &hub, 
        10, 
        NULL, 
        tskNO_AFFINITY);

    xTaskCreateUniversal(
    runPixels, 
    "runPixels", 
    4 * 1024, 
    NULL, 
    15, 
    NULL, 
    tskNO_AFFINITY);

    {
        Log log("init");
        log << "WiFi";

        bool success = true;
        if (success && !wifiData.load())
            success = false;
        if (success && !WiFi.mode(WIFI_AP_STA))
            success = false;
        if (success && !WiFi.begin(wifiData.data.ssid, wifiData.data.pass))
            success = false;
        if (success && !WiFi.setAutoReconnect(true))
            success = false;
        if (success && !MDNS.begin(wifiData.data.host))
            success = false;
        if (success && !MDNS.addService("http", "tcp", ota.init(80, false)))
            success = false;

        if (success)
        {
            log.success();
            ss.init.wifi = 1;
            Log("init") << "Hostname: " << wifiData.data.host;
            Log("init") << "Network: " << wifiData.data.ssid;
            Log("init") << "Password: " << wifiData.data.pass;
        }
        else
            log.failed();
    }
    
    Log("init") << "Initialization complete";
}

void loop()
{
    if (ss.init.imu)
    {
        if (imu.wasReset())
            imu.enableReport(SH2_ARVR_STABILIZED_RV, IMU_REPORT);
        
        if (imu.getSensorEvent(&sensorValue) && sensorValue.sensorId == SH2_ARVR_STABILIZED_RV)
        {
            quaternionToEuler(
                sensorValue.un.arvrStabilizedRV.real, 
                sensorValue.un.arvrStabilizedRV.i,
                sensorValue.un.arvrStabilizedRV.j,
                sensorValue.un.arvrStabilizedRV.k,
                &ss.sensors.ypr, 
                true);
        }
    }

    ss.sensors.pressure[adcReadPin++] = adcVoltageToPressure(adc.readVoltage(ADC_REF));
    adcReadPin %= ADC_PORTS;
    adc.setMux(adcReadPin + ADS1120::AIN0_AVSS);

    if (debugMem.isReady())
    {
        Log("MemoryUsage") << sysMemUsage();
        Log("MemoryUsageExt") << sysMemUsageExternal();
    }
    
    if (hubPost.isReady())
    {
        hub._lock.lock();
        Log("hub") << hub._table.toString();
        Log("hub") << hub._graph.toString();
        Log("hub") << "bytes: " << hub.totalBytes();
        hub._lock.unlock();
        Log("freee") << ESP.getMinFreeHeap();
    }

    if (robotStatePost.isReady())
    {
        robotState.lock();
        SharedBuffer& buf = robotState.touch(hub.id());
        buf.resize(sizeof(StageState));
        memcpy(buf.data(), (uint8_t*) &ss, sizeof(StageState));
        robotState.unlock();
    }

    // if (strcmp(wifiData.data.host, "module3") == 0)
    // {
    //     hub[1].out.lock();
    //     while (!hub[1].out.empty())
    //         hub[1].out.pop();
    //     hub[1].out.unlock();
    // }

    // if (strcmp(wifiData.data.host, "module2") == 0)
    // {
    //     hub[0].out.lock();
    //     while (!hub[0].out.empty())
    //         hub[0].out.pop();
    //     hub[0].out.unlock();
    // }

    robotState.lock();
    for (auto it = robotState.data.begin(); it != robotState.data.end(); ++it)
    {
        if (robotState.canRead(it.key()) && it.key() != hub.id())
        {
            StageState* sss = (StageState*) it->buf.data();
            Log("robotState") << "id: " << it.key() << " pyr:\t" << sss->sensors.ypr.pitch << "\t" << sss->sensors.ypr.yaw << "\t" << sss->sensors.ypr.roll << "\tpress:\t" << sss->sensors.pressure[0] << "\t" << sss->sensors.pressure[1] << "\t" << sss->sensors.pressure[2] << "\t" << sss->sensors.pressure[3];
        }
    }
    robotState.unlock();

    // if (suart1.read())
    // {
    //     ByteStream* stream = suart1.getRXStream();

    //     uint8_t buf[128];

    //     while (true)
    //     {
    //         stream->lock();
    //         if (stream->isEmptyUnlocked())
    //         {
    //             stream->unlock();
    //             break;
    //         }

    //         unsigned pulled = stream->getUnlocked(buf, 128);
    //         stream->unlock();

    //         for (unsigned i = 0; i < pulled; ++i)
    //             hub[0].in.insert(buf[i]);
    //     }
    // }

    // hub[0].out.lock();
    // while (!hub[0].out.empty())
    // {
    //     GridPacket& pkt = hub[0].out.top();
    //     suart1.write(pkt.raw(), pkt.size());
    //     hub[0].out.pop();
    // }
    // hub[0].out.unlock();

    // if (suart2.read())
    // {
    //     ByteStream* stream = suart2.getRXStream();

    //     uint8_t buf[128];

    //     while (true)
    //     {
    //         stream->lock();
    //         if (stream->isEmptyUnlocked())
    //         {
    //             stream->unlock();
    //             break;
    //         }

    //         unsigned pulled = stream->getUnlocked(buf, 128);
    //         stream->unlock();

    //         for (unsigned i = 0; i < pulled; ++i)
    //             hub[1].in.insert(buf[i]);
    //     }
    // }

    // hub[1].out.lock();
    // while (!hub[1].out.empty())
    // {
    //     GridPacket& pkt = hub[1].out.top();
    //     suart2.write(pkt.raw(), pkt.size());
    //     hub[1].out.pop();
    // }
    // hub[1].out.unlock();
    
    // hub.update();

    // while (usb.messages.size())
    // {
    //     USBMessageBroker::Message& msg = usb.messages.top();
    //     switch (msg.type())
    //     {
    //         case 1:
    //         {
    //             SetDevice* setdevcmd = (SetDevice*) msg.data();
    //             if (setdevcmd->on)
    //                 ss.drivers.all |= 1 << setdevcmd->driver;
    //             else
    //                 ss.drivers.all &= ~(1 << setdevcmd->driver);
    //             mcp.write(driverMap[setdevcmd->driver], setdevcmd->on);
    //             break;
    //         }
    //     }
    //     usb.messages.pop();
    // }

    // if (sendStateRate.isReady())
    // {
    //     ss.timestamp = sysTime();

    //     ss.power = 12 * 0.075;
    //     for (unsigned i = 0; i < 8; i++)
    //     {
    //         if (0x01 & (ss.drivers.all >> i))
    //             ss.power += 12 * 0.2;
    //     }

    //     ss.ypr[0] = ypr.yaw;
    //     ss.ypr[1] = ypr.pitch;
    //     ss.ypr[2] = ypr.roll;

    //     //usb.send(0, 0, (uint8_t*) &ss, sizeof(ss));
    // }

    //usb.update();

    // if ((strcmp(wifiData.data.host, "module1") == 0) || (strcmp(wifiData.data.host, "module3") == 0))
    // {
    //     ByteStream* bs = suart1.getTXStream();
    //     while (!hub[0].out.empty())
    //     {
    //         GridPacket& pkt = hub[0].out.top();
    //         bs->lock();
    //         bs->putUnlocked(pkt.raw(), pkt.size());
    //         bs->unlock();
    //         hub[0].out.pop();
    //     }

    //     bs = suart1.getRXStream();
    //     bs->lock();
    //     while (!bs->isEmptyUnlocked())
    //     {
    //         hub[0].in.insert(bs->getUnlocked());
    //     }
    //     bs->unlock();
    // }

    // if (hubDebug.isReady())
    // {
    //     Log("hub") << hub._table.toString();
    //     Log("hub") << hub._graph.toString();
    //     Log("hub") << "bytes: " << hub.totalBytes();
    // }

    
    // if (debug.isReady())
    // {
    //     if (ss.init.single.adc || ss.init.single.imu)
    //         logc("\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
            
    //     if (ss.init.single.adc)
    //     {
    //         log("ADC", "Voltage: ", false);
    //         logc(adc.readVoltage());
    //     }

    //     if (ss.init.single.imu)
    //     {
    //         log("IMU", "Yaw: ", false);
    //         logc(ypr.yaw, false);
    //         logc("\tPitch: ", false);
    //         logc(ypr.pitch, false);
    //         logc("\tRoll: ", false);
    //         logc(ypr.roll);
    //     }
    // }

    // while (comm.available())
    //     comm.write(comm.read());
    
    // while (comm.available())
    // {
    //     trans.insert(comm.read());
    //     if (trans.packets.size())
    //     {
    //         log("recv", sysMicros() - start, false);
    //         logc('\t', false);
    //         logc((char*) trans.packets.front().get().data);
    //         trans.packets.pop();
    //     }
    // }   
}
