#include "hash.h"
#include <cstddef>

const static unsigned primes[] = 
{
    11,         13,         17,         29,         53,         
    67,         79,         97,         131,        193,
    257,        389,        521,        769,        1031,
    1543,       2053,       3079,       6151,       12289,
    24593,      49157,      98317,      196613,     393241,
    786433,     1572869,    3145739,    6291469,    12582917,
    25165843,   50331653,   100663319,  201326611,  402653189,
    805306457,  1610612741, 3221225473, 4294967291
};

//
//// Hash<V>::Node Class
//

template <class V> Hash<V>::Node::Node()
{

}

template <class V> Hash<V>::Node::Node(unsigned key, const V& value) : key(key), value(value)
{

}

//
//// Hash<V>::Itterator Class
//

template <class V> Hash<V>::Itterator::Itterator(unsigned idx, std::vector<Node*>* table) : _idx(idx), _table(table) 
{

}

template <class V> V& Hash<V>::Itterator::operator*()
{
    return (*_table)[_idx]->value;
}

template <class V> typename Hash<V>::Itterator& Hash<V>::Itterator::operator++()
{
    while (_idx < _table->size() && !(*_table)[++_idx]);
    return *this;
}

template <class V> typename Hash<V>::Itterator Hash<V>::Itterator::operator++(int)
{
    while (_idx < _table->size() && !(*_table)[++_idx]);
    return Itterator(_idx, _table);
}

template <class V> bool Hash<V>::Itterator::operator!=(const Itterator& other) const
{
    return _idx != other._idx;
}

template <class V> unsigned Hash<V>::Itterator::key() const
{
    return (*_table)[_idx]->key; 
}

template <class V> unsigned Hash<V>::Itterator::idx() const
{
    return _idx; 
}

//
//// Hash<V> Class
//

template <class V> Hash<V>::Hash(unsigned minSize, float minLoad, float maxLoad) : _size(0)
{
    _minLoad = (minLoad < 0 || minLoad > 0.5f) ? 0.25f : minLoad;
    _maxLoad = (maxLoad < _minLoad || maxLoad > 1) ? 0.75f : maxLoad;
    
    if (minSize != -1)
    {
        _prime = 1;
        while (primes[_prime] * _maxLoad < minSize)
            _prime += 2;
        _minPrime = _prime;
    }
    else
    {
        _minPrime = 1;
        _prime = _minPrime;
    }
    
    _table = new std::vector<Node*>(primes[_prime], NULL);
    _probe = new std::vector<bool>(primes[_prime], false);
}

template <class V> Hash<V>::Hash(const Hash<V>& other)
{
    _copy(other);
}

template <class V> Hash<V>::~Hash()
{
    _destroy();
}

template <class V> void Hash<V>::operator=(const Hash& other)
{
    _destroy();
    _copy(other);
}

template <class V> V& Hash<V>::operator[](unsigned key)
{
    unsigned idx = _contains(key);
    if (idx != (unsigned) -1)
        return (*_table)[idx]->value;    
    return _insert(key, V());
}

template <class V> V* Hash<V>::at(unsigned key)
{
    unsigned idx = _contains(key);
    if (idx != (unsigned) -1)
        return &(*_table)[idx]->value;
    return NULL;
}

template <class V> void Hash<V>::remove(unsigned key, bool canResize)
{
    unsigned idx = _contains(key);
    if (idx != (unsigned) -1)
        _remove(idx, canResize);
}

template <class V> void Hash<V>::remove(const Itterator& it, bool canResize)
{
    _remove(it.idx(), canResize);
}

template <class V> void Hash<V>::reserve(unsigned size)
{
    if (size < _size)
        return;
    
    unsigned prime = _prime;
    while (primes[prime] * _maxLoad < size)
        prime += 2;
    _minPrime = prime;
    _realloc(prime);
}

template <class V> void Hash<V>::shrink()
{
    unsigned newPrime = _prime;
    while (newPrime > 2 && newPrime > _minPrime && _size < primes[newPrime] * _minLoad)
        newPrime -= 2;
    _realloc(newPrime);
}

template <class V> void Hash<V>::clear()
{
    for (unsigned i = 0; i < _table->size(); i++)
    {
        if ((*_table)[i])
            delete (*_table)[i];
    }

    _size = 0;
    _prime = _minPrime;
    *_table = std::vector<Node*>(primes[_prime], NULL);
    *_probe = std::vector<bool>(primes[_prime], false);
}

