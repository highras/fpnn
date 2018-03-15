#ifndef HashArray_h_
#define HashArray_h_

#include "hashint.h"
#include <stdlib.h>
#include <stdexcept>

namespace fpnn {

template<typename T>
class HashArray
{
public:
	enum { MAX_SIZE = 2147483648U };

	HashArray(size_t size): _size(size)
	{
		if (size == 0 || size > MAX_SIZE)
			throw std::out_of_range("HashArray size is zero or too large");
		_array = new T[_size];
	}

	~HashArray()
	{
		delete[] _array;
	}

	size_t size() const
	{
		return _size;
	}

	T& operator[](size_t idx) const
	{
		if (idx >= _size)
			throw std::out_of_range("HashArray index should be less than size");
		return _array[idx];
	}

	T& which(const void *p) const
	{
		return _array[hash32_uintptr((uintptr_t)p) % _size];
	}

private:
	T *_array;
	const uint32_t _size;
};

}

#endif

