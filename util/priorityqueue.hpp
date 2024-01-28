#include "priorityqueue.h"

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
    _pq.emplace(item, priority);
}

template <class T> void MinPriorityQueue<T>::pop()
{
    _pq.pop();
}

template <class T> T& MinPriorityQueue<T>::top()
{
    return (T&) _pq.top().item;
}

template <class T> unsigned MinPriorityQueue<T>::topPriority()
{
    return _pq.top().priority;
}