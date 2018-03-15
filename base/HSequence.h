#ifndef HSequence_h_
#define HSequence_h_

#include "hseq.h"
#include <stdint.h>
#include <string>
#include <vector>

namespace fpnn {

class HSequence
{
public:
	HSequence(const std::vector<std::string>& buckets, uint32_t keymask);

	HSequence(const std::vector<std::string>& buckets, const std::vector<int>& weights, uint32_t keymask);

	~HSequence();

	bool enable_cache();

	size_t size() const 			{ return hseq_total(_hseq); }
	uint32_t mask() const			{ return _mask; }

	int which(uint32_t keyhash);
	size_t sequence(uint32_t keyhash, int seqs[], size_t num);

	/* If (ssize_t)len < 0, key is NUL-terminated string. */
	int which(const void *key, size_t len);
	size_t sequence(const void *key, size_t len, int seqs[], size_t num);

private:
	hseq_t *_hseq;
	uint32_t _mask;
	bool _cache_u8;
	union {
		void *ptr;
		uint8_t *u8;
		uint16_t *u16;
	} _cache;
};
}
#endif
