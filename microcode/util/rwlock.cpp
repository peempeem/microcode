#include "rwlock.h"

//
//// ReadWriteLock Class
//

void ReadWriteLock::readLock()
{
    while (true)
    {
        _access.lock();
        if (!_currentWriterHandle)
        {
            _readers++;
            _access.unlock();
            break;
        }
        _readerHandles.push(xTaskGetCurrentTaskHandle(), uxTaskPriorityGet(NULL));
        _access.unlock();
        uint32_t notification;
        xTaskNotifyWait(0, UINT32_MAX, &notification, portMAX_DELAY);
    }
}

void ReadWriteLock::readUnlock()
{
    _access.lock();
    if (--_readers == 0 && _currentWriterHandle)
        xTaskNotify(_currentWriterHandle, 0, eNoAction);
    _access.unlock();
}

void ReadWriteLock::writeLock()
{
    _access.lock();
    if (!_currentWriterHandle)
        _currentWriterHandle = xTaskGetCurrentTaskHandle();
    else
    {
        _writerHandles.push(xTaskGetCurrentTaskHandle(), uxTaskPriorityGet(NULL));
        _access.unlock();
        uint32_t notification;
        xTaskNotifyWait(0, UINT32_MAX, &notification, portMAX_DELAY);
        _access.lock();
    }

    if (!_readers)
    {
        _access.unlock();
        return;
    }

    _access.unlock();
    uint32_t notification;
    xTaskNotifyWait(0, UINT32_MAX, &notification, portMAX_DELAY);
}

void ReadWriteLock::writeUnlock()
{
    _access.lock();
    if (!_writerHandles.empty())
    {
        _currentWriterHandle = _writerHandles.top().handle;
        _writerHandles.pop();
        xTaskNotify(_currentWriterHandle, 0, eNoAction);
    }
    else
    {
        _currentWriterHandle = NULL;

        while (!_readerHandles.empty())
        {
            xTaskNotify(_readerHandles.top().handle, 0, eNoAction);
            _readerHandles.pop();
        }
    }
    _access.unlock();
}

//
//// ReadWriteLock::TaskHandleWrapper Class
//

ReadWriteLock::TaskHandleWrapper::TaskHandleWrapper(TaskHandle_t handle) 
    : handle(handle)
{

}