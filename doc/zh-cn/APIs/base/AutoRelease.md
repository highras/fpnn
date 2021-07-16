## AutoRelease

### 介绍

类似于 Go 的 defer。  
但 Go 的 defer 是在函数退出前执行，而 AutoRelease 系列是在作用域(函数/代码块)结束时执行。

### 命名空间

	namespace fpnn;

#### AutoDeleteGuard

	template <typename K>
	class AutoDeleteGuard
	{
	public:
		AutoDeleteGuard(K* naked_ptr);
		~AutoDeleteGuard();
		void release();
	};

当超出作用域时，自动调用 delete 释放初始化时传入的已经 new 出来的对象。

* **void release()**

	取消自动执行。

#### AutoDeleteArrayGuard

	template <typename K>
	class AutoDeleteArrayGuard
	{
	public:
		AutoDeleteArrayGuard(K* naked_ptr);
		~AutoDeleteArrayGuard();
	};

当超出作用域时，自动调用 delete [] 释放初始化时传入的已经 new 出来的**对象数组**。

#### AutoFreeGuard

	class AutoFreeGuard
	{
	public:
		AutoFreeGuard(void* naked_ptr);
		~AutoFreeGuard();
	};

当超出作用域时，自动调用 free 释放初始化时传入的已经 malloc 出来的内存块。

#### PointerContainerGuard

	template <template <typename, typename> class Container, typename Element, class Alloc = std::allocator<Element*>>
	class PointerContainerGuard
	{
	public:
		PointerContainerGuard(Container<Element*, Alloc>* container, bool deleteCounter = false);
		~PointerContainerGuard();
		void release();
	};

适用于指向指针容器的容器指针对象。  
适用容器对象为：vector、stack、list、deque、queue 等，不适用于 set 和 map。  
当超出作用域时，自动调用将遍历容器内的指针，逐一调用 delete 进行释放，然后调用 delete 释放容器。

* **void release()**

	取消自动执行。

#### FinallyGuard

	class FinallyGuard
	{
	public:
		explicit FinallyGuard(std::function<void ()> function);
		virtual ~FinallyGuard();
	};

当超出作用域时，自动执行 lambda 函数。

#### ConditionalFinallyGuard

	template <typename K>
	class ConditionalFinallyGuard
	{
	public:
		explicit ConditionalFinallyGuard(std::function<void (const K& cond)> function, const K& defaultCondition);
		virtual ~ConditionalFinallyGuard();
		void changeCondition(const K& condition);
	};

当超出作用域时，自动使用最后设置的 condition 参数执行 lambda 函数。

* **void changeCondition(const K& condition)**

	更改自动执行时的 condition 参数。 

#### CannelableFinallyGuard

	class CannelableFinallyGuard
	{
	public:
		explicit CannelableFinallyGuard(std::function<void ()> function);
		virtual ~CannelableFinallyGuard();
		void cancel();
	};

当超出作用域时，自动执行 lambda 函数。中途可调用 `cancel()` 方法取消执行。

* **void cancel()**

	取消自动执行。