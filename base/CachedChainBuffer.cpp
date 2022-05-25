#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "CachedChainBuffer.h"

using namespace fpnn;

//======================================//
//-           Chain Buffer             -//
//======================================//
CachedChainBuffer::ChainBuffer::ChainBuffer(int capacity, MemoryPool* memoryPool)
{
	_length = 0;
	_capacity = capacity;
	_blockCount = 1;
	_mp = memoryPool;
	
	_first = (ChainBlock*)_mp->gain();
	new(_first) ChainBlock(_first);
	_current = _first;
}

CachedChainBuffer::ChainBuffer::~ChainBuffer()
{
	do
	{
		_current = _first->_next;
		_mp->recycle((void**)&_first);
		
		_first = _current;
	} while (_first);
}

CachedChainBuffer::ChainBlock* CachedChainBuffer::ChainBuffer::append()
{
	ChainBlock* cb = (ChainBlock*)_mp->gain();
	new(cb) ChainBlock(cb);
	
	_current->_next = cb;
	_current = cb;
	_blockCount += 1;
	
	return cb;
}

void CachedChainBuffer::ChainBuffer::append(const void *data, int length)
{
	unsigned char* src = (unsigned char*)data;
	while (length > 0)
	{
		int remain = _capacity - _current->_length;
		if (remain)
		{
			int size = (remain > length) ? length : remain;
			memcpy(_current->_buf + _current->_length, src, size);
			length -= size;
			src += size;
			
			_length += size;
			_current->_length += size;
		}
		else
			append();
	}
}

int CachedChainBuffer::ChainBuffer::writeTo(void *buffer, int length, int offset)
{
	ChainBlock *cb = _first;
	while (offset >= _capacity)
	{
		if (!cb->_next)
			return 0;
			
		cb = cb->_next;
		offset -= _capacity;
	}
	
	int count = 0;
	unsigned char* out = (unsigned char*)buffer;
	
	while (length)
	{
		int remain = _capacity - offset;
		int size = (length > remain) ? remain : length;
		memcpy(out, cb->_buf + offset, size);
		length -= size;
		offset = 0;
		
		count += size;
		out += size;
		
		if (length)
		{
			if (cb->_next)
				cb = cb->_next;
			else
				break;
		}
	}
	return count;
}

struct iovec* CachedChainBuffer::ChainBuffer::getIOVec(int &length)
{
	struct iovec *vec = (struct iovec *)malloc(sizeof(struct iovec) * _blockCount);
	ChainBlock *cb = _first;
	for (int i = 0; i < _blockCount; i++)
	{
		vec[i].iov_base = cb->_buf;
		vec[i].iov_len = cb->_length;
		cb = cb->_next;
	}
	length = _blockCount;
	return vec;
}

int CachedChainBuffer::ChainBuffer::readfd(int fd, int length)
{
	int readed = 0;
	while (length > 0)
	{
		int remain = _capacity - _current->_length;
		if (remain)
		{
			int size = (remain > length) ? length : remain;
			int readBytes = (int)::read(fd, _current->_buf + _current->_length, size);
			if (readBytes == -1)
				return readed;

			readed += readBytes;
			length -= readBytes;
			
			_length += readBytes;
			_current->_length += readBytes;

			if (size != readBytes)
				return readed;
		}
		else
			append();
	}
	return readed;
}

int CachedChainBuffer::ChainBuffer::writefd(int fd, int length, int offset)
{
	ChainBlock *cb = _first;
	while (offset >= _capacity)
	{
		if (!cb->_next)
			return 0;
			
		cb = cb->_next;
		offset -= _capacity;
	}
	
	int count = 0;
	while (length)
	{
		int remain = _capacity - offset;
		int size = (length > remain) ? remain : length;
		int writeBytes = (int)::write(fd, cb->_buf + offset, size);
		if (writeBytes == -1)
			return count;

		if (writeBytes != size)
		{
			if (writeBytes > 0)
				count += writeBytes;

			return count;
		}

		length -= size;
		offset = 0;
		
		count += size;
		
		if (length)
		{
			if (cb->_next)
				cb = cb->_next;
			else
				break;
		}
	}
	return count;
}

int CachedChainBuffer::ChainBuffer::fread(FILE* stream, int length)
{
	int readed = 0;
	while (length > 0)
	{
		int remain = _capacity - _current->_length;
		if (remain)
		{
			int size = (remain > length) ? length : remain;
			int readBytes = (int)::fread(_current->_buf + _current->_length, 1, size, stream);
			if (readBytes <= 0)
				return readed;

			readed += readBytes;
			length -= readBytes;
			
			_length += readBytes;
			_current->_length += readBytes;

			if (size != readBytes)
				return readed;
		}
		else
			append();
	}
	return readed;
}

int CachedChainBuffer::ChainBuffer::fwrite(FILE* stream, int length, int offset)
{
	ChainBlock *cb = _first;
	while (offset >= _capacity)
	{
		if (!cb->_next)
			return 0;
			
		cb = cb->_next;
		offset -= _capacity;
	}
	
	int count = 0;
	while (length)
	{
		int remain = _capacity - offset;
		int size = (length > remain) ? remain : length;
		int writeBytes = (int)::fwrite(cb->_buf + offset, 1, size, stream);
		if (writeBytes <= 0)
			return count;

		if (writeBytes != size)
		{
			if (writeBytes > 0)
				count += writeBytes;

			return count;
		}

		length -= size;
		offset = 0;
		
		count += size;
		
		if (length)
		{
			if (cb->_next)
				cb = cb->_next;
			else
				break;
		}
	}
	return count;
}





void* CachedChainBuffer::ChainBuffer::chunkBuf(int index, int& data_length)
{
	if (index >= _blockCount)
	{
		data_length = 0;
		return NULL;
	}

	ChainBlock *cb = _first;
	while (index)
	{
		cb = cb->_next;
		index -= 1;
	}
	data_length = cb->_length;
	return cb->_buf;
}

void* CachedChainBuffer::ChainBuffer::header(int require_length)
{
	if (_first->_length < require_length)
		return NULL;

	return _first->_buf;
}

int CachedChainBuffer::ChainBuffer::find(const char c, int start_pos, int* chunckIndex, int* chunkBufferOffset)
{
	int cbIdx = 0;
	int cbOffset = 0;
	ChainBlock *cb = _first;

	if (start_pos >= _length)
		return -1;

	while (start_pos > _capacity)
	{
		start_pos -= _capacity;
		cbIdx += 1;
		cb = cb->_next;
	}

	cbOffset = start_pos;
	bool found = false;
	while (cb)
	{
		while (cbOffset < cb->_length)
		{
			if (cb->_buf[cbOffset] == (unsigned char)c)
			{
				found = true;
				break;
			}
			else
				cbOffset++;
		}

		if (found)
			break;
		else
		{
			cb = cb->_next;
			cbOffset = 0;
			cbIdx += 1;
		}
	}

	if (found)
	{
		if (chunckIndex)
			*chunckIndex = cbIdx;

		if (chunkBufferOffset)
			*chunkBufferOffset = cbOffset;

		return (cbIdx * _capacity + cbOffset);
	}
	else
		return -1;
}

bool CachedChainBuffer::ChainBuffer::memcmp(const void* target, int len, int start_pos)
{
	int cbOffset = 0;
	ChainBlock *cb = _first;

	if (len <= 0)
		return false;

	if (start_pos + len > _length)
		return false;

	while (start_pos > _capacity)
	{
		start_pos -= _capacity;
		cb = cb->_next;
	}

	cbOffset = start_pos;
	const unsigned char* mem = (const unsigned char*)target;
	for (int i = 0; i < len; i++, cbOffset++)
	{
		if (cbOffset >= cb->_length)
		{
			cb = cb->_next;
			cbOffset = 0;
		}
		
		if (mem[i] != cb->_buf[cbOffset])
			return false;
	}
	
	return true;
}

int CachedChainBuffer::ChainBuffer::getLines(std::vector<std::string>& lines, int offset)
{
	if (offset >= _length)
		return 0;

	ChainBlock *cb = _first;
	while (offset > _capacity)
	{
		offset -= _capacity;
		cb = cb->_next;
	}

	bool unclosure = false;
	while (cb)
	{
		int pos = offset;
		while (pos < cb->_length)
		{
			if (cb->_buf[pos] == '\n' || cb->_buf[pos] == '\r')
			{
				if (pos > offset)
				{
					if (unclosure)
					{
						lines[lines.size() - 1].append((char*)cb->_buf + offset, pos - offset);
						unclosure = false;
					}
					else
						lines.push_back(std::string((char*)cb->_buf + offset, pos - offset));
				}

				if (unclosure)
					unclosure = false;

				pos += 1;
				offset = pos;
				continue;
			}
			else
				pos += 1;
		}

		if (pos != offset)
		{
			if (unclosure)
				lines[lines.size() - 1].append((char*)cb->_buf + offset, pos - offset);
			else
			{
				lines.push_back(std::string((char*)cb->_buf + offset, pos - offset));
				unclosure = true;
			}
		}
		else
			unclosure = false;
		
		cb = cb->_next;
		offset = 0;
	}

	return (int)lines.size();
}
