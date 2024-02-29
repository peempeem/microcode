#include "ads1120.h"
#include "../util/log.h"

#define CMD_POWER_DOWN      0x03
#define CMD_RESET           0x07
#define CMD_START_SYNC      0x08
#define CMD_READ_DATA       0x1F
#define CMD_READ_REG        0x20
#define CMD_WRITE_REG       0x40

ADS1120::ADS1120(SPIClass& spi, unsigned cs, unsigned drdy, unsigned clkspeed)
    : _spi(&spi), _cs(cs), _drdy(drdy), _clkspeed(clkspeed)
{

}

bool ADS1120::begin()
{
    pinMode(_cs, OUTPUT);
    digitalWrite(_cs, HIGH);

    pinMode(_drdy, INPUT);

    reset();
    _sendCommand(CMD_START_SYNC);

    _initialized = true;
    Registers reg = _readReg(0);
    reg.reg0.gain_config = Gain64;
    _writeReg(0, reg);
    reg = _readReg(0);

    if (reg.reg0.gain_config == Gain64)
    {
        reg.reg0.gain_config = Gain1;
        _writeReg(0, reg);
        return true;
    }
    _initialized = false;
    return false;
}

void ADS1120::reset()
{
    _sendCommand(CMD_RESET);
    vTaskDelay(5);
}

void ADS1120::powerDown()
{
    _sendCommand(CMD_POWER_DOWN);
}

float ADS1120::readVoltage(float ref)
{
    uint16_t data;
    if (!_readData(data))
        return nanf("");
    
    return ref * ((int16_t) data / 32768.0f) / (float) _gain;
}

float ADS1120::readTemperature()
{
    uint16_t data;
    if (!_readData(data))
        return -1;
    
    data = data >> 2;

    if (data >= 8192)
        return (~(data - 1) ^ 0xC000) * -0.03125f;
    return data * 0.03125f;
}

void ADS1120::setMux(unsigned mux)
{
    if (mux <= APWR4_MON)
    {
        Registers reg = _readReg(0);
        reg.reg0.mux_config = mux;
        _writeReg(0, reg);
    }
}

void ADS1120::setGain(unsigned gain)
{
    if (gain <= Gain128)
    {
        Registers reg = _readReg(0);
        reg.reg0.gain_config = gain;
        _writeReg(0, reg);
        _gain = 1 << gain;
    }
}

void ADS1120::setPGABypass(bool bypass)
{
    Registers reg = _readReg(0);
    reg.reg0.pga_bypass = bypass;
    _writeReg(0, reg);
}

void ADS1120::setDataRate(unsigned rate)
{
    if (rate <= N1000)
    {
        if (_mode != NormalMode)
            setOpMode(NormalMode);
    }
    else if (rate <= DC250)
    {
        if (_mode != DutyCycleMode)
            setOpMode(DutyCycleMode);
        rate -= DC5;
    }
    else if (rate <= T2000)
    {
        if (_mode != TurboMode)
            setOpMode(TurboMode);
        rate -= T40;
    }
    else
        return;
    
    Registers reg = _readReg(1);
    reg.reg1.data_rate = rate;
    _writeReg(1, reg);
}

void ADS1120::setOpMode(unsigned mode)
{
    if (mode <= TurboMode)
    {
        _mode = mode;
        Registers reg = _readReg(1);
        reg.reg1.operating_mode = mode;
        _writeReg(1, reg);
    }
}

void ADS1120::setConversionMode(unsigned mode)
{
    if (mode <= Continuous)
    {
        _cmode = mode;
        Registers reg = _readReg(1);
        reg.reg1.conversion_mode = mode;
        _writeReg(1, reg);

        if (mode == Continuous)
            _sendCommand(CMD_START_SYNC);
    }
}

void ADS1120::setTemperatureMode(unsigned mode)
{
    if (mode <= EnableTemp)
    {
        Registers reg = _readReg(1);
        reg.reg1.temperature_sensor_mode = mode;
        _writeReg(1, reg);
    }
}

