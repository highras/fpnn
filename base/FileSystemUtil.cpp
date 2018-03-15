#include <fstream>
#include <utime.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "StringUtil.h"
#include "md5.h"
#include "hex.h"
#include "FileSystemUtil.h"

using namespace fpnn;

bool FileSystemUtil::fetchFileContentInLines(const std::string& filename, std::vector<std::string>& lines, bool ignoreEmptyLine, bool trimLine)
{
	std::ifstream fin(filename.c_str()); 
	if (fin.is_open())
	{
		while(fin.good())
		{
			std::string line;
			std::getline(fin, line);

			if (trimLine)
				StringUtil::trim(line);
			
			if (ignoreEmptyLine && line.empty())
				continue;

			lines.push_back(line);
		}
		fin.close();
		return true;
	}
	return false;
}

bool FileSystemUtil::readFileContent(const std::string& file, std::string& content){
	std::ifstream in(file, std::ios::in);
	if (in.is_open()){
		std::istreambuf_iterator<char> beg(in), end;
		content = std::string(beg, end);
		in.close();
		return true;
	}
	return false;
}

bool FileSystemUtil::saveFileContent(const std::string& file, const std::string& content){
	std::ofstream out(file, std::ofstream::binary);
	if(out.is_open()){
		out.write(content.data(), content.size());
		out.close();
		return true;
	}
	return false;
}

bool FileSystemUtil::readFileAttrs(const std::string& file, FileSystemUtil::FileAttrs& attrs){
	struct stat buf;
	int result = stat(file.c_str(), &buf);
	if(result == 0){
		attrs.size = buf.st_size;
		attrs.atime = buf.st_atime;
		attrs.mtime = buf.st_mtime;
		attrs.ctime = buf.st_ctime;
		return true;
	}
	return false;
}

bool FileSystemUtil::setFileAttrs(const std::string& file, const FileSystemUtil::FileAttrs& attrs){
	struct utimbuf new_times;
	new_times.actime = attrs.atime;
	new_times.modtime = attrs.mtime;
	if(!utime(file.c_str(), &new_times))
		return true;
	return false;
}

bool FileSystemUtil::getFileNameAndExt(const std::string& file, std::string& name, std::string& ext){
	std::vector<std::string> elem;
	StringUtil::split(file, "/\\", elem);
	if(elem.size() > 0){
		name = elem[elem.size() - 1];
		std::vector<std::string> elem2;
		StringUtil::split(name, ".", elem2);
		if(elem2.size() > 1){
			ext = elem2[elem2.size() - 1];
		}
		return true;
	}
	return false;
}

bool FileSystemUtil::readFileAndAttrs(const std::string& file, FileAttrs& attrs){
	bool ret = readFileContent(file, attrs.content);
	if(!ret) return ret;
	
	ret = getFileNameAndExt(file, attrs.name, attrs.ext);
	if(!ret) return ret;
	ret = readFileAttrs(file, attrs);
	if(!ret) return ret;

	unsigned char digest[16];
	char hexstr[32 + 1];
	md5_checksum(digest, attrs.content.data(), attrs.content.size());
	Hexlify(hexstr, digest, sizeof(digest));
	attrs.sign = hexstr;

	return true;
}
