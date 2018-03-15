#include "HSequence.h"
#include "bit.h"
#include "crc.h"

using namespace fpnn;

HSequence::HSequence(const std::vector<std::string>& buckets, uint32_t keymask)
{
	_cache.ptr = NULL;
	_mask = keymask ? round_up_power_two(keymask) - 1 : UINT32_MAX;

	size_t size = buckets.size();
	std::vector<hseq_bucket_t> bs(size);
	for (size_t i = 0; i < size; ++i)
	{
		bs[i].identity = buckets[i].data();
		bs[i].idlen = buckets[i].length();
		bs[i].weight = 0;
	}
	_hseq = hseq_create(&bs[0], size);
}

HSequence::HSequence(const std::vector<std::string>& buckets, const std::vector<int>& weights, uint32_t keymask)
{
	_cache.ptr = NULL;
	_mask = keymask ? round_up_power_two(keymask) - 1 : UINT32_MAX;

	size_t size = buckets.size();
	std::vector<hseq_bucket_t> bs(size);
	for (size_t i = 0; i < size; ++i)
	{
		bs[i].identity = buckets[i].data();
		bs[i].idlen = buckets[i].length();
		bs[i].weight = weights[i];
	}
	_hseq = hseq_create(&bs[0], size);
}

HSequence::~HSequence()
{
	hseq_destroy(_hseq);
	if (_cache.ptr)
		free(_cache.ptr);
}

bool HSequence::enable_cache()
{
	if (!_cache.ptr && _mask <= INT32_MAX)
	{
		size_t size = hseq_total(_hseq);
		if (size < UINT8_MAX)
		{
			_cache_u8 = true;
			_cache.ptr = calloc((_mask + 1), sizeof(uint8_t));
		}
		else if (size < UINT16_MAX)
		{
			_cache_u8 = false;
			_cache.ptr = calloc((_mask + 1), sizeof(uint16_t));
		}
	}

	return bool(_cache.ptr);
}

int HSequence::which(uint32_t keyhash)
{
	uint32_t h = keyhash & _mask;
	if (_cache.ptr)
	{
		int x = _cache_u8 ? _cache.u8[h] : _cache.u16[h];
		if (!x)
		{
			x = hseq_hash_which(_hseq, h) + 1;
			if (_cache_u8)
				_cache.u8[h] = x;
			else
				_cache.u16[h] = x;
		}
		return x - 1;
	}

	return hseq_hash_which(_hseq, h);
}

int HSequence::which(const void *key, size_t len)
{
	uint32_t keyhash = (ssize_t)len >= 0 ? crc32_checksum(key, len) : crc32_checksum_cstr((char *)key);
	return this->which(keyhash);
}

size_t HSequence::sequence(uint32_t keyhash, int seqs[], size_t num)
{
	return hseq_hash_sequence(_hseq, keyhash & _mask, seqs, num);
}

size_t HSequence::sequence(const void *key, size_t len, int seqs[], size_t num)
{
	uint32_t keyhash = (ssize_t)len >= 0 ? crc32_checksum(key, len) : crc32_checksum_cstr((char *)key);
	return hseq_hash_sequence(_hseq, keyhash & _mask, seqs, num);
}

