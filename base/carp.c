#include "carp.h"
#include "heap.h"
#include <endian.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <limits.h>

typedef struct item_t item_t;
struct item_t
{
	uint64_t hash;
	double factor;
};

struct carp_t
{
	int total;
	bool weighted;
	carp_combine_function combine;
	item_t items[];
};

/* http://www.cris.com/~Ttwang/tech/inthash.htm
*/
static inline uint32_t hash32shiftmult(uint32_t a)
{
	a = (a ^ 61) ^ (a >> 16);
	a = a + (a << 3);
	a = a ^ (a >> 4);
	a = a * 0x27d4eb2d;
	a = a ^ (a >> 15);
	return a;
}

static uint32_t hash_combine(uint64_t m, uint32_t k)
{
	uint32_t low = m;
	uint32_t high = m >> 32;
	k = (k - low) ^ high;
	return hash32shiftmult(k);
}

carp_t *carp_create(const uint64_t *members, size_t num, carp_combine_function combine/*NULL*/)
{
	return carp_create_with_weight(members, NULL, num, combine);
}

carp_t *carp_create_with_weight(const uint64_t *members, const uint32_t *weights/*NULL*/,
				size_t num, carp_combine_function combine/*NULL*/)
{
	size_t i;
	carp_t *carp;
	double sum = 0.0;
	uint32_t weight;

	if (num > INT_MAX || num == 0)
		return NULL;

	carp = (carp_t *)malloc(sizeof(carp_t) + num * sizeof(item_t));
	if (!carp)
		return NULL;

	carp->total = num;
	carp->weighted = false;
	carp->combine = combine ? combine : hash_combine;

	weight = weights ? weights[0] : 1;
	for (i = 0; i < num; ++i)
	{
		carp->items[i].hash = members[i];
		carp->items[i].factor = 1.0;
		if (weights)
		{
			sum += weights[i];
			if (weights[i] != weight)
				carp->weighted = true;
		}
	}

	if (carp->weighted)
	{
		double product;
		double p0, x0;
		int k;

		int *idxs = (int *)malloc(num * sizeof(int));
		if (!idxs)
			goto error;

		for (i = 0; i < num; ++i)
			idxs[i] = i;

#define IDX_LESS(h, r1, r2) 	(weights[h[r1]] < weights[h[r2]] 	\
					|| (weights[h[r1]] == weights[h[r2]] && h[r1] < h[r2]))
#define IDX_SWAP(h, r1, r2)	do {			\
	int tmp = h[r1]; h[r1] = h[r2]; h[r2] = tmp;	\
} while (0)
		HEAPIFY(idxs, num, IDX_LESS, IDX_SWAP);
		HEAP_SORT(idxs, num, IDX_LESS, IDX_SWAP);
#undef IDX_SWAP
#undef IDX_LESS

		for (i = 0; i < num && weights[idxs[i]] == 0; ++i)
		{
			carp->items[idxs[i]].factor = 0.0;
		}

		k = idxs[i];
		p0 = weights[k] / sum;
		x0 = pow(num * p0, 1.0 / num);
		product = x0;
		carp->items[k].factor = x0;

		for (++i; i < num; ++i)
		{
			double p1 = p0;
			double x1 = x0;
			size_t ni = num - i;

			k = idxs[i];
			p0 = weights[k] / sum;
			x0 = ni * (p0 - p1) / product;
			x0 += pow(x1, ni);
			x0 = pow(x0, 1.0 / ni);
			product *= x0;
			carp->items[k].factor = x0;
		}

		free(idxs);
	}

	return carp;
error:
	if (carp)
		free(carp);
	return NULL;
}

void carp_destroy(carp_t *carp)
{
	if (carp)
		free(carp);
}

size_t carp_total(carp_t *carp)
{
	return carp->total;
}

