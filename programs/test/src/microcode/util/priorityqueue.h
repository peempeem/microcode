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

template<class T, template<class> class C> class StaticPriorityQueue
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
            unsigned priority;
            T item;
                        
            Item();
            Item(const T& item, unsigned priority);

            bool operator>(const Item& other) const;
            bool operator<(const Item& other) const;
        };

        std::priority_queue<Item, std::vector<Item>, C<Item>> _pq;
};

#include "priorityqueue.hpp"
