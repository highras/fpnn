#ifndef FPNN_File_System_Utility_h
#define FPNN_File_System_Utility_h

#include <string>
#include <vector>

namespace fpnn
{
	namespace FileSystemUtil
	{
		struct FileAttrs{
			std::string name;
			std::string sign;
			std::string content;
			std::string ext;
			int64_t size;
			int64_t atime;
			int64_t mtime;
			int64_t ctime;
		};

		bool fetchFileContentInLines(const std::string& filename, std::vector<std::string>& lines, bool ignoreEmptyLine = true, bool trimLine = true);
		bool readFileContent(const std::string& file, std::string& content);
		bool saveFileContent(const std::string& file, const std::string& content);
		bool appendFileContent(const std::string& file, const std::string& content);
		
		bool readFileAttrs(const std::string& file, FileAttrs& attrs);
		bool setFileAttrs(const std::string& file, const FileAttrs& attrs);

		bool getFileNameAndExt(const std::string& file, std::string& name, std::string& ext);

		bool readFileAndAttrs(const std::string& file, FileAttrs& attrs);


		bool createDirectory(const char* path);
		bool createDirectory(const std::string& path);
		bool createDirectories(const char* path);
		bool createDirectories(const std::string& path);
		std::string getSelfExectuedFilePath();
		
		/* Only normal file, or symbolic which point a normal file, or sub-directories when excludeSubDirectories == false. */
		std::vector<std::string> getFilesInDirectory(const char* directoryPath, bool excludeSubDirectories = true);
		std::vector<std::string> getFilesInDirectory(const std::string& directoryPath, bool excludeSubDirectories = true);

		std::vector<std::string> getSubDirectories(const char* directoryPath);
		std::vector<std::string> getSubDirectories(const std::string& directoryPath);

		//get all file in this directory and all sub directories
		std::vector<std::string> getFilesInDirectories(const char* directoryPath);
		std::vector<std::string> getFilesInDirectories(const std::string directoryPath);

		//find files in this directory and all sub directories
		std::vector<std::string> findFilesInDirectories(const char* directoryPath, const char* name);
		std::vector<std::string> findFilesInDirectories(const std::string& directoryPath, const std::string& name);

		//delete all file in this directory and the directory if this directory is empty
		bool deleteFilesInDirectory(const char* directoryPath);
		bool deleteFilesInDirectory(const std::string& directoryPath);

		//delelte all file in this directory and all files in subdirectories
		bool deleteFilesInDirectories(const char* directoryPath);
		bool deleteFilesInDirectories(const std::string& directoryPath);
	}

	class FileLocker
	{
		int _fd;
		char *_lock_file;
		
		bool canRelock(int fd);
		bool lock(bool may_relock);
		
	public:
		FileLocker(const char *lock_file);
		~FileLocker();
		
		bool locked() { return (_fd != 0); }
	};
}

#endif
