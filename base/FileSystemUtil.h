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
			int32_t size;
			int32_t atime;
			int32_t mtime;
			int32_t ctime;
		};

		bool fetchFileContentInLines(const std::string& filename, std::vector<std::string>& lines, bool ignoreEmptyLine = true, bool trimLine = true);
		bool readFileContent(const std::string& file, std::string& content);
		bool saveFileContent(const std::string& file, const std::string& content);
		
		bool readFileAttrs(const std::string& file, FileAttrs& attrs);
		bool setFileAttrs(const std::string& file, const FileAttrs& attrs);

		bool getFileNameAndExt(const std::string& file, std::string& name, std::string& ext);

		bool readFileAndAttrs(const std::string& file, FileAttrs& attrs);
	}
}

#endif
