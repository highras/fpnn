## ChainBuffer

### 介绍

链式缓存 [IChainBuffer](IChainBuffer.md) 的具体实现。 

### 命名空间

	namespace fpnn;

### 关键定义

	class ChainBuffer: public IChainBuffer
	{
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

### 构造函数

	ChainBuffer(int capacity = 512);

**参数说明**

* **`int capacity`**

	单块缓存的缓存大小。

### 成员函数
 
所有成员函数，请参考 [IChainBuffer](IChainBuffer.md)。