template <class V> bool Hash<V>::contains(unsigned key)
{
    return _contains(key) != (unsigned) -1;
}

template <class V> bool Hash<V>::empty()
{
    return _size == 0;
}

template <class V> unsigned Hash<V>::size()
{
    return _size;
}

template <class V> unsigned Hash<V>::containerSize()
{
    return _table->size();
}

template <class V> typename Hash<V>::Itterator Hash<V>::begin()
{
    for (unsigned i = 0; i < _table->size(); i++)
    {
        if ((*_table)[i])
            return Itterator(i, _table);
    }
    return end();
}

template <class V> typename Hash<V>::Itterator Hash<V>::end()
{
    return Itterator(_table->size(), _table);
}

template <class V> V& Hash<V>::_insert(unsigned key, const V& value)
{
    if (_size + 1 > _table->size() * _maxLoad)
        _realloc(_prime + 2);
    
    unsigned start = _hash1(key);
    unsigned h2 = _hash2(key);
    unsigned idx = start;
    do
    {
        if (!(*_table)[idx])
        {
            (*_table)[idx] = new Node(key, value);
            _size++;
            return (*_table)[idx]->value;
        }
        (*_probe)[idx] = true;
        idx = (idx + h2) % _table->size();
    } 
    while (start != idx);

    _realloc(_prime + 2);
    return _insert(key, value);
}

template <class V> void Hash<V>::_remove(unsigned idx, bool canResize)
{
    delete (*_table)[idx];
    (*_table)[idx] = NULL;
    _size--;

    if (canResize && _size < _table->size() * _minLoad)
    {
        unsigned newPrime = (_prime > 2 && _prime > _minPrime) ? _prime - 2 : _minPrime;
        _realloc(newPrime);
    }
}

template <class V> void Hash<V>::_realloc(unsigned newPrime)
{
    if (_prime == newPrime)
        return;
    
    _prime = newPrime;
    
    std::vector<Node*>* table = new std::vector<Node*>(primes[_prime], NULL);
    std::vector<bool>* probe = new std::vector<bool>(primes[_prime], false);

    for (Node* node : *_table)
    {
        if (node)
        {
            unsigned start = _hash1(node->key);
            unsigned h2 = _hash2(node->key);
            unsigned idx = start;
            do
            {
                if (!(*table)[idx])
                {
                    (*table)[idx] = node;
                    break;
                }
                (*probe)[idx] = true;
                idx = (idx + h2) % table->size();
            } 
            while (start != idx);
        }
    }
    
    delete _table;
    delete _probe;

    _table = table;
    _probe = probe;
}

template <class V> unsigned Hash<V>::_contains(unsigned key)
{
    unsigned start = _hash1(key);
    unsigned h2 = _hash2(key);
    unsigned idx = start;
    do
    {   
        if ((*_table)[idx] && (*_table)[idx]->key == key)
            return idx;
        if (!(*_probe)[idx])
            break;
        idx = (idx + h2) % _table->size();
    } 
    while (start != idx);
    return (unsigned) -1;
}

template <class V> unsigned Hash<V>::_hash1(unsigned key)
{
    return key % primes[_prime];
}

template <class V> unsigned Hash<V>::_hash2(unsigned key)
{
    return primes[_prime - 1] - (key % primes[_prime - 1]);
}

template <class V> void Hash<V>::_destroy()
{
    for (unsigned i = 0; i < _table->size(); i++)
    {
        if ((*_table)[i])
            delete (*_table)[i];
    }

    delete _table;
    delete _probe;
}

template <class V> void Hash<V>::_copy(const Hash& other)
{
    _minLoad = other._minLoad;
    _maxLoad = other._maxLoad;
    _size = other._size;
    _minPrime = other._minPrime;
    _prime = other._prime;
    _table = new std::vector<Node*>(primes[_prime]);
    _probe = new std::vector<bool>(*other._probe);
    
    for (unsigned i = 0; i < other._table->size(); i++)
    {
        if ((*other._table)[i])
            (*_table)[i] = new Node((*other._table)[i]->key, (*other._table)[i]->value);
        else
            (*_table)[i] = NULL;
    }
}