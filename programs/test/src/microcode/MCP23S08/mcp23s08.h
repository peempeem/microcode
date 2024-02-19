#pragma once

#include "../hal/hal.h"

#if __has_include(<SPI.h>)

#include <SPI.h>

class MCP23S08
{
    public:
        MCP23S08(SPIClass* spi, unsigned cs, unsigned rst=-1, unsigned clkspeed=1e6, unsigned addr=0);

        bool begin();

        void setMode(unsigned pin, unsigned mode);

        void write(unsigned pin, bool state);
        void write8(uint8_t state);
        bool read(unsigned pin);
        uint8_t read8();

        
    
    private:
        enum Registers
        {
            IODIR,
            IPOL,
            GPINTEN,
            DEFVAL,
            INTCON,
            IOCON,
            GPPU,
            INTF,
            INTCAP,
            GPIO,
            OLAT
        };

        SPIClass* _spi;
        unsigned _cs;
        unsigned _rst;
        unsigned _clkspeed;
        uint8_t _opCode;

        void _writeReg(uint8_t addr, uint8_t data);
        uint8_t _readReg(uint8_t addr);
};

#endif
