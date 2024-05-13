#pragma once

#include <stdint.h>
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
                V* operator->();
                Itterator& operator++();
                Itterator operator++(int);
                bool operator!=(const Itterator& other) const;
                unsigned key() const;
                unsigned idx() const;
            
            private:
                unsigned _idx;
                std::vector<Node*>* _table;
        };

        Hash(unsigned minSize=-1, unsigned minLoad=25, unsigned maxLoad=75);
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

        std::vector<unsigned> keys();

        Itterator begin();
        Itterator end();

    private:
        std::vector<Node*>* _table;
        std::vector<bool>* _probe;
        unsigned _minLoad;
        unsigned _maxLoad;
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

template <class V> class StaticHash
{
    public:
        struct Node
        {
            unsigned key;
            V value;

            Node();
            Node(unsigned key, const V& value);
        };
        
        struct Probe
        {
            uint8_t probe   : 1;
            uint8_t used    : 1;

            Probe(uint8_t probe=0, uint8_t used=0) : probe(probe), used(used) {}
        };

        class Itterator
        {
            public:
                Itterator(unsigned idx, std::vector<Node>* table, std::vector<Probe>* probe);

                V& operator*();
                Itterator& operator++();
                Itterator operator++(int);
                bool operator!=(const Itterator& other) const;
                unsigned key() const;
                unsigned idx() const;
            
            private:
                unsigned _idx;
                std::vector<Node>* _table;
                std::vector<Probe>* _probe;
        };

        StaticHash(unsigned minSize=-1, float minLoad=0.25f, float maxLoad=0.75f);
        StaticHash(const StaticHash& other);
        ~StaticHash();

        void operator=(const StaticHash& other);

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
        std::vector<Node>* _table;
        std::vector<Probe>* _probe;
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
        void _copy(const StaticHash& other);
};

#include "hash.hpp"
