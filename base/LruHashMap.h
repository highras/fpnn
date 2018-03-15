#ifndef LruHashMap_h_
#define LruHashMap_h_

#include "HashFunctor.h"
#include "obpool.h"
#include "bit.h"
#include "queue.h"
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

namespace fpnn {

template<typename Key, typename Data, typename HashFun=HashFunctor<Key> >
class LruHashMap
{
	struct Node;
public:
	typedef Key key_type;
	typedef Data data_type;
	typedef Node node_type;
	typedef HashFun hash_func;

	LruHashMap(size_t slot_num, size_t max_size = 0);
	~LruHashMap();

	size_t count() const { return _total; }

	node_type* find(const Key& key);
	node_type* use(const Key& key);		// find and adjust LRU list
	node_type* insert(const Key& key, const Data& val);
	node_type* replace(const Key& key, const Data& val);
	bool remove(const Key& key);
	void remove_node(node_type* node);
	void fresh_node(node_type* node);
	void stale_node(node_type* node);
	size_t drain(size_t num);

	node_type *most_fresh() const;
	node_type *most_stale() const;
	node_type *next_fresh(const node_type *node) const;
	node_type *next_stale(const node_type *node) const;

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

		friend class LruHashMap;

		Node *hash_next;
		TAILQ_ENTRY(Node) lru_link;
		uint32_t hash;
	public:
		Key key;
		Data data;
	};

	TAILQ_HEAD(lrulist_t, Node);
	enum { BKT_MIN_NUM = 1024 };

	uint32_t _mask;
	Node **_tab;
	HashFun _hash;
	struct lrulist_t _lru;
	size_t _total;
	size_t _max_size;
	obpool_t _pool;
};


template<typename Key, typename Data, typename HashFun>
LruHashMap<Key,Data,HashFun>::LruHashMap(size_t slot_num, size_t max_size)
{
	if (max_size == 0)
	{
		_max_size = slot_num;
	}
	else
	{
		_max_size = max_size;
		if (slot_num > max_size)
			slot_num = max_size;
	}

	if (slot_num > INT_MAX)
		slot_num = INT_MAX;

	_total = 0;
	uint32_t bkt_num = round_up_power_two(slot_num);
	if (bkt_num < BKT_MIN_NUM)
		bkt_num = BKT_MIN_NUM;
	_tab = (Node **)calloc(bkt_num, sizeof(Node*));
	_mask = bkt_num - 1;
	obpool_init(&_pool, sizeof(Node));
	TAILQ_INIT(&_lru);
}

template<typename Key, typename Data, typename HashFun>
LruHashMap<Key,Data,HashFun>::~LruHashMap()
{
	Node *node;
	TAILQ_FOREACH(node, &_lru, lru_link)
	{
		node->Node::~Node();
	}
	obpool_finish(&_pool);
	free(_tab);
}

