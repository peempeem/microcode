#include "priorityqueue.h"

//
//// MinPriorityQueue<T> Class
//

template <class T> MinPriorityQueue<T>::~MinPriorityQueue()
{
    while (!empty())
        pop();
}

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
    _pq.emplace(new T(item), priority);
}

template <class T> void MinPriorityQueue<T>::pop()
{
    delete _pq.top().item;
    _pq.pop();
}

template <class T> T& MinPriorityQueue<T>::top()
{
    return (T&) *_pq.top().item;
}

template <class T> unsigned MinPriorityQueue<T>::topPriority()
{
    return _pq.top().priority;
}

//
//// MinPriorityQueue<T>::Item Class
//

template <class T> MinPriorityQueue<T>::Item::Item()
{

}

template <class T> MinPriorityQueue<T>::Item::Item(T* item, unsigned priority) : item(item), priority(priority)
{

}

template <class T> bool MinPriorityQueue<T>::Item::operator>(const Item& other) const
{
    return priority > other.priority;
}