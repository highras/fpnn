#ifndef HashFunctor_h_
#define HashFunctor_h_

#include "jenkins.h"
#include "hashint.h"
#include <string>

namespace fpnn {

template<typename T>
struct HashFunctor
{
	unsigned int operator()(const T& t)
	{
		return t.hash();
	}
};

template<>
struct HashFunctor<std::string>
{
	unsigned int operator()(const std::string& t)
	{
		return jenkins_hash(t.c_str(), t.length(), 0);
	}
};

template<>
struct HashFunctor<uint16_t>
{
	unsigned int operator()(const uint16_t& t)
	{
		return hash32_uint32(t);
	}
};

template<>
struct HashFunctor<int16_t>
{
	unsigned int operator()(const int16_t& t)
	{
		return hash32_uint32(t);
	}
};

template<>
struct HashFunctor<uint32_t>
{
	unsigned int operator()(const uint32_t& t)
	{
		return hash32_uint32(t);
	}
};

template<>
struct HashFunctor<int32_t>
{
	unsigned int operator()(const int32_t& t)
	{
		return hash32_uint32(t);
	}
};

template<>
struct HashFunctor<uint64_t>
{
	unsigned int operator()(const uint64_t& t)
	{
		return hash32_uint64(t);
	}
};

template<>
struct HashFunctor<int64_t>
{
	unsigned int operator()(const int64_t& t)
	{
		return hash32_uint64(t);
	}
};

}
#endif
