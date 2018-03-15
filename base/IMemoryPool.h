#ifndef FPNN_Memory_Pool_Interface_H
#define FPNN_Memory_Pool_Interface_H

#include <stdint.h>
#include <stdlib.h>
#include <utility>
namespace fpnn {
/*===============================================================================
  CLASS & STRUCTURE DEFINITIONS
  =============================================================================== */
struct MemoryPoolStatus
{
	int32_t				usedCount;		//-- Not include the Overdraft nodes.
	int32_t				freeCount;
	int32_t				overdraftCount;
	int32_t				totalUsedCount;
	int32_t				totalCount;

	MemoryPoolStatus(): usedCount(0), freeCount(0), overdraftCount(0), totalUsedCount(0), totalCount(0) {}
};

class IMemoryPool
{
	protected:
		inline size_t roundedSize(size_t orginialSize)
		{
			const size_t ptrlen = sizeof(void*);
			size_t rest = orginialSize % ptrlen;
			if (rest == 0)
				return orginialSize;
			else
				return orginialSize - rest + ptrlen;
		}

	public:
		virtual				~IMemoryPool() {}

		virtual bool		init(size_t blockSize, int32_t initBlocks, int32_t perAppendBlocks, int32_t perfectBlocks, int32_t maxBlocks) = 0;
		virtual void		release() = 0;

		virtual void*		gain() = 0;
		virtual void		recycle(void**) = 0;
		virtual void		recycle(void*&) = 0;

		virtual bool		inited() = 0;
		virtual void		status(struct MemoryPoolStatus&) = 0;
};

struct MemoryData
{
	struct MemoryData		*Pre;
	struct MemoryData		*Next;
	//bool					bOverdraft;					//-- The compilers will align data types on a natural boundary.
	void*					bOverdraft;					//Using pointer type rather than bool type just for memory alignment. NULL <=> false, !NULL <=> true.
};
}
#endif
