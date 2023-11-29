#include "icon.h"
#include "../../hal/hal.h"
#include "../../util/filesys.h"

Icon::Icon()
{
    _path = NULL;
    _dims.set(0, 0);
}

Icon::Icon(const char* path, bool async, unsigned priority) : async(async), priority(priority)
{
    if (filesys.read(path, (uint8_t*) &_dims, sizeof(_dims)))
        _path = path;
    else
    {
        _path = NULL;
        _dims.set(0, 0);
    }
}

unsigned Icon::width()
{
    return _dims.width;
}

unsigned Icon::height()
{
    return _dims.height;
}

static void _allocate(void* ptr)
{
    Icon::IconAllocatorData* adata = (Icon::IconAllocatorData*) ptr;
    Icon::IconBufferData* bdata = (Icon::IconBufferData*) adata->buf.data();
    filesys.read(adata->path, bdata->data, adata->buf.size() - 1, sizeof(Dimension2D<unsigned>));
    bdata->allocated = true;
    delete adata;
    vTaskDelete(NULL);
}

bool Icon::allocate()
{
    if (!_path || _buf.size() > 0)
        return false;

    _buf = SharedBuffer(1 + _dims.area() * 2);
    ((IconBufferData*) _buf.data())->allocated = false;

    IconAllocatorData* data = new IconAllocatorData();
    data->path = _path;
    data->async = async;
    data->buf = _buf;
    
    if (async)
        xTaskCreate(_allocate, "ialloc", 2048, (void*) data, priority, NULL);
    else
        _allocate((void*) data);
    return true;
}

bool Icon::allocated()
{
    return _buf.size() > 0 && ((IconBufferData*) _buf.data())->allocated;
}

void Icon::deallocate()
{
    _buf = SharedBuffer();
}

uint16_t* Icon::bitmap()
{
    return (uint16_t*) ((IconBufferData*) _buf.data())->data;
}

uint16_t Icon::at(unsigned x, unsigned y)
{
    return bitmap()[x + y * _dims.width];
}