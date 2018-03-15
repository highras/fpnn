#ifndef HSEQ_H_
#define HSEQ_H_

#include <stdlib.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef struct hseq_t hseq_t;

typedef struct {
	const void *identity;
	int idlen;	/* If idlen < 0, identity is NUL-terminated string */
	int weight;
} hseq_bucket_t;


hseq_t *hseq_create(const hseq_bucket_t buckets[], size_t num);

void hseq_destroy(hseq_t *hs);

size_t hseq_total(hseq_t *hs);


int hseq_hash_which(hseq_t *hs, uint32_t keyhash);

size_t hseq_hash_sequence(hseq_t *hs, uint32_t keyhash, int seqs[], size_t seqs_num);


/* If (ssize_t)len < 0, key is NUL-terminated string.
   The following two functions use CRC32 to hash the keys.
 */
int hseq_which(hseq_t *hs, const void *key, size_t len);

size_t hseq_sequence(hseq_t *hs, const void *key, size_t len, int seqs[], size_t seqs_num);


#ifdef __cplusplus
}
#endif

#endif