void ADS1120::setBurnoutCurrentSources(bool enabled)
{
    Registers reg = _readReg(1);
    reg.reg1.burnout_current_sources = enabled;
    _writeReg(1, reg);
}

void ADS1120::setVoltageRef(unsigned ref)
{
    if (ref <= AnalogSupply)
    {
        Registers reg = _readReg(2);
        reg.reg2.voltage_reference_selection = ref;
        _writeReg(2, reg);
    }
}

void ADS1120::setFIR(unsigned filter)
{
    if (filter <= Filter60Hz)
    {
        Registers reg = _readReg(2);
        reg.reg2.fir_filter_config = filter;
        _writeReg(2, reg);
    }
}

void ADS1120::setPowerSwitch(unsigned mode)
{
    if (mode <= Automatic)
    {
        Registers reg = _readReg(2);
        reg.reg2.power_switch_config = mode;
        _writeReg(2, reg);
    }
}

void ADS1120::setIDACCurrent(unsigned current)
{
    if (current <= IDAC_1500uA)
    {
        Registers reg = _readReg(2);
        reg.reg2.idac_current_setting = current;
        _writeReg(2, reg);
    }
}

void ADS1120::setIDAC1Routing(unsigned routing)
{
    if (routing <= IDAC_REFN0)
    {
        Registers reg = _readReg(3);
        reg.reg3.idac1_routing_config = routing;
        _writeReg(3, reg);
    }
}

void ADS1120::setIDAC2Routing(unsigned routing)
{
    if (routing <= IDAC_REFN0)
    {
        Registers reg = _readReg(3);
        reg.reg3.idac2_routing_config = routing;
        _writeReg(3, reg);
    }
}

void ADS1120::setDRDYMode(unsigned mode)
{
    if (mode <= DOUT_DRDY)
    {
        Registers reg = _readReg(3);
        reg.reg3.drdy_mode = mode;
        _writeReg(3, reg);
    }
}

void ADS1120::_sendCommand(uint8_t cmd)
{
    _spi->beginTransaction(SPISettings(_clkspeed, MSBFIRST, SPI_MODE1));
    digitalWrite(_cs, LOW);
    _spi->transfer(cmd);
    digitalWrite(_cs, HIGH);
    _spi->endTransaction();
}

void ADS1120::_writeReg(uint8_t addr, Registers reg)
{
    if (!_initialized)
        return;
    
    _spi->beginTransaction(SPISettings(_clkspeed, MSBFIRST, SPI_MODE1));
    digitalWrite(_cs, LOW);
    _spi->transfer(CMD_WRITE_REG | (addr << 2));
    _spi->transfer(reg.raw);
    digitalWrite(_cs, HIGH);
    _spi->endTransaction();
}

ADS1120::Registers ADS1120::_readReg(uint8_t addr)
{
    if (!_initialized)
        return Registers();
    
    _spi->beginTransaction(SPISettings(_clkspeed, MSBFIRST, SPI_MODE1));
    digitalWrite(_cs, LOW);
    _spi->transfer(CMD_READ_REG | (addr << 2));
    Registers reg = _spi->transfer(0xFF);
    digitalWrite(_cs, HIGH);
    _spi->endTransaction();
    return reg;
}

bool ADS1120::_readData(uint16_t& data, unsigned timeout)
{
    if (!_initialized)
        return false;
    
    _spi->beginTransaction(SPISettings(_clkspeed, MSBFIRST, SPI_MODE1));
    digitalWrite(_cs, LOW);

    if (_cmode == SingleShot)
        _spi->transfer(CMD_START_SYNC);

    uint64_t end = sysTime() + timeout;
    while (digitalRead(_drdy) == HIGH)
    {
        if (sysTime() >= end)
        {
            digitalWrite(_cs, HIGH);
            _spi->endTransaction();
            return false;
        }
    }

    data = _spi->transfer16(0xFFFF);
    digitalWrite(_cs, HIGH);
    _spi->endTransaction();
    return true;
}
