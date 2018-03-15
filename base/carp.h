/*
   Cache Array Routing Protocol v1.0
   http://icp.ircache.net/carp.txt

   But we changed the original Url and Proxy Member from string to hash
   values. This lets the user be able to choose their favorite hash functions.
   We also changed the hash combination function to get an uniform 
   distribution result.
*/
#ifndef CARP_H_
#define CARP_H_

#include <stdlib.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef uint32_t (*carp_combine_function)(uint64_t member, uint32_t key);

typedef struct carp_t carp_t;


carp_t *carp_create(const uint64_t *members, size_t num, carp_combine_function combine/*NULL*/);

carp_t *carp_create_with_weight(const uint64_t *members, const uint32_t *weights/*NULL*/,
				size_t num, carp_combine_function combine/*NULL*/);


void carp_destroy(carp_t *carp);


size_t carp_total(carp_t *carp);


int carp_which(carp_t *carp, uint32_t key);


size_t carp_sequence(carp_t *carp, uint32_t key, int seqs[], size_t seqs_num);



#ifdef __cplusplus
}
#endif

#endif
