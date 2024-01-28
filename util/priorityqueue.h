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
        struct Item
        {
            Item() : item(NULL) { }
            Item(T item, unsigned priority) : item(item), priority(priority) { }

            bool operator>(const Item& other) const { return priority > other.priority; }

            T item;
            unsigned priority;
        };

        std::priority_queue<Item, std::vector<Item>, std::greater<Item>> _pq;
};

#include "priorityqueue.hpp"
