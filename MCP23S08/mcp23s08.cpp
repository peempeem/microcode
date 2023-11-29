#include "mcp23s08.h"

#if __has_include(<SPI.h>)

MCP23S08::MCP23S08(SPIClass* spi, unsigned cs, unsigned rst, unsigned clkspeed, unsigned addr) : _spi(spi), _cs(cs), _rst(rst), _clkspeed(clkspeed)
{
    _opCode = 0x40 | ((addr & 0x03) << 1);
}

bool MCP23S08::begin()
{
    sysPinMode(_rst, OUTPUT);
    sysPinMode(_cs, OUTPUT);

    sysDigitalWrite(_rst, LOW);
    vTaskDelay(1);
    digitalWrite(_rst, HIGH);

    _writeReg(IODIR, 0xFF);
    for (uint8_t i = 1; i < OLAT; i++)
        _writeReg(i, 0x00);

    return _readReg(IODIR) == 0xFF;
}

void MCP23S08::setMode(unsigned pin, unsigned mode)
{
    switch (mode)
    {
        case INPUT:
            _writeReg(IODIR, ~(~_readReg(IODIR) & ~(1 << pin)));
            _writeReg(GPPU, _readReg(GPPU) & ~(1 << pin));
            break;
        
        case OUTPUT:
            _writeReg(IODIR, ~(~_readReg(IODIR) | (1 << pin)));
            _writeReg(GPPU, _readReg(GPPU) & ~(1 << pin));
            break;
    }
}

void MCP23S08::write(unsigned pin, bool state)
{
    _writeReg(OLAT, (_readReg(OLAT) & ~(1 << pin)) | (state << pin));
}

void MCP23S08::write8(uint8_t state)
{
    _writeReg(OLAT, state);
}

bool MCP23S08::read(unsigned pin)
{
    return (_readReg(GPIO) >> pin) & 1;
}

uint8_t MCP23S08::read8()
{
    return _readReg(GPIO);
}

void MCP23S08::_writeReg(uint8_t addr, uint8_t data)
{
    _spi->beginTransaction(SPISettings(_clkspeed, MSBFIRST, SPI_MODE0));
    sysDigitalWrite(_cs, LOW);
    _spi->transfer(_opCode);
    _spi->transfer(addr);
    _spi->transfer(data);
    sysDigitalWrite(_cs, HIGH);
    _spi->endTransaction();
}

uint8_t MCP23S08::_readReg(uint8_t addr)
{
    _spi->beginTransaction(SPISettings(_clkspeed, MSBFIRST, SPI_MODE0));
    sysDigitalWrite(_cs, LOW);
    _spi->transfer(_opCode | 1);
    _spi->transfer(addr);
    uint8_t data = _spi->transfer(0);
    sysDigitalWrite(_cs, HIGH);
    _spi->endTransaction();
    return data;
}

#endif
