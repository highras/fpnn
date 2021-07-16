## LruHashMap

### 介绍

最近最少使用的 hash 字典容器。

### 命名空间

	namespace fpnn;

### 关键定义

	template<typename Key, typename Data, typename HashFun=HashFunctor<Key> >
	class LruHashMap
	{
		struct Node
		{
			Key key;
			Data data;
		};

	public:
		typedef Key key_type;
		typedef Data data_type;
		typedef Node node_type;
		typedef HashFun hash_func;

		LruHashMap(size_t slot_num, size_t max_size = 0);
		~LruHashMap();

		size_t count() const;

		node_type* find(const Key& key);
		node_type* use(const Key& key);
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
	};

### 构造函数

	LruHashMap(size_t slot_num, size_t max_size = 0);

**参数说明**

* **`slot_num`**

	hash 槽数。会被圆整为 2 的次方。

* **`max_size`**

	最大 hash 槽数。`0`表示与 `slot_num` 相同。

### 成员函数

#### count

	size_t count() const;

当前容器包含条目数量。

#### find

	node_type* find(const Key& key);

查找对象。但不改变 LRU 状态。

#### use

	node_type* use(const Key& key);

查找，如果找到，更新 LRU 状态。

#### insert

	node_type* insert(const Key& key, const Data& val);

插入数据。  
如果键值相同，则取消插入，并返回 `NULL`，否则返回插入后的节点指针。

#### replace

	node_type* replace(const Key& key, const Data& val);

插入或者覆盖数据。  
返回插入后/覆盖后节点的指针。

#### remove

	bool remove(const Key& key);

删除 `key` 对应的节点 & 数据。

#### remove_node

	void remove_node(node_type* node);

删除 `node` 对应的节点 & 数据。

#### fresh_node

	void fresh_node(node_type* node);

将节点对应的 LRU 状态调整为最近使用。

#### stale_node

	void stale_node(node_type* node);

将节点对应的 LRU 状态调整为最久未使用。

#### drain

	size_t drain(size_t num);

删除最久未使用的 `num` 个节点。

#### most_fresh

	node_type *most_fresh() const;

返回最近使用的节点。

#### most_stale

	node_type *most_stale() const;

返回最久未使用的节点。

#### next_fresh

	node_type *next_fresh(const node_type *node) const;

下一个最近使用的节点。

#### next_stale

	node_type *next_fresh(const node_type *node) const;

下一个最久未使用的节点。
