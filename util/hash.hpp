#include "hash.h"
#include <cstddef>

const unsigned primes[] = 
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

template <class V> Hash<V>::Node::Node()
{

}

template <class V> Hash<V>::Node::Node(unsigned key, const V& value) : key(key), value(value)
{

}

template <class V> Hash<V>::Itterator::Itterator(unsigned idx, std::vector<Node*>* table) : _idx(idx), _table(table) 
{

}

template <class V> V& Hash<V>::Itterator::operator*()
{
    return (*_table)[_idx]->value;
}


template <class V> typename Hash<V>::Itterator& Hash<V>::Itterator::operator++()
{
    while (_idx < _table->size())
    {
        _idx++;
        if ((*_table)[_idx])
            break;
    }
    return *this;
}

template <class V> typename Hash<V>::Itterator Hash<V>::Itterator::operator++(int)
{
    unsigned idx = _idx;
    while (idx < _table.size())
    {
        idx++;
        if ((*_table)[idx])
            break;
    }
    return Itterator(idx, _table);
}

template <class V> bool Hash<V>::Itterator::operator!=(const Itterator& other)
{
    return _idx != other._idx;
}

template <class V> Hash<V>::Hash(float load) : _size(0), _prime(1)
{
    _load = (load < 0.5f || load > 1) ? 0.7f : load;
    _table = std::vector<Node*>(primes[_prime], NULL);
    _probe = std::vector<bool>(primes[_prime], false);
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

template <class V> void Hash<V>::insert(unsigned key, const V& value)
{
    unsigned idx = _contains(key);
    if (idx != (unsigned) -1)
        _table[idx]->value = value;
    else
        _insert(key, value);
}

template <class V> void Hash<V>::remove(unsigned key)
{
    unsigned idx = _contains(key);
    if (idx != (unsigned) -1)
    {
        delete _table[idx];
        _table[idx] = NULL;
        _size--;
    }
}

template <class V> void Hash<V>::clear()
{
    _size = 0;
    for (unsigned i = 0; i < _table.size(); i++)
    {
        if (_table[i])
        {

            delete _table[i];
            _table[i] = NULL;
        }
        _probe[i] = false;
    }
}

template <class V> V& Hash<V>::operator[](unsigned key)
{
    unsigned idx = _contains(key);
    if (idx != (unsigned) -1)
        return _table[idx]->value;    
    return _insert(key, V());
}


template <class V> V* Hash<V>::at(unsigned key)
{
    unsigned idx = _contains(key);
    if (idx != (unsigned) -1)
        return &_table[idx]->value;
    return NULL;
}


template <class V> bool Hash<V>::contains(unsigned key)
{
    return _contains(key) != (unsigned) -1;
}

template <class V> unsigned Hash<V>::size()
{
    return _size;
}

template <class V> bool Hash<V>::empty()
{
    return _size == 0;
}

template <class V> std::vector<unsigned> Hash<V>::keys()
{
    std::vector<unsigned> hashKeys;
    hashKeys.reserve(_size);
    for (unsigned i = 0; i < _table.size(); i++)
    {
        if (_table[i])
            hashKeys.push_back(_table[i]->key);
    }
    return hashKeys;
}

template <class V> typename Hash<V>::Itterator Hash<V>::begin()
{
    for (unsigned i = 0; i < _table.size(); i++)
    {
        if (_table[i])
            return Itterator(i, &_table);
    }
    return end();
}

template <class V> typename Hash<V>::Itterator Hash<V>::end()
{
    return Itterator(_table.size(), &_table);
}

template <class V> V& Hash<V>::_insert(unsigned key, const V& value)
{
    if (_size + 1 > _table.size() * _load)
        _resize();
    
    unsigned start = _hash1(key);
    unsigned h2 = _hash2(key);
    unsigned idx = start;
    do
    {
        if (!_table[idx])
        {
            _table[idx] = new Node(key, value);
            _size++;
            return _table[idx]->value;
        }
        _probe[idx] = true;
        idx = (idx + h2) % _table.size();
    } 
    while (start != idx);

    _resize();
    return _insert(key, value);
}

template <class V> void Hash<V>::_resize()
{
    _prime++;
    std::vector<Node*> table = std::vector<Node*>(primes[_prime], NULL);
    std::vector<bool> probe = std::vector<bool>(primes[_prime], false);
    for (unsigned i = 0; i < primes[_prime]; i++)
        probe[i] = false;
    for (Node* node : _table)
    {
        if (node)
        {
            unsigned start = _hash1(node->key);
            unsigned h2 = _hash2(node->key);
            unsigned idx = start;
            do
            {
                if (!table[idx])
                {
                    table[idx] = node;
                    break;
                }
                probe[idx] = true;
                idx = (idx + h2) % table.size();
            } 
            while (start != idx);
        }
    }

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
        if (_table[idx] && _table[idx]->key == key)
            return idx;
        if (!_probe[idx])
            break;
        idx = (idx + h2) % _table.size();
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
    for (unsigned i = 0; i < _table.size(); i++)
    {
        if (_table[i])
            delete _table[i];
    }
}

template <class V> void Hash<V>::_copy(const Hash& other)
{
    _load = other._load;
    _size = other._size;
    _prime = other._prime;
    _table = std::vector<Node*>(primes[_prime], NULL);
    _probe = other._probe;
    
    for (unsigned i = 0; i < other._table.size(); i++)
    {
        if (other._table[i])
            _table[i] = new Node(other._table[i]->key, other._table[i]->value);
    }
}