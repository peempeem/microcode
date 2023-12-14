#include "priorityqueue.h"
#include <algorithm>

template <class T> void PriorityQueue<T>::pop()
{
    unsigned priority = _nextPriority();
    _items[priority].pop();
    if (_items[priority].empty())
        _items.remove(priority);
    
    _priority = -1;
}

template <class T> unsigned PriorityQueue<T>::_nextPriority()
{
    if (_priority != (unsigned) -1)
        return _priority;
    
    std::vector<unsigned> priorities = _items.keys();
    _priority = *std::min_element(priorities.begin(), priorities.end());
    return _priority;
}
