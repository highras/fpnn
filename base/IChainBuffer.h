#ifndef CHAIN_BUFFER_Interface_H
#define CHAIN_BUFFER_Interface_H

#include <sys/uio.h>
#include <stdio.h>
#include <vector>
#include <string>
#include <memory>

namespace fpnn {
class IChainBuffer
{
public:
	virtual ~IChainBuffer() {}
	
	virtual int length() const = 0;
	virtual void append(const void *data, int length) = 0;	//--  append into chainBuffer
	virtual int writeTo(void *buffer, int length, int offset) = 0;		//-- write to out buffer from chainBuffer
	virtual struct iovec* getIOVec(int &count) = 0;  //-- use free() to release.

	virtual int readfd(int fd, int length) = 0; 	//-- read fd and write into chainbuffer.
	virtual int writefd(int fd, int length, int offset) = 0;	//-- write to fd from chinaBuffer.

	virtual int fread(FILE* stream, int length) = 0; 	//-- read stream and write into chainbuffer.
	virtual int fwrite(FILE* stream, int length, int offset) = 0;	//-- write to stream from chinaBuffer.

	//-- New interface
	virtual int chunkCount() = 0;
	virtual int chunkSize() = 0;
	virtual void* chunkBuf(int index, int& data_length) = 0;
	virtual void* header(int require_length) = 0;

	virtual int find(const char c, int start_pos = 0, int* chunckIndex = NULL, int* chunkBufferOffset = NULL) = 0;
	virtual bool memcmp(const void* target, int len, int start_pos = 0) = 0;

	virtual int getLines(std::vector<std::string>& lines, int offset = 0) = 0;
};
typedef std::shared_ptr<IChainBuffer> IChainBufferPtr;
}
#endif
