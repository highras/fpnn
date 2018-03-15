#include "obpool.h"
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <assert.h>
#include <pthread.h>

static pthread_mutex_t _mx = PTHREAD_MUTEX_INITIALIZER;
static obpool_t _op8 = OBPOOL_INITIALIZER(8);
static obpool_t _op16 = OBPOOL_INITIALIZER(16);
static obpool_t _op32 = OBPOOL_INITIALIZER(32);

obpool_t *obpool8 = &_op8;
obpool_t *obpool16 = &_op16;
obpool_t *obpool32 = &_op32;


#define PAGESIZE	4096
#define ROUNDUP_TO_PAGESIZE(x)	(((x) + PAGESIZE - 1) / PAGESIZE * PAGESIZE)

struct _pool_obj {
	struct _pool_obj *next;
};


#define NUM_PTRS	((PAGESIZE - sizeof(struct _head))/sizeof(void *))

struct _pool_clist;

struct _head {
	struct _pool_clist *next;
	size_t num;
};

struct _pool_clist {
	struct _head h;
	void *chunks[NUM_PTRS];
};


void obpool_finish(obpool_t *pool)
{
	if (pool->clist)
	{
		struct _pool_clist *clist, *next;
		if (pool->chunk_free)
		{
			for (clist = pool->clist; clist; clist = next)
			{
				int i;
				next = clist->h.next;
				for (i = clist->h.num - 1; i >= 0; --i)
					pool->chunk_free(pool->chunk_allocator, clist->chunks[i]);
				pool->chunk_free(pool->chunk_allocator, clist);
			}
		}
		else if (!pool->chunk_alloc)
		{
			for (clist = pool->clist; clist; clist = next)
			{
				int i;
				next = clist->h.next;
				for (i = clist->h.num - 1; i >= 0; --i)
					free(clist->chunks[i]);
				free(clist);
			}
		}
	}
	else
	{
		struct _pool_obj *obj, *next;
		if (pool->chunk_free)
		{
			for (obj = pool->obj_list; obj; obj = next)
			{
				next = obj->next;
				pool->chunk_free(pool->chunk_allocator, obj);
			}
		}
		else if (!pool->chunk_alloc)
		{
			for (obj = pool->obj_list; obj; obj = next)
			{
				next = obj->next;
				free(obj);
			}
		}
	}
	pool->obj_size = 0;
	pool->obj_list = NULL;
	pool->clist = NULL;
}

static struct _pool_obj *grow(obpool_t *pool)
{
	size_t obj_size = pool->obj_size;
	size_t chunk_num_obj;

	assert(pool->num_total < pool->num_limit);

	/* align to sizeof(void *) */
	if (obj_size % sizeof(struct _pool_obj))
	{
		size_t mask = sizeof(struct _pool_obj) - 1;
		obj_size = (obj_size + mask) & ~mask;
		pool->obj_size = obj_size;
	}

	if (obj_size >= 16384)
	{
		chunk_num_obj = 0;
	}
	else
	{
		chunk_num_obj = PAGESIZE / obj_size;
		if (chunk_num_obj < 8)
			chunk_num_obj = 8;

		if (chunk_num_obj > pool->num_limit - pool->num_total)
			chunk_num_obj = pool->num_limit - pool->num_total;
	}
	
	assert(pool->obj_list == NULL);
	if (chunk_num_obj)
	{
		void *chunk, *p, *last;
		if (!pool->clist || pool->clist->h.num >= NUM_PTRS)
		{
			struct _pool_clist *clist;

			if (pool->chunk_alloc)
				clist = (struct _pool_clist *)pool->chunk_alloc(pool->chunk_allocator, sizeof(clist[0]));
			else	
				clist = (struct _pool_clist *)malloc(sizeof(clist[0]));
			if (clist == NULL)
				return NULL;

			pool->mem_total += ROUNDUP_TO_PAGESIZE(sizeof(clist[0]));
			clist->h.next = pool->clist;
			clist->h.num = 0;
			pool->clist = clist;
		}

		if (pool->chunk_alloc)
			chunk = pool->chunk_alloc(pool->chunk_allocator, chunk_num_obj * obj_size);
		else
			chunk = malloc(chunk_num_obj * obj_size);
		if (chunk == NULL)
			return NULL;

		pool->mem_total += ROUNDUP_TO_PAGESIZE(chunk_num_obj * obj_size);
		pool->clist->chunks[pool->clist->h.num++] = chunk;

                last = (char *)chunk + (chunk_num_obj - 1)*obj_size;
                for (p = chunk; p < last; p = (char *)p + obj_size)
                {
                        ((struct _pool_obj *)p)->next = (struct _pool_obj *)((char *)p + obj_size);
                }
                ((struct _pool_obj *)last)->next = NULL;
		pool->obj_list = (struct _pool_obj *)chunk;
		pool->num_total += chunk_num_obj;
	}
	else
	{
		struct _pool_obj *obj = NULL;

		assert(pool->clist == NULL);
		if (pool->chunk_alloc)
			obj = (struct _pool_obj *)pool->chunk_alloc(pool->chunk_allocator, obj_size);
		else
			obj = (struct _pool_obj *)malloc(obj_size);
		if (obj == NULL)
			return NULL;

		pool->mem_total += ROUNDUP_TO_PAGESIZE(obj_size);
		obj->next = pool->obj_list;
		pool->obj_list = obj;
		pool->num_total += 1;
	}
	return pool->obj_list;
}

inline void *obpool_acquire(obpool_t *pool)
{
	struct _pool_obj *obj = pool->obj_list;

	if (obj == NULL)
	{
		if (pool->num_total >= pool->num_limit)
			return NULL;

		if ((obj = grow(pool)) == NULL)
			return NULL;
	}

	pool->num_acquire++;
	pool->obj_list = pool->obj_list->next;
	return obj;
}

void *obpool_acquire_zero(obpool_t *pool)
{
	void *p = obpool_acquire(pool);
	if (p)
		memset(p, 0, pool->obj_size);
	return p;
}

inline void obpool_release(obpool_t *pool, void *p)
{
	struct _pool_obj *obj = (struct _pool_obj *)p;

	if (obj)
	{
		obj->next = pool->obj_list;
		pool->obj_list = obj;
		pool->num_acquire--;
	}
}

inline void *obpool_mt_acquire(obpool_t *pool)
{
	void *p;
	pthread_mutex_lock(&_mx);
	p = obpool_acquire(pool);
	pthread_mutex_unlock(&_mx);
	return p;
}

void *obpool_mt_acquire_zero(obpool_t *pool)
{
	void *p = obpool_mt_acquire(pool);
	if (p)
		memset(p, 0, pool->obj_size);
	return p;
}

void obpool_mt_release(obpool_t *pool, void *obj)
{
	if (obj)
	{
		pthread_mutex_lock(&_mx);
		obpool_release(pool, obj);
		pthread_mutex_unlock(&_mx);
	}
}


#ifdef TEST_OBPOOL

int main(int argc, char **argv)
{
	int i;
	obpool_t _pool, *p = &_pool;

	obpool_init(p, 33);
	for (i = 0; i < 1024*1024; ++i)
		obpool_acquire(p);
		
	obpool_finish(p);
	return 0;
}

#endif

