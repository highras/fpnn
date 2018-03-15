#ifndef CHAIN_BUFFER_H
#define CHAIN_BUFFER_H

#include "IChainBuffer.h"


namespace fpnn {

class ChainBuffer: public IChainBuffer
{
private:
	struct ChainBlock
	{
		ChainBlock *_next;
		int _length;
		unsigned char *_buf;

		ChainBlock(int capacity);
		~ChainBlock();
	};
	
	ChainBlock *_first;
	ChainBlock *_current;
	
	int _length;
	int _capacity;
	int _blockCount;
	
	ChainBlock* append(); 
	
public:
	ChainBuffer(int capacity = 512);
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
typedef std::shared_ptr<ChainBuffer> ChainBufferPtr;

}

#endif
