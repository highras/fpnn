#ifndef HashMap_h_
#define HashMap_h_

#include "HashFunctor.h"
#include "obpool.h"
#include "bit.h"
#include <stdlib.h>
#include <assert.h>

namespace fpnn {

template<typename Key, typename Data, typename HashFun=HashFunctor<Key> >
class HashMap
{
	struct Node;
public:
	typedef Key key_type;
	typedef Data data_type;
	typedef Node node_type;
	typedef HashFun hash_func;

	HashMap(size_t slot_num);
	~HashMap();

	size_t count() const { return _total; }

	node_type* find(const Key& key);
	node_type* insert(const Key& key, const Data& dat);
	node_type* replace(const Key& key, const Data& dat);
	bool remove(const Key& key);
	void remove_node(node_type* the_node);
	node_type* next_node(node_type* the_node);

private:
	struct Node
	{
		Node(const Key& key_, const Data& data_): key(key_), data(data_)
		{}

		void *operator new(size_t size, obpool_t *pool)
		{
			assert(pool->obj_size == size);
			return obpool_acquire(pool);
		}
		void operator delete(void *p, obpool_t *pool)
		{
			obpool_release(pool, p);
		}

		friend class HashMap;

		Node* hash_next;
		unsigned int hash;
	public:
		Key key;
		Data data;
	};

	unsigned int _mask;
	Node** _tab;
	obpool_t _pool;
	size_t _total;
	HashFun _hash;
};


template<typename Key, typename Data, typename HashFun>
HashMap<Key,Data,HashFun>::HashMap(size_t slot_num)
{
	slot_num = round_up_power_two(slot_num < INT_MAX ? slot_num : INT_MAX);
	_mask = slot_num - 1;
	_tab = (Node **)calloc(slot_num, sizeof(Node*));
	_total = 0;
	obpool_init(&_pool, sizeof(Node));
}

template<typename Key, typename Data, typename HashFun>
HashMap<Key,Data,HashFun>::~HashMap()
{
	for (unsigned int h = 0; h <= _mask; ++h)
	{
		Node *node, *next;
		for (node = _tab[h]; node; node = next)
		{
			next = node->hash_next;
			node->Node::~Node();
		}
	}
	free(_tab);
	obpool_finish(&_pool);
}

template<typename Key, typename Data, typename HashFun>
typename HashMap<Key,Data,HashFun>::node_type*
HashMap<Key,Data,HashFun>::find(const Key& key)
{
	unsigned int hash = _hash(key);
	unsigned int bkt = hash & _mask;
	Node *node;
	for (node = _tab[bkt]; node; node = node->hash_next)
	{
		if (node->hash == hash && node->key == key)
		{
			return node;
		}
	}
	return NULL;
}

template<typename Key, typename Data, typename HashFun>
typename HashMap<Key,Data,HashFun>::node_type*
HashMap<Key,Data,HashFun>::insert(const Key& key, const Data& dat)
{
	unsigned int hash = _hash(key);
	unsigned int bkt = hash & _mask;
	Node *node;
	for (node = _tab[bkt]; node; node = node->hash_next)
	{
		if (node->hash == hash && node->key == key)
		{
			return NULL;
		}
	}

	node = new(&_pool) Node(key, dat);
	node->hash = hash;
	node->hash_next = _tab[bkt];
	_tab[bkt] = node;
	_total++;
	return node;
}

template<typename Key, typename Data, typename HashFun>
typename HashMap<Key,Data,HashFun>::node_type*
HashMap<Key,Data,HashFun>::replace(const Key& key, const Data& val)
{
	unsigned int hash = _hash(key);
	unsigned int bkt = hash & _mask;
	Node *node;
	for (node = _tab[bkt]; node; node = node->hash_next)
	{
		if (node->hash == hash && node->key == key)
		{
			node->data = val;
			return node;
		}
	}

	node = new(&_pool) Node(key, val);
	node->hash = hash;
	node->hash_next = _tab[bkt];
	_tab[bkt] = node;
	_total++;
	return node;
}

template<typename Key, typename Data, typename HashFun>
bool HashMap<Key,Data,HashFun>::remove(const Key& key)
{
	unsigned int hash = _hash(key);
	unsigned int bkt = hash & _mask;
	Node *node, *prev = NULL;
	for (node = _tab[bkt]; node; prev = node, node = node->hash_next)
	{
		if (node->hash == hash && node->key == key)
		{
			if (prev)
				prev->hash_next = node->hash_next;
			else
				_tab[bkt] = node->hash_next;
			_total--;
			node->Node::~Node();
			obpool_release(&_pool, node);
			return true;
		}
	}
	return false;
}

template<typename Key, typename Data, typename HashFun>
void HashMap<Key,Data,HashFun>::remove_node(node_type *the_node)
{
	unsigned int bkt = (the_node->hash & _mask);
	Node *node, *prev = NULL;
	for (node = _tab[bkt]; node; prev = node, node = node->hash_next)
	{
		if (node == the_node)
		{
			if (prev)
				prev->hash_next = node->hash_next;
			else
				_tab[bkt] = node->hash_next;
			node->Node::~Node();
			obpool_release(&_pool, node);
			_total--;
			return;
		}
	}
	assert(!"can't reach here");
}

template<typename Key, typename Data, typename HashFun>
typename HashMap<Key,Data,HashFun>::node_type*
HashMap<Key,Data,HashFun>::next_node(node_type *the_node)
{
	unsigned int bkt = 0;
	if (the_node)
	{
		if (the_node->hash_next)
			return the_node->hash_next;
		else
		{
			bkt = the_node->hash & _mask;
			++bkt;
			if (bkt > _mask)
				return NULL;
		}
	}

	while (bkt <= _mask)
	{
		if (_tab[bkt])
			return _tab[bkt];
		++bkt;
	}

	return NULL;
}

}

#endif
