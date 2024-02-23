#include "priorityqueue.h"

//
//// MinPriorityQueue<T> Class
//

template <class T> bool MinPriorityQueue<T>::empty()
{
    return _pq.empty();
}

template <class T> unsigned MinPriorityQueue<T>::size()
{
    return _pq.size();
}

template <class T> void MinPriorityQueue<T>::push(const T& item, unsigned priority)
{
    _pq.push(priority);
    _data[priority].emplace(item);
}

template <class T> void MinPriorityQueue<T>::pop()
{
    unsigned priority = _pq.top();
    _pq.pop();
    std::queue<T>& q = _data[priority];
    q.pop();
    if (q.empty())
        _data.remove(priority);
}

template <class T> T& MinPriorityQueue<T>::top()
{
    return _data[_pq.top()].front();
}

template <class T> unsigned MinPriorityQueue<T>::topPriority()
{
    return _pq.top();
}
