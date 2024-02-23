#pragma once

#include "../hal/hal.h"
#include <SPI.h>

class ADS1120
{
    public:
        enum Multiplexer
        {
            AIN0_AIN1,
            AIN0_AIN2,
            AIN0_AIN3,
            AIN1_AIN2,
            AIN1_AIN3,
            AIN2_AIN3,
            AIN1_AIN0,
            AIN3_AIN2,
            AIN0_AVSS,
            AIN1_AVSS,
            AIN2_AVSS,
            AIN3_AVSS,
            REF4_MON,
            APWR4_MON
        };

        enum Gain
        {
            Gain1,
            Gain2,
            Gain4,
            Gain8,
            Gain16,
            Gain32,
            Gain64,
            Gain128
        };

        enum DataRate
        {
            N20,
            N45,
            N90,
            N175,
            N330,
            N600,
            N1000,
            
            DC5,
            DC11_25,
            DC22_5,
            DC44,
            DC82_5,
            DC150,
            DC250,

            T40,
            T90,
            T180,
            T350,
            T660,
            T1200,
            T2000
        };

        enum PowerMode
        {
            NormalMode,
            DutyCycleMode,
            TurboMode
        };

        enum ConversionMode
        {
            SingleShot,
            Continuous
        };

        enum TemperatureMode
        {
            DisableTemp,
            EnableTemp
        };

        enum VoltageReference
        {
            Internal_2_048V,
            External_REFP0_REFN0,
            External_AIN0_AIN3_or_REFP1_REFN1,
            AnalogSupply
        };

        enum Filters
        {
            NoFilter,
            Filter50_60Hz,
            Filter50Hz,
            Filter60Hz
        };

        enum PowerSwitch
        {
            Open,
            Automatic
        };

        enum IDACCurrent
        {
            IDAC_Off,
            IDAC_10uA,
            IDAC_50uA,
            IDAC_100uA,
            IDAC_250uA,
            IDAC_500uA,
            IDAC_1000uA,
            IDAC_1500uA
        };

        enum IDACRouting
        {
            IDAC_Disabled,
            IDAC_AIN0_REFP1,
            IDAC_AIN1,
            IDAC_AIN2,
            IDAC_AIN3_REFN1,
            IDAC_REFP0,
            IDAC_REFN0
        };

        enum DRDYBehavior
        {
            DRDY_Only,
            DOUT_DRDY
        };

        ADS1120(SPIClass& spi, unsigned cs, unsigned drdy, unsigned clkspeed=5e6);

        bool begin();
        
        void reset();
        void powerDown();

        float readVoltage(float ref);
        float readTemperature();

        void setMux(unsigned mux);
        void setGain(unsigned gain);
        void setPGABypass(bool bypass);
        void setDataRate(unsigned rate);
        void setOpMode(unsigned mode);
        void setConversionMode(unsigned mode);
        void setTemperatureMode(unsigned mode);
        void setBurnoutCurrentSources(bool enabled);
        void setVoltageRef(unsigned ref);
        void setFIR(unsigned filter);
        void setPowerSwitch(unsigned mode);
        void setIDACCurrent(unsigned current);
        void setIDAC1Routing(unsigned routing);
        void setIDAC2Routing(unsigned routing);
        void setDRDYMode(unsigned value);

    private:
        union Registers
        {
            struct Register0
            {
                uint8_t pga_bypass  : 1;
                uint8_t gain_config : 3;
                uint8_t mux_config  : 4;
            } reg0;

            struct Register1
            {
                uint8_t burnout_current_sources     : 1;
                uint8_t temperature_sensor_mode     : 1;
                uint8_t conversion_mode             : 1;
                uint8_t operating_mode              : 2;
                uint8_t data_rate                   : 3;
            } reg1;

            struct Register2
            {
                uint8_t idac_current_setting        : 3;
                uint8_t power_switch_config         : 1;
                uint8_t fir_filter_config           : 2;
                uint8_t voltage_reference_selection : 2;
            } reg2;

            struct Register3
            {
                uint8_t reserved                : 1;
                uint8_t drdy_mode               : 1;
                uint8_t idac2_routing_config    : 3;
                uint8_t idac1_routing_config    : 3;
            } reg3;

            uint8_t raw;

            Registers() {}
            Registers(uint8_t data) : raw(data) {}
        };

        SPIClass* _spi;
        unsigned _cs;
        unsigned _drdy;
        unsigned _clkspeed;
        int _mode = -1;
        int _cmode = -1;
        unsigned _gain = 1;
        bool _initialized = false;

        void _sendCommand(uint8_t cmd);
        void _writeReg(uint8_t addr, Registers reg);
        Registers _readReg(uint8_t addr);
        bool _readData(uint16_t& data, unsigned timeout=5e5);
};
