#pragma once

#include "hash.h"
#include <queue>

template <class T> class MinPriorityQueue
{
    public:
        bool empty();
        unsigned size();

        void push(const T& item, unsigned priority);
        void pop();

        T& top();
        unsigned topPriority();

    private:
        std::priority_queue<unsigned, std::vector<unsigned>, std::greater<unsigned>> _pq;
        Hash<std::queue<T>> _data;
};

#include "priorityqueue.hpp"