template<typename Key, typename Data, typename HashFun>
typename LruHashMap<Key,Data,HashFun>::node_type* 
LruHashMap<Key,Data,HashFun>::find(const Key& key)
{
	uint32_t hash = _hash(key);
	uint32_t bkt = hash & _mask;
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
typename LruHashMap<Key,Data,HashFun>::node_type* 
LruHashMap<Key,Data,HashFun>::use(const Key& key)
{
	uint32_t hash = _hash(key);
	uint32_t bkt = hash & _mask;
	Node *node;
	for (node = _tab[bkt]; node; node = node->hash_next)
	{
		if (node->hash == hash && node->key == key)
		{
			if (node != TAILQ_LAST(&_lru, lrulist_t))
			{
				TAILQ_REMOVE(&_lru, node, lru_link);
				TAILQ_INSERT_TAIL(&_lru, node, lru_link);
			}
			return node;
		}
	}
	return NULL;
}

template<typename Key, typename Data, typename HashFun>
typename LruHashMap<Key,Data,HashFun>::node_type*
LruHashMap<Key,Data,HashFun>::insert(const Key& key, const Data& val)
{
	uint32_t hash = _hash(key);
	uint32_t bkt = hash & _mask;
	Node *node;
	for (node = _tab[bkt]; node; node = node->hash_next)
	{
		if (node->hash == hash && node->key == key)
		{
			return NULL;
		}
	}

	if (_total >= _max_size || (node = new(&_pool) Node(key, val)) == NULL)
	{
		node = TAILQ_FIRST(&_lru);
		assert(node);
		remove_node(node);
		node = new(&_pool) Node(key, val);
	}

	node->hash = hash;
	TAILQ_INSERT_TAIL(&_lru, node, lru_link);
	node->hash_next = _tab[bkt];
	_tab[bkt] = node;
	_total++;
	return node;
}

template<typename Key, typename Data, typename HashFun>
typename LruHashMap<Key,Data,HashFun>::node_type*
LruHashMap<Key,Data,HashFun>::replace(const Key& key, const Data& val)
{
	uint32_t hash = _hash(key);
	uint32_t bkt = hash & _mask;
	Node *node;
	for (node = _tab[bkt]; node; node = node->hash_next)
	{
		if (node->hash == hash && node->key == key)
		{
			node->data = val;
			if (node != TAILQ_LAST(&_lru, lrulist_t))
			{
				TAILQ_REMOVE(&_lru, node, lru_link);
				TAILQ_INSERT_TAIL(&_lru, node, lru_link);
			}
			return node;
		}
	}

	if (_total >= _max_size || (node = new(&_pool) Node(key, val)) == NULL)
	{
		node = TAILQ_FIRST(&_lru);
		assert(node);
		remove_node(node);
		node = new(&_pool) Node(key, val);
	}

	node->hash = hash;
	TAILQ_INSERT_TAIL(&_lru, node, lru_link);
	node->hash_next = _tab[bkt];
	_tab[bkt] = node;
	_total++;
	return node;
}

template<typename Key, typename Data, typename HashFun>
bool LruHashMap<Key,Data,HashFun>::remove(const Key& key)
{
	uint32_t hash = _hash(key);
	uint32_t bkt = hash & _mask;
	Node *node, *prev = NULL;
	for (node = _tab[bkt]; node; prev = node, node = node->hash_next)
	{
		if (node->hash == hash && node->key == key)
		{
			if (prev)
				prev->hash_next = node->hash_next;
			else
				_tab[bkt] = node->hash_next;
			TAILQ_REMOVE(&_lru, node, lru_link);

			node->Node::~Node();
			obpool_release(&_pool, node);
			_total--;
			return true;
		}
	}
	return false;
}

template<typename Key, typename Data, typename HashFun>
void LruHashMap<Key,Data,HashFun>::remove_node(node_type* the_node)
{
	uint32_t bkt = (the_node->hash & _mask);
	Node *node, *prev = NULL;
	for (node = _tab[bkt]; node; prev = node, node = node->hash_next)
	{
		if (node == the_node)
		{
			if (prev)
				prev->hash_next = node->hash_next;
			else
				_tab[bkt] = node->hash_next;
			TAILQ_REMOVE(&_lru, node, lru_link);

			node->Node::~Node();
			obpool_release(&_pool, node);
			_total--;
			return;
		}
	}
	assert(!"can't reach here");
}

template<typename Key, typename Data, typename HashFun>
void LruHashMap<Key,Data,HashFun>::fresh_node(node_type* node)
{
	if (node != TAILQ_LAST(&_lru, lrulist_t))
	{
		TAILQ_REMOVE(&_lru, node, lru_link);
		TAILQ_INSERT_TAIL(&_lru, node, lru_link);
	}
}

template<typename Key, typename Data, typename HashFun>
void LruHashMap<Key,Data,HashFun>::stale_node(node_type* node)
{
	if (node != TAILQ_FIRST(&_lru))
	{
		TAILQ_REMOVE(&_lru, node, lru_link);
		TAILQ_INSERT_HEAD(&_lru, node, lru_link);
	}
}

template<typename Key, typename Data, typename HashFun>
size_t LruHashMap<Key,Data,HashFun>::drain(size_t num)
{
	Node *node;
	for (size_t i = 0; (node = TAILQ_FIRST(&_lru)) != NULL && i < num; ++i)
	{
		remove_node(node);
	}
	return _total;
}

template<typename Key, typename Data, typename HashFun>
typename LruHashMap<Key,Data,HashFun>::node_type* 
LruHashMap<Key,Data,HashFun>::most_fresh() const
{
	return TAILQ_LAST(&_lru, lrulist_t);
}

template<typename Key, typename Data, typename HashFun>
typename LruHashMap<Key,Data,HashFun>::node_type* 
LruHashMap<Key,Data,HashFun>::most_stale() const
{
	return TAILQ_FIRST(&_lru);
}

template<typename Key, typename Data, typename HashFun>
typename LruHashMap<Key,Data,HashFun>::node_type* 
LruHashMap<Key,Data,HashFun>::next_fresh(const node_type *node) const
{
	return node ? TAILQ_PREV(node, lrulist_t, lru_link) : TAILQ_LAST(&_lru, lrulist_t);
}

template<typename Key, typename Data, typename HashFun>
typename LruHashMap<Key,Data,HashFun>::node_type* 
LruHashMap<Key,Data,HashFun>::next_stale(const node_type *node) const
{
	return node ? TAILQ_NEXT(node, lru_link) : TAILQ_FIRST(&_lru);
}

}
#endif
