#pragma once

#include "hash.h"
#include <queue>

template <class T> class PriorityQueue
{
    public:
        PriorityQueue() {}

        unsigned size() { return _items.size(); }
        void push(T& item, unsigned priority) {  _items[priority].push(item); }
        void pop();
        T& front() { return _items[_nextPriority()].front(); }

        unsigned _nextPriority();

    private:
        unsigned _priority = -1;
        Hash<std::queue<T>> _items;
};

#include "priorityqueue.hpp"
