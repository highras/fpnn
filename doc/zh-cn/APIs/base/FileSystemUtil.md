## FileSystemUtil

### 介绍

文件系统辅助工具集合。

### 命名空间

	namespace fpnn;

[FileLocker](#FileLocker) 所处命名空间。

	namespace fpnn::FileSystemUtil;

其余 API 所处命名空间。

### FileAttrs

文件及属性对象。

	namespace FileSystemUtil
	{
		struct FileAttrs{
			std::string name;
			std::string sign;
			std::string content;
			std::string ext;
			int32_t size;
			int32_t atime;
			int32_t mtime;
			int32_t ctime;
		};
	}

**成员说明**

+ `name`

	文件名。

+ `sign`

	以大写16进制表示的文件 md5 值。

+ `content`

	文件内容。

+ `ext`

	文件扩展名。

+ `size`

	文件大小。

+ `atime`

	文件访问时间，

+ `mtime`

	文件修改时间。

+ `ctime`

	文件创建时间。

### 全局函数

#### fetchFileContentInLines

	bool fetchFileContentInLines(const std::string& filename, std::vector<std::string>& lines, bool ignoreEmptyLine = true, bool trimLine = true);

读取 `const std::string& filename` 参数指定的文件内容，并按行存储到 `std::vector<std::string>& lines` 参数中。

#### readFileContent

	bool readFileContent(const std::string& file, std::string& content);

读取 `const std::string& file` 参数指定的文件内容到 `std::string& content` 参数中。

#### saveFileContent

	bool saveFileContent(const std::string& file, const std::string& content);

将 `const std::string& content` 参数中的内容，存储到 `const std::string& file` 参数指定的文件中。如果文件已存在，则覆盖原始文件。

#### appendFileContent

	bool appendFileContent(const std::string& file, const std::string& content);

将 `const std::string& content` 参数中的内容，追加存储到 `const std::string& file` 参数指定的文件中。

#### readFileAttrs

	bool readFileAttrs(const std::string& file, FileAttrs& attrs);

读取 `const std::string& file` 参数指定的文件**属性**到 `FileAttrs& attrs` 中（仅属性被读取，**不含文件内容**）。

#### setFileAttrs

	bool setFileAttrs(const std::string& file, const FileAttrs& attrs);

通过 `FileAttrs& attrs` 设置 `const std::string& file` 参数指定的文件属性。

#### getFileNameAndExt

	bool getFileNameAndExt(const std::string& file, std::string& name, std::string& ext);

将 `const std::string& file` 指定的文件路径名，拆解为文件名和文件扩展名，并存储到 `std::string& name` 参数和 `std::string& ext` 参数中。

#### readFileAndAttrs

	bool readFileAndAttrs(const std::string& file, FileAttrs& attrs);

读取 `const std::string& file` 参数指定的文件**内容和属性**到 `FileAttrs& attrs` 中。

#### createDirectory

	bool createDirectory(const char* path);
	bool createDirectory(const std::string& path);

创建单级目录/最末级目录。  
如果中间级目录不存在，将返回失败。

#### createDirectories

	bool createDirectories(const char* path);
	bool createDirectories(const std::string& path);

创建目录。如果中间级目录不存在，则将连带创建。

#### getSelfExectuedFilePath

	std::string getSelfExectuedFilePath();

获取进程自身所在目录绝对路径。

#### getFilesInDirectory

	std::vector<std::string> getFilesInDirectory(const char* directoryPath, bool excludeSubDirectories = true);
	std::vector<std::string> getFilesInDirectory(const std::string& directoryPath, bool excludeSubDirectories = true);

获取指定目录下的文件。仅包含**普通文件**，和指向**普通文件**的符号连接。当 `excludeSubDirectories` 为 false 时，还将包含次级目录。

#### getFilesInDirectories

	std::vector<std::string> getFilesInDirectories(const char* directoryPath);
	std::vector<std::string> getFilesInDirectories(const std::string directoryPath);

获取指定目录及其下所有子目录中的文件。仅包含**普通文件**，和指向**普通文件**的符号连接。

#### findFilesInDirectories

	std::vector<std::string> findFilesInDirectories(const char* directoryPath, const char* name);
	std::vector<std::string> findFilesInDirectories(const std::string& directoryPath, const std::string& name);

在指定目录及其下所有子目录中，查找名称包含 `name` 字符子串的文件，并返回。返回的文件包含以 `directoryPath` 为前导路径的相对路径。

#### deleteFilesInDirectory

	bool deleteFilesInDirectory(const char* directoryPath);
	bool deleteFilesInDirectory(const std::string& directoryPath);

删除指定目录下所有文件。如果指定目录不含子目录，同时删除指定目录。

#### deleteFilesInDirectories

	bool deleteFilesInDirectories(const char* directoryPath);
	bool deleteFilesInDirectories(const std::string& directoryPath);

删除指定目录及其下所有文件和子目录。

### FileLocker

	class FileLocker
	{
	public:
		FileLocker(const char *lock_file);
		~FileLocker();
		
		bool locked();
	};

文件锁。

#### 构造函数

	FileLocker(const char *lock_file);

**参数说明**

* **`const char *lock_file`**

	锁文件路径。

#### locked

	bool locked();

判断是否锁成功。
