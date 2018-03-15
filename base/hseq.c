/*
   The idea is from 
	   Cache Array Routing Protocol v1.0
	   http://icp.ircache.net/carp.txt

   But the original hash function and hash combination function result in
   a quite unbalanced disrubtion. So we changed the hash function for key
   (url) to standard crc32, the hash function for item (member proxy) to
   a kind of crc64, and the hash combination function to a better one. 
*/
#include "hseq.h"
#include "crc.h"
#include "crc64.h"
#include "heap.h"
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

typedef struct item_t item_t;
struct item_t {
	uint32_t low;
	uint32_t high;
	double factor;
};

struct hseq_t {
	size_t total;
	bool weighted;
	item_t items[];
};

hseq_t *hseq_create(const hseq_bucket_t buckets[], size_t num)
{
	size_t i;
	hseq_t *hs;
	double sum = 0.0;
	int weight;

	if ((ssize_t)num <= 0)
		return NULL;

	hs = (hseq_t *)malloc(sizeof(hseq_t) + num * sizeof(item_t));
	if (!hs)
		return NULL;

	hs->total = num;
	hs->weighted = false;

	weight = buckets[0].weight > 0 ? buckets[0].weight : 0;
	for (i = 0; i < num; ++i)
	{
		const hseq_bucket_t *b = &buckets[i];
		uint64_t hash = (b->idlen >= 0) ? crc64_checksum(b->identity, b->idlen)
						: crc64_checksum_cstr((char *)b->identity);
		hs->items[i].low = hash;
		hs->items[i].high = hash >> 32;
		hs->items[i].factor = 1.0;

		int w = b->weight > 0 ? b->weight : 0;
		sum += w;
		if (w != weight)
			hs->weighted = true;
	}

	if (hs->weighted)
	{
		double product;
		double p0, x0;
		int k;

		int *idxs = (int *)malloc(num * sizeof(int));
		if (!idxs)
			goto error;

		for (i = 0; i < num; ++i)
			idxs[i] = i;

#define IDX_LESS(h, r1, r2) 	(buckets[h[r1]].weight < buckets[h[r2]].weight 	\
					|| (buckets[h[r1]].weight == buckets[h[r2]].weight && h[r1] < h[r2]))
#define IDX_SWAP(h, r1, r2)	do {			\
	int tmp = h[r1]; h[r1] = h[r2]; h[r2] = tmp;	\
} while (0)
		HEAPIFY(idxs, num, IDX_LESS, IDX_SWAP);
		HEAP_SORT(idxs, num, IDX_LESS, IDX_SWAP);
#undef IDX_SWAP
#undef IDX_LESS

		for (i = 0; i < num && buckets[idxs[i]].weight <= 0; ++i)
		{
			hs->items[idxs[i]].factor = 0.0;
		}

		k = idxs[i];
		p0 = buckets[k].weight / sum;
		x0 = pow(num * p0, 1.0/num);
		product = x0;
		hs->items[k].factor = x0;

		for (++i; i < num; ++i)
		{
			double p1 = p0;
			double x1 = x0;
			size_t ni = num - i;

			k = idxs[i];
			p0 = buckets[k].weight / sum;
			x0 = ni * (p0 - p1) / product;
			x0 += pow(x1, ni);
			x0 = pow(x0, 1.0/ni);
			product *= x0;
			hs->items[k].factor = x0;
		}

		free(idxs);
	}

	return hs;
error:
	if (hs)
		free(hs);
	return NULL;
}

void hseq_destroy(hseq_t *hs)
{
	if (hs)
		free(hs);
}

size_t hseq_total(hseq_t *hs)
{
	return hs->total;
}

static inline uint32_t MIX(uint32_t a, uint32_t low, uint32_t high)
{
	a = (a - low) ^ high;

	a = (a ^ 61) ^ (a >> 16);
	a = a + (a << 3);
	a = a ^ (a >> 4);
	a = a * 0x27d4eb2d;
	a = a ^ (a >> 15);
	return a;
}

int hseq_which(hseq_t *hs, const void *key, size_t len)
{
	uint32_t khash = (ssize_t)len >= 0 ? crc32_checksum(key, len) : crc32_checksum_cstr((char *)key);
	return hseq_hash_which(hs, khash);
}

int hseq_hash_which(hseq_t *hs, uint32_t keyhash)
{
	size_t i;
	int idx = 0;

	if (hs->weighted)
	{
		double max = 0.0;
		for (i = 0; i < hs->total; ++i)
		{
			item_t *it = &hs->items[i];
			double x = it->factor * MIX(keyhash, it->low, it->high);

			if (x > max)
			{
				max = x;
				idx = i;
			}
		}
	}
	else
	{
		uint32_t max = 0;
		for (i = 0; i < hs->total; ++i)
		{
			item_t *it = &hs->items[i];
			uint32_t x = MIX(keyhash, it->low, it->high);

			if (x > max)
			{
				max = x;
				idx = i;
			}
		}
	}
	return idx;
}


