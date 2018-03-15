#ifndef OBPOOL_H_
#define OBPOOL_H_ 1


#include <stdlib.h>
#include <string.h>
#include <limits.h>


#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	unsigned long obj_size;
	unsigned long num_acquire;
	unsigned long num_total;
	unsigned long mem_total;
	unsigned long num_limit;
	struct _pool_obj *obj_list;
	struct _pool_clist *clist;
	void *(*chunk_alloc)(void *allocator, size_t size);
	void (*chunk_free)(void *allocator, void *p);
	void *chunk_allocator;
} obpool_t;


/*
 * NB: OBJ_SIZE must be larger than sizeof(void *).
 * The allocated memory is aligned at OBJ_SIZE boundry.
 * If you want it aligned at some power of two boundry, 
 * you should round up the OBJ_SIZE you self.
 */
#define OBPOOL_INITIALIZER(OBJ_SIZE)				\
	OBPOOL_LIMIT_INITIALIZER(OBJ_SIZE, ULONG_MAX)


#define OBPOOL_LIMIT_INITIALIZER(OBJ_SIZE, MAX_NUM_OBJ)		\
	OBPOOL_FULL_INITIALIZER(OBJ_SIZE, MAX_NUM_OBJ, 0, 0, 0)

#define OBPOOL_FULL_INITIALIZER(OBJ_SIZE, MAX_NUM_OBJ, ALLOC, FREE, ALLOCATOR)	\
	{ (OBJ_SIZE), 0, 0, 0, (MAX_NUM_OBJ), 0, 0, (ALLOC), (FREE), (ALLOCATOR) }


#define obpool_init(POOL, OBJ_SIZE)				\
	obpool_limit_init(POOL, OBJ_SIZE, ULONG_MAX)


#define obpool_limit_init(POOL, OBJ_SIZE, MAX_NUM_OBJ)		\
	obpool_full_init(POOL, OBJ_SIZE, MAX_NUM_OBJ, 0, 0, 0)


#define obpool_full_init(POOL, OBJ_SIZE, MAX_NUM_OBJ, ALLOC, FREE, ALLOCATOR)	do {	\
	obpool_t *_p__ = (POOL);				\
	memset(_p__, 0, sizeof(obpool_t));			\
	_p__->num_limit = (MAX_NUM_OBJ);			\
	if ((_p__->obj_size = (OBJ_SIZE)) < sizeof(void *))	\
		_p__->obj_size = sizeof(void *);		\
	_p__->chunk_alloc = (ALLOC);				\
	_p__->chunk_free = (FREE);				\
	_p__->chunk_allocator = (ALLOCATOR);			\
} while (0)


extern obpool_t *obpool8;
extern obpool_t *obpool16;
extern obpool_t *obpool32;


void obpool_finish(obpool_t *pool);


void *obpool_acquire(obpool_t *pool);

void *obpool_acquire_zero(obpool_t *pool);

void obpool_release(obpool_t *pool, void *obj);


void *obpool_mt_acquire(obpool_t *pool);

void *obpool_mt_acquire_zero(obpool_t *pool);

void obpool_mt_release(obpool_t *pool, void *obj);


#ifdef __cplusplus
}
#endif

#endif

