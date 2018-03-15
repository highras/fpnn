#ifndef CACHED_CHAIN_BUFFER_H
#define CACHED_CHAIN_BUFFER_H

#include "IChainBuffer.h"
#include "MemoryPool.h"
#include "ObjectPool.h"

namespace fpnn {

class CachedChainBuffer
{
	struct ChainBlock
	{
		ChainBlock *_next;
		int _length;
		unsigned char *_buf;

		ChainBlock(void* buffer): _next(0), _length(0)
		{
			_buf = (unsigned char*)buffer - sizeof(struct ChainBlock);
		}
	};
	
public:
	class ChainBuffer: public IChainBuffer
	{
	private:
		ChainBlock *_first;
		ChainBlock *_current;
		
		int _length;
		int _capacity;
		int _blockCount;
		
		MemoryPool* _mp;
		
		ChainBlock* append(); 
		
	public:
		ChainBuffer(int capacity, MemoryPool* memoryPool);
		virtual ~ChainBuffer();
		
		virtual int length() const { return _length; }
		virtual void append(const void *data, int length);	//--  append into chainBuffer
		virtual int writeTo(void *buffer, int length, int offset);	//-- write to out buffer from chainBuffer
		virtual struct iovec* getIOVec(int &count);  //-- use free() to release.

		virtual int readfd(int fd, int length); 	//-- read fd and write into chainbuffer.
		virtual int writefd(int fd, int length, int offset);	//-- write to fd from chinaBuffer.

		virtual int fread(FILE* stream, int length); 	//-- read stream and write into chainbuffer.
		virtual int fwrite(FILE* stream, int length, int offset);	//-- write to stream from chinaBuffer.

		//-- New interface
		virtual int chunkCount() { return _blockCount; }
		virtual int chunkSize() { return _capacity; }
		virtual void* chunkBuf(int index, int& data_length);
		virtual void* header(int require_length);

		virtual int find(const char c, int start_pos = 0, int* chunckIndex = NULL, int* chunkBufferOffset = NULL);
		virtual bool memcmp(const void* target, int len, int start_pos = 0);

		virtual int getLines(std::vector<std::string>& lines, int offset = 0);
	};
	
private:
	int _capacity;
	MemoryPool _mp;
	ObjectPool<ChainBuffer> _op;

public:
	CachedChainBuffer(int capacity = 512): _capacity(capacity) {}
	~CachedChainBuffer()
	{
		release();
	}
	
	inline bool initMemoryPool(int32_t initBlocks, int32_t perAppendBlocks, int32_t perfectBlocks, int32_t maxBlocks)
	{
		return _mp.init(_capacity + sizeof(ChainBlock), initBlocks, perAppendBlocks, perfectBlocks, maxBlocks);
	}
	inline bool initObjectPool(int32_t initBlocks, int32_t perAppendBlocks, int32_t perfectBlocks, int32_t maxBlocks)
	{
		return _op.init(initBlocks, perAppendBlocks, perfectBlocks, maxBlocks);
	}
	
	inline ChainBuffer* createChainBuffer()
	{
		return _op.gain(_capacity, &_mp);
	}
	inline void recycle(ChainBuffer*& chainBuffer)
	{
		_op.recycle(chainBuffer);
	}
	
	inline void release()
	{
		_op.release();
		_mp.release();
	}
};

}

#endif
