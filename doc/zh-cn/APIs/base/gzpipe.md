## gzPipe

### 介绍

Gzip 压缩解压缩处理模块。

### 命名空间

	namespace fpnn::gzPipe;

### 全局函数

#### compress

	std::string compress(const std::string& data);
	std::string compress(const void* data, size_t size);

压缩数据并返回。  
遇到错误时，会抛出异常。


#### decompress

	std::string decompress(const std::string& data);
	std::string decompress(const void* data, size_t size);

解压数据并返回。  
遇到错误时，会抛出异常。