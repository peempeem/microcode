#pragma once

#include "microcode/hal/hal.h"
#include "microcode/util/rate.h"
#include "microcode/util/timer.h"
#include "microcode/ota/ota.h"
#include "microcode/grid/hub.h"
#include "microcode/util/eventScheduler.h"
#include "defines.h"
#include "types.h"

#include <Adafruit_NeoPixel.h>

void runPixels(void* args)
{
    Adafruit_NeoPixel pixels(NUM_PXL, PXL_PIN, NEO_GRB + NEO_KHZ800);
    uint32_t notification = 0;

    struct Light
    {
        Rate rate;
        uint8_t max;
        float offset;
    } r, g, b;

    enum State
    {
        START,
        IDLE,
        UPDATING,
        UPDATING_CHECK,
        REBOOTING1,
        REBOOTING2
    } state;

    EventScheduler sched;

    while (true)
    {
        xTaskNotifyWait(0, UINT32_MAX, &notification, PIXEL_REFERESH_PERIOD / portTICK_PERIOD_MS);
        
        if (sched.isEmpty())
        {
            if (ota.isUpdating())
            {
                if (state != UPDATING)
                    sched.schedule(UPDATING, 0);
            }
            else if (state != IDLE)
                sched.schedule(IDLE, 0);
        }
        
        sched.update();
        
        while (!sched.currentEvents.empty())
        {
           switch (sched.currentEvents.front())
           {
                case IDLE:
                {
                    r.rate.setHertz(1);
                    g.rate.setHertz(2);
                    b.rate.setHertz(3);
                    r.max = 255;
                    g.max = 255;
                    b.max = 255;
                    r.offset = 0.1f;
                    g.offset = 0.1f;
                    b.offset = 0.1f;
                    state = IDLE;
                    break;
                }

                case UPDATING:
                {
                    r.rate.setHertz(1);
                    g.rate.setHertz(1);
                    b.rate.setHertz(1);
                    r.max = 128;
                    g.max = 48;
                    b.max = 3;
                    r.offset = 0;
                    g.offset = 0;
                    b.offset = 0;
                    state = UPDATING;
                    sched.schedule(UPDATING_CHECK, 100);
                    break;
                }

                case UPDATING_CHECK:
                {
                    if (ota.isRebooting())
                        sched.schedule(REBOOTING1, 0);
                    else if (ota.isUpdating())
                        sched.schedule(UPDATING_CHECK, 50);
                    state = UPDATING_CHECK;
                    break;
                }

                case REBOOTING1:
                {
                    r.rate.setHertz(4);
                    g.rate.setHertz(4);
                    b.rate.setHertz(4);
                    r.max = 65;
                    g.max = 139;
                    b.max = 255;
                    r.offset = 1 / (float) NUM_PXL;
                    g.offset = 1 / (float) NUM_PXL;
                    b.offset = 1 / (float) NUM_PXL;
                    sched.schedule(REBOOTING2, 750);
                    state = REBOOTING1;
                    break;
                }

                case REBOOTING2:
                {
                    r.max = 0;
                    g.max = 0;
                    b.max = 0;
                    state = REBOOTING2;
                    break;
                }
           }
           sched.currentEvents.pop();
        }

        for (unsigned i = 0; i < NUM_PXL; i++)
        {
            pixels.setPixelColor(i, 
                pixels.Color(
                    r.rate.getStageCos(r.offset * i) * r.max, 
                    g.rate.getStageCos(b.offset * i) * g.max, 
                    b.rate.getStageCos(g.offset * i) * b.max));
        }

        pixels.show();

        if (notification & BIT(0) || state == REBOOTING2)
            vTaskDelete(NULL);
    }
}

struct TransferPacketArgs
{
    GridMessageHub* hub;
    unsigned port;
    SUART* suart;

    TransferPacketArgs(GridMessageHub* hub, unsigned port, SUART* suart)
        : hub(hub), port(port), suart(suart) {}
};

void transferPackets(void* args)
{
    TransferPacketArgs* tpa = (TransferPacketArgs*) args;
    uint32_t notification = 0;

    while (true)
    {
        xTaskNotifyWait(0, UINT32_MAX, &notification, 1 / portTICK_PERIOD_MS);
        
        if (tpa->suart->read())
        {
            ByteStream* stream = tpa->suart->getRXStream();

            uint8_t buf[128];
            unsigned pulled;
            do
            {
                unsigned pulled = stream->get(buf, sizeof(buf));
                for (unsigned i = 0; i < pulled; ++i)
                    (*tpa->hub)[tpa->port].in.insert(buf[i]);
            } while (pulled);
        }

        (*tpa->hub)[tpa->port].out.lock();
        while (!(*tpa->hub)[tpa->port].out.empty())
        {
            GridPacket& pkt = (*tpa->hub)[tpa->port].out.top();
            tpa->suart->write(pkt.raw(), pkt.size());
            (*tpa->hub)[tpa->port].out.pop();
        }
        (*tpa->hub)[tpa->port].out.unlock();

        if (notification & BIT(0))
        {
            delete args;
            vTaskDelete(NULL);
        }
    }
}

void updateHub(void* args)
{
    GridMessageHub* hub = (GridMessageHub*) args;
    uint32_t notification = 0;

    while (true)
    {
        xTaskNotifyWait(0, UINT32_MAX, &notification, 1 / portTICK_PERIOD_MS);
        
        hub->update();

        if (notification & BIT(0))
            vTaskDelete(NULL);
    }
}