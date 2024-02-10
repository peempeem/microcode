#pragma once

#include "hash.h"
#include <queue>

template <class T> class MinPriorityQueue
{
    public:
        ~MinPriorityQueue();

        bool empty();
        unsigned size();

        void push(const T& item, unsigned priority);
        void pop();

        T& top();
        unsigned topPriority();

    private:
        struct Item
        {
            unsigned priority;
            T* item;
                        
            Item();
            Item(T* item, unsigned priority);

            bool operator>(const Item& other) const;
        };

        std::priority_queue<Item, std::vector<Item>, std::greater<Item>> _pq;
};

#include "priorityqueue.hpp"
