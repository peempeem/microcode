#pragma once

#include <vector>
#include <list>
   
template <class V> class Hash
{
    public:
        struct Node
        {
            unsigned key;
            V value;

            Node();
            Node(unsigned key, const V& value);
        };

        class Itterator
        {
            public:
                Itterator(unsigned idx, std::vector<Node*>* table);

                V& operator*();
                Itterator& operator++();
                Itterator operator++(int);
                bool operator!=(const Itterator& other);
            
            private:
                unsigned _idx;
                std::vector<Node*>* _table;
        };

        Hash(float resize=0.7f);
        Hash(const Hash& other);
        ~Hash();

        void operator=(const Hash& other);

        void insert(unsigned key, const V& value);
        void remove(unsigned key);

        void clear();

        V& operator[](unsigned key);
        V* at(unsigned key);

        bool contains(unsigned key);
        unsigned size();
        bool empty();

        std::vector<unsigned> keys();

        Itterator begin();
        Itterator end();

    private:
        std::vector<Node*> _table;
        std::vector<bool> _probe;
        unsigned _size;
        float _load;
        unsigned _prime;

        V& _insert(unsigned key, const V& value);
        void _resize();
        unsigned _contains(unsigned key);
        unsigned _hash1(unsigned key);
        unsigned _hash2(unsigned key);

        void _destroy();
        void _copy(const Hash& other);
};

#include "hash.hpp"
