#pragma once

#include "mutex.h"
#include "priorityqueue.h"

class ReadWriteLock
{
    public:
        void readLock();
        void readUnlock();

        void writeLock();
        void writeUnlock();

    private:
        struct TaskHandleWrapper
        {
            TaskHandle_t handle;

            TaskHandleWrapper(TaskHandle_t handle);
        };

        Mutex _access;
        unsigned _readers = 0;
        TaskHandle_t _currentWriterHandle = NULL;
        StaticPriorityQueue<TaskHandleWrapper, std::less> _readerHandles;
        StaticPriorityQueue<TaskHandleWrapper, std::less> _writerHandles;
};
