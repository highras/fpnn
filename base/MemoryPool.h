#ifndef FPNN_Memory_Pool_H
#define FPNN_Memory_Pool_H

/*===============================================================================
  INCLUDES AND VARIABLE DEFINITIONS
  =============================================================================== */
#include <mutex>
#include "IMemoryPool.h"
namespace fpnn {

/*===============================================================================
  CLASS & STRUCTURE DEFINITIONS
  =============================================================================== */
class MemoryPool: public IMemoryPool
{
	private:
		std::mutex _mutex;

		MemoryData*		_recycleLink;
		MemoryData*		_activeLink;
		MemoryData*		_activeOverdraftLink;

		int32_t				_appendCount;			//-- The number of added blocks in each appending operation.
		int32_t				_perfectCount;
		int32_t				_maxCount;

		int32_t				_usedCount;			//-- Not include the Overdraft nodes.
		int32_t				_freeCount;
		int32_t				_overdraftCount;
		int32_t				_totalCount;

		bool				_inited;
		size_t				_blockSize;

	private:
		MemoryData* GainNode();

		void RecycleNode(MemoryData* memoryData);

		void RecycleOverdraftNode(MemoryData* memoryData);

		bool AppendNode(int32_t needBlocks);

		MemoryData* AppendOverdraftNode();

		bool Append(int32_t needBlocks, MemoryData** ppOverdraftNode);

		void	ReviseDataRelation(int32_t initCount);
		void	clean();

		bool createPool(int32_t initCount);		//-- Create memory pool.

	protected:
		inline MemoryData* getMemoryDataPtr(void* memory)
		{
			const size_t mdlen = sizeof(struct MemoryData);
			if (!memory || memory < (void *)mdlen)
				return NULL;

			unsigned char* buf = (unsigned char*)memory;
			return (MemoryData*)(buf - mdlen);
		}
		inline void* getMemoryPtr(MemoryData* data)
		{
			if (!data)
				return NULL;

			const size_t mdlen = sizeof(struct MemoryData);
			unsigned char* buf = (unsigned char*)data;
			return buf + mdlen;
		}

	public:
		MemoryPool(): _recycleLink(0), _activeLink(0), _activeOverdraftLink(0),
		_appendCount(10), _perfectCount(200), _maxCount(1000),
		_usedCount(0), _freeCount(0), _overdraftCount(0), _totalCount(0),
		_inited(false), _blockSize(256)
	{}

		~MemoryPool()
		{
			clean();
		}

		virtual bool init(size_t blockSize, int32_t initBlocks, int32_t perAppendBlocks, int32_t perfectBlocks, int32_t maxBlocks);
		virtual void release();

		virtual void* gain();
		virtual void recycle(void** memory);
		virtual void recycle(void* & memory);

		virtual bool inited() { return _inited; }
		virtual void status(struct MemoryPoolStatus& mps);
};

}

#endif
