#include "mac.h"
#include <string.h>
#include <sstream>

MAC::MAC()
{

}

MAC::MAC(const uint8_t* addr)
{
    set(addr);
}

MAC::MAC(const MAC& mac)
{
    set(mac);
}

void MAC::operator=(const MAC& other)
{
    set(other);
}

bool MAC::operator==(const MAC& other)
{
    return is(other);
}

void MAC::set(const uint8_t* addr)
{
    memcpy(this->addr, addr, sizeof(MAC));
}

void MAC::set(const MAC& mac)
{
    memcpy(addr, mac.addr, sizeof(MAC));
}

bool MAC::is(const uint8_t* addr)
{
    return memcmp(this->addr, addr, sizeof(MAC)) == 0;
}

bool MAC::is(const MAC& mac)
{
    return memcmp(addr, mac.addr, sizeof(MAC)) == 0;
}

std::string MAC::toString()
{
    std::stringstream ss;
    for (unsigned i = 0; i < sizeof(MAC); i++)
    {
        ss << (int) addr[i];
        if (i != sizeof(MAC) - 1)
            ss << ", ";
    }
    return ss.str();
}