int carp_which(carp_t *carp, uint32_t keyhash)
{
	size_t i;
	int idx = 0;

	if (carp->weighted)
	{
		double max = 0.0;
		for (i = 0; i < carp->total; ++i)
		{
			item_t *it = &carp->items[i];
			double x = it->factor * carp->combine(it->hash, keyhash);

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
		for (i = 0; i < carp->total; ++i)
		{
			item_t *it = &carp->items[i];
			uint32_t x = carp->combine(it->hash, keyhash);

			if (x > max)
			{
				max = x;
				idx = i;
			}
		}
	}
	return idx;
}


#define GREATER(h, r1, r2)	((h[r1] > h[r2]) || (h[r1] == h[r2] && seqs[r1] < seqs[r2]))

#define SWAP_I(h, i1, i2)		do { 				\
	uint32_t tmp = h[i1]; h[i1] = h[i2]; h[i2] = tmp;		\
	int idx = seqs[i1]; seqs[i1] = seqs[i2]; seqs[i2] = idx;	\
} while(0)

#define SWAP_R(h, i1, i2)		do { 				\
	double tmp = h[i1]; h[i1] = h[i2]; h[i2] = tmp;			\
	int idx = seqs[i1]; seqs[i1] = seqs[i2]; seqs[i2] = idx;	\
} while(0)

size_t carp_sequence(carp_t *carp, uint32_t keyhash, int *seqs, size_t seqs_num)
{
	size_t i;

	if ((ssize_t)seqs_num <= 0)
		return 0;

	if (seqs_num > carp->total)
		seqs_num = carp->total;

	if (carp->weighted)
	{
		double *res = (double *)alloca(seqs_num * sizeof(res[0]));
		for (i = 0; i < seqs_num; ++i)
		{
			item_t *it = &carp->items[i];
			double x = it->factor * carp->combine(it->hash, keyhash);

			res[i] = x;
			seqs[i] = i;
			HEAP_FIX_UP(res, i, GREATER, SWAP_R);
		}

		for (; i < carp->total; ++i)
		{
			item_t *it = &carp->items[i];
			double x = it->factor * carp->combine(it->hash, keyhash);

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
			item_t *it = &carp->items[i];
			uint32_t x = carp->combine(it->hash, keyhash);

			res[i] = x;
			seqs[i] = i;
			HEAP_FIX_UP(res, i, GREATER, SWAP_I);
		}

		for (; i < carp->total; ++i)
		{
			item_t *it = &carp->items[i];
			uint32_t x = carp->combine(it->hash, keyhash);

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


#ifdef TEST_CARP

#include "crc.h"
#include "crc64.h"
#include <stdio.h>
#include <iostream>

void get_random_name(char *str, size_t size)
{
	size_t i;
	if (size == 0)
		return;
	--size;
	for (i = 0; i < size; ++i)
		str[i] = 'a' + random() % 26;
	str[i] = 0;
}

#define SIZE	25
#define NUM	256

int main(int argc, char **argv)
{
	int i;
	uint64_t members[NUM];
	uint32_t weights[NUM];
	int count[NUM] = {0};
	int seq[NUM];
	char buf[SIZE] = {0};
	carp_t *carp;
	
	for (i = 0; i < NUM-2 ; ++i)
	{
		//get_random_name(buf, sizeof(buf));
            
                snprintf(buf, sizeof(buf), "10.1.1.%d:11211",i);
		members[i] = crc64_checksum_cstr(buf);
		//weights[i] = i % 2 ? 1 : 2;
		weights[i] = 1 ;
	}

	carp = carp_create_with_weight(members, weights, NUM, NULL);

	for (i = 0; i < 1024 * 1024; ++i)
	{
		uint32_t hash;
		get_random_name(buf, sizeof(buf));
                snprintf(buf, sizeof(buf), "%d",i);
		hash = crc32_checksum_cstr(buf);
/*		seq[0] = carp_which(carp, hash); */
		carp_sequence(carp, hash, seq, 5);
		count[seq[0]]++;
		std::cout << i << "\t" << seq[0] << std::endl;
	}
	// output distribution
	/*
	for (i = 0; i < NUM; ++i)
		printf("%d\t%lu\t%d\n", i, members[i], count[i]);
	*/
	return 0;
}

#endif