struct i_result_t {
	uint32_t x;
	int idx;
};

struct r_result_t {
	double x;
	int idx;
};

#define GREATER(h, r1, r2)	((h[r1] > h[r2]) || (h[r1] == h[r2] && seqs[r1] < seqs[r2]))

#define SWAP_I(h, i1, i2)		do { 				\
	uint32_t tmp = h[i1]; h[i1] = h[i2]; h[i2] = tmp;		\
	int idx = seqs[i1]; seqs[i1] = seqs[i2]; seqs[i2] = idx;	\
} while(0)

#define SWAP_R(h, i1, i2)		do { 				\
	double tmp = h[i1]; h[i1] = h[i2]; h[i2] = tmp;			\
	int idx = seqs[i1]; seqs[i1] = seqs[i2]; seqs[i2] = idx;	\
} while(0)

size_t hseq_sequence(hseq_t *hs, const void *key, size_t len, int *seqs, size_t seqs_num)
{
	uint32_t khash = (ssize_t)len >= 0 ? crc32_checksum(key, len) : crc32_checksum_cstr((char *)key);
	return hseq_hash_sequence(hs, khash, seqs, seqs_num);
}

size_t hseq_hash_sequence(hseq_t *hs, uint32_t keyhash, int *seqs, size_t seqs_num)
{
	size_t i;

	if ((ssize_t)seqs_num <= 0)
		return 0;

	if (seqs_num > hs->total)
		seqs_num = hs->total;

	if (hs->weighted)
	{
		double *res = (double *)alloca(seqs_num * sizeof(res[0]));
		for (i = 0; i < seqs_num; ++i)
		{
			item_t *it = &hs->items[i];
			double x = it->factor * MIX(keyhash, it->low, it->high);

			res[i] = x;
			seqs[i] = i;
			HEAP_FIX_UP(res, i, GREATER, SWAP_R);
		}

		for (; i < hs->total; ++i)
		{
			item_t *it = &hs->items[i];
			double x = it->factor * MIX(keyhash, it->low, it->high);

			if (x > res[0])
			{
				res[0] = x;
				seqs[0] = i;
				HEAP_FIX_DOWN(res, 0, seqs_num, GREATER, SWAP_R);
			}
		}

		HEAP_SORT(res, seqs_num, GREATER, SWAP_R);
	}
	else
	{
		uint32_t *res = (uint32_t *)alloca(seqs_num * sizeof(res[0]));
		for (i = 0; i < seqs_num; ++i)
		{
			item_t *it = &hs->items[i];
			uint32_t x = MIX(keyhash, it->low, it->high);

			res[i] = x;
			seqs[i] = i;
			HEAP_FIX_UP(res, i, GREATER, SWAP_I);
		}

		for (; i < hs->total; ++i)
		{
			item_t *it = &hs->items[i];
			uint32_t x = MIX(keyhash, it->low, it->high);

			if (x > res[0])
			{
				res[0] = x;
				seqs[0] = i;
				HEAP_FIX_DOWN(res, 0, seqs_num, GREATER, SWAP_I);
			}
		}

		HEAP_SORT(res, seqs_num, GREATER, SWAP_I);
	}

	return seqs_num;
}

#undef SWAP_R
#undef SWAP_I
#undef GREATER


#ifdef TEST_HSEQ

#include <stdio.h>

void get_random_name(char *str, size_t size)
{
	size_t i;
	if (size == 0)
		return;
	--size;
	for (i = 0; i < size; ++i)
		str[i] = 'a' + random()%26;
	str[i] = 0;
}

#define SIZE	12
#define NUM	256

int main(int argc, char **argv)
{
	int i;
	hseq_bucket_t buckets[NUM];
	int count[NUM] = {0};
	int seq[NUM];
	char buf[SIZE];
	hseq_t *hs;
	
	for (i = 0; i < NUM; ++i)
	{
		buckets[i].identity = malloc(SIZE);
		buckets[i].idlen = -1;
		buckets[i].weight = i%2 ? 1 : 2;
		get_random_name((char *)buckets[i].identity, SIZE);
	}

	hs = hseq_create(buckets, NUM);

	get_random_name(buf, sizeof(buf));
	for (i = 0; i < 1024 * 1024; ++i)
	{
		get_random_name(buf, sizeof(buf));
		hseq_sequence(hs, buf, -1, seq, 5);
//		seq[0] = hseq_which(hs, buf, -1);
		count[seq[0]]++;
	}
	
	for (i = 0; i < NUM; ++i)
		printf("%d\t= %d\n", i, count[i]);

	return 0;
}

#endif
