#ifndef JENKINS_H_
#define JENKINS_H_ 1

#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif


/* The same as jenkins_hashlittle(). */
uint32_t jenkins_hash(const void *key, size_t length, uint32_t initval);


/* The same as jenkins_hashlittle2(). */
void jenkins_hash2(const void *key, size_t length, uint32_t *pc, uint32_t *pb);


uint64_t jenkins_hash64(const void *key, size_t length, uint64_t initval);



uint32_t jenkins_hashword(
	const uint32_t *k,            	/* the key, an array of uint32_t values */
	size_t          length,       	/* the length of the key, in uint32_ts */
	uint32_t        initval);	/* the previous hash, or an arbitrary value */


void jenkins_hashword2 (
	const uint32_t *k,              /* the key, an array of uint32_t values */
	size_t          length,         /* the length of the key, in uint32_ts */
	uint32_t       *pc,             /* IN: seed OUT: primary hash value */
	uint32_t       *pb);         	/* IN: more seed OUT: secondary hash value */


uint32_t jenkins_hashlittle(const void *key, size_t length, uint32_t initval);


void jenkins_hashlittle2( 
	const void *key,       /* the key to hash */
	size_t      length,    /* length of the key */
	uint32_t   *pc,        /* IN: primary initval, OUT: primary hash */
	uint32_t   *pb);       /* IN: secondary initval, OUT: secondary hash */


uint32_t jenkins_hashbig( const void *key, size_t length, uint32_t initval);


#ifdef __cplusplus
}
#endif

#endif

