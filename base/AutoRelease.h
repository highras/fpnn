#ifndef FPNN_Auto_Release_H
#define FPNN_Auto_Release_H

#include <stdlib.h>
#include <memory>
#include <functional>

namespace fpnn {

template <typename K>
class AutoDeleteGuard
{
	K* _nakedPtr;

public:
	AutoDeleteGuard(K* naked_ptr): _nakedPtr(naked_ptr) {}
	~AutoDeleteGuard() { if (_nakedPtr) delete _nakedPtr; }
	void release() { _nakedPtr = 0; }
};

template <typename K>
class AutoDeleteArrayGuard
{
	K* _nakedPtr;

public:
	AutoDeleteArrayGuard(K* naked_ptr): _nakedPtr(naked_ptr) {}
	~AutoDeleteArrayGuard() { delete []_nakedPtr; }
};

class AutoFreeGuard
{
	void* _buffer;

public:
	AutoFreeGuard(void* naked_ptr): _buffer(naked_ptr) {}
	~AutoFreeGuard() { free(_buffer); }
};

//-- for vector, stack, list, deque, queue, ... Not for set & map.
template <template <typename, typename> class Container, typename Element, class Alloc = std::allocator<Element*>>
class PointerContainerGuard
{
	Container<Element*, Alloc>* _container;
	bool _deleteCounter;

public:
	PointerContainerGuard(Container<Element*, Alloc>* container, bool deleteCounter = false):
		_container(container), _deleteCounter(deleteCounter) {}

	~PointerContainerGuard()
	{
		if (_container)
		{
			for (Element* e: *_container)
				delete e;

			if (_deleteCounter)
				delete _container;
		}
	}

	void release() { _container = 0; }
};

/*
	If you want to using goto to clean something, you can use FinallyGuard to instead.
*/
class FinallyGuard
{
private:
	std::function<void ()> _function;

public:
	explicit FinallyGuard(std::function<void ()> function): _function(function) {}
	virtual ~FinallyGuard()
	{
		_function();
	}
};

template <typename K>
class ConditionalFinallyGuard
{
private:
	K _condition;
	std::function<void (const K& cond)> _function;

public:
	explicit ConditionalFinallyGuard(std::function<void (const K& cond)> function, const K& defaultCondition):
		_condition(defaultCondition), _function(function) {} 
	virtual ~ConditionalFinallyGuard()
	{
		_function(_condition);
	}
	void changeCondition(const K& condition) { _condition = condition; }
};

class CannelableFinallyGuard
{
private:
	bool _cannel;
	std::function<void ()> _function;

public:
	explicit CannelableFinallyGuard(std::function<void ()> function): _cannel(false), _function(function) {}
	virtual ~CannelableFinallyGuard()
	{
		if (!_cannel)
			_function();
	}
	void cancel() { _cannel = true; }
};

}

#endif
