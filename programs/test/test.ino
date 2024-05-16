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

Rate                robotStatePost(50);

SPIClass            spiBus(VSPI);
MCP23S08            mcp(spiBus, MCP_CS, MCP_RST);
ADS1120             adc(spiBus, ADC_CS, ADC_DRDY);
Adafruit_BNO08x     imu(IMU_RST);

PVar<WiFiData>      wifiData("/wifi");

GridMessageHub      hub(2);
SharedGridBuffer    robotState("RobotState", (int) GridMessageHub::Priority::USER);
SharedGridBuffer    robotControl("RobotControl", (int) GridMessageHub::Priority::USER);
SharedGridBuffer    robotLighting("RobotLighting", (int) GridMessageHub::Priority::USER);

unsigned            adcReadPin = 0;
sh2_SensorValue_t   sensorValue;
StageState          ss;
RunPixelsArgs       rpa;

void setup()
{
    Log("init") << "Begin initialization...";

    if (suart0.init(115200))
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

    xTaskCreateUniversal(
        runPixels, 
        "runPixels", 
        4 * 1024, 
        &rpa, 
        1, 
        NULL, 
        1);

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
        if (success && !MDNS.addService("http", "tcp", ota.init(80)))
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

    {
        Log log("init");
        log << "Comm 0";
        if (!suart1.init(COMM_BAUD, RX0, TX0, false))
            log.failed();
        else
        {
            log.success();
            ss.init.comm0 = 1;
        }
    }

    {
        Log log("init");
        log << "Comm 1";
        if (!suart2.init(COMM_BAUD, RX1, TX1, false))
            log.failed();
        else
        {
            log.success();
            ss.init.comm1 = 1;
        }
    }

    hub.setLinkSpeed(0, COMM_BAUD);
    hub.setLinkSpeed(1, COMM_BAUD);
    hub.setName(wifiData.data.host);
    hub.listenFor(robotState);
    hub.listenFor(robotControl);
    hub.listenFor(robotLighting);
    hub.setInputStream(0, *suart1.getRXStream());
    hub.setOutputStream(0, *suart1.getTXStream());
    hub.setInputStream(1, *suart2.getRXStream());
    hub.setOutputStream(1, *suart2.getTXStream());
    
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

    if (ss.init.adc)
    {
        ss.sensors.pressure[adcReadPin++] = adcVoltageToPressure(adc.readVoltage(ADC_REF));
        adcReadPin %= ADC_PORTS;
        adc.setMux(adcReadPin + ADS1120::AIN0_AVSS);
    }

    suart1.read();
    suart2.read();

    hub.updateTop(0);
    hub.updateTop(1);
    hub.updateBottom();

    suart1.write();
    suart2.write();

    if (robotStatePost.isReady())
    {
        robotState.lock();
        SharedBuffer& buf = robotState.touch(hub.id());
        buf.resize(sizeof(StageState));
        ss.timestamp = sysTime();
        memcpy(buf.data(), (uint8_t*) &ss, sizeof(StageState));
        robotState.unlock();
    }

    robotControl.lock();
    if (robotControl.canRead(hub.id()))
    {
        ss.drivers = *((StageState::Drivers*) robotControl.data[hub.id()].buf.data());
        robotControl.unlock();
        uint8_t byte;
        for (unsigned i = 0; i < 8; ++i)
        {
            if (*((uint8_t*) &ss.drivers) & (1 << i))
                byte |= (1 << driverMap[i]);
            else
                byte &= ~(1 << driverMap[i]);
        }
        mcp.write8(byte);
    }
    else
        robotControl.unlock();
    
    robotLighting.lock();
    if (robotLighting.canRead(hub.id()))
    {
        SetLEDS leds;
        if (robotLighting.data[hub.id()].buf.size() >= sizeof(SetLEDS))
            leds = *((SetLEDS*) robotLighting.data[hub.id()].buf.data());
        robotLighting.unlock();

        rpa.lock.lock();
        for (unsigned i = 0; i < NUM_PXL; ++i)
            rpa.leds[i].setRGB(leds.rgb[i].r, leds.rgb[i].g, leds.rgb[i].b);
        rpa.set = true;
        rpa.timer.reset();
        rpa.lock.unlock();
    }
    else
        robotLighting.unlock();
}
