#include "src/microcode/util/rwlock.h"
#include "src/microcode/util/log.h"
#include "src/microcode/hardware/suart.h"

ReadWriteLock rw;
SpinLock lock;
unsigned writes = 0;
unsigned reads = 0;

void writeTask(void* args)
{
    uint32_t notification;

    while (true)
    {
        xTaskNotifyWait(0, UINT32_MAX, &notification, 1 / portTICK_PERIOD_MS);
        rw.writeLock();
        writes++;
        rw.writeUnlock();
    }
}

void readTask(void* args)
{
    uint32_t notification;

    while (true)
    {
        xTaskNotifyWait(0, UINT32_MAX, &notification, 1 / portTICK_PERIOD_MS);
        rw.readLock();
        lock.lock();
        reads++;
        lock.unlock();
        rw.readUnlock();
    }
}

void setup()
{
    delay(3000);
    suart0.init(115200);
    Log() >> *suart0.getTXStream();

    for (unsigned i = 0; i < 10; ++i)
    {
        xTaskCreateUniversal(
            readTask, 
            "readTask", 
            1024 * 3, 
            NULL,
            5 + i, 
            NULL, 
            tskNO_AFFINITY);
    }

    for (unsigned i = 0; i < 4; ++i)
    {
        xTaskCreateUniversal(
            writeTask, 
            "writeTask", 
            1024 * 3, 
            NULL,
            5 + i, 
            NULL, 
            tskNO_AFFINITY);
    }
}

void loop()
{
    Log("counts") << "reads: " << reads << " writes: " << writes;
    delay(100);
}
