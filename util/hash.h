#pragma once

#include <vector>
   
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
                bool operator!=(const Itterator& other) const;
                unsigned key() const;
                unsigned idx() const;
            
            private:
                unsigned _idx;
                std::vector<Node*>* _table;
        };

        Hash(unsigned minSize=-1, float minLoad=0.25f, float maxLoad=0.75f);
        Hash(const Hash& other);
        ~Hash();

        void operator=(const Hash& other);

        V& operator[](unsigned key);
        V* at(unsigned key);

        void remove(unsigned key, bool canResize=true);
        void remove(const Itterator& it, bool canResize=false);
        void reserve(unsigned size);
        void shrink();
        void clear();

        bool contains(unsigned key);
        bool empty();
        unsigned size();
        unsigned containerSize();

        Itterator begin();
        Itterator end();

    private:
        std::vector<Node*>* _table;
        std::vector<bool>* _probe;
        float _minLoad;
        float _maxLoad;
        unsigned _size;
        unsigned _minPrime;
        unsigned _prime;

        V& _insert(unsigned key, const V& value);
        void _remove(unsigned idx, bool canResize=true);
        void _realloc(unsigned newPrime);
        unsigned _contains(unsigned key);
        unsigned _hash1(unsigned key);
        unsigned _hash2(unsigned key);

        void _destroy();
        void _copy(const Hash& other);
};

#include "hash.hpp"
