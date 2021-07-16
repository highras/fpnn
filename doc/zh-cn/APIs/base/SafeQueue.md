## SafeQueue

### 介绍

线程安全的 queue 模板。

### 命名空间

	namespace fpnn;

### 关键定义

#### EmptyException

	template <typename K>
	class SafeQueue
	{
	public:
		class EmptyException: public std::exception
		{
		public:
			virtual const char* what() const throw ();
		};
	};

SafeQueue 为空时，`pop()` 接口将抛出的异常。

#### SafeQueue

	template <typename K>
	class SafeQueue
	{
	public:
		SafeQueue();
		~SafeQueue();
		
		bool empty();
		size_t size();
		void push(K &data);
		K pop();
		void clear();
		void swap(std::queue<K> &queue);
		void swap(SafeQueue<K> &squeue);
	};

### 成员函数

#### empty

	bool empty();

判断 SafeQueue 是否为空。

#### size

	size_t size();

获取 SafeQueue 数据长度。

#### push

	void push(K &data);

向 SafeQueue 队尾写入数据。

#### pop

	K pop();

获取 SafeQueue 队首数据，并将队首数据移出 SafeQueue。

#### clear

	void clear();

清除 SafeQueue 所有数据。

#### swap

	void swap(std::queue<K> &queue);
	void swap(SafeQueue<K> &squeue);

和 std::queue 或者 SafeQueue 交换数据。
