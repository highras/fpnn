#include <fstream>
#include <utime.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <stddef.h>	//-- offsetof macro.
#include "AutoRelease.h"
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

std::string FileSystemUtil::getSelfExectuedFilePath()
{
	const size_t BufferSize = 1024;
	char pathBuffer[BufferSize];
	
	if (readlink("/proc/self/exe", pathBuffer, BufferSize) == -1)
		return "";

	char *pointer = strrchr(pathBuffer, '/');
	if (pointer)
	{
		if (pointer != pathBuffer)
			return std::string(pathBuffer, (size_t)(pointer - pathBuffer) + 1);
		else
			return "/";
	}

	return "";
}

bool FileSystemUtil::createDirectory(const char* path)
{
	if (access(path, F_OK /*| W_OK*/) != 0)		//-- Create the folder which don't exist.
	{
		if (mkdir(path, S_IRWXU | S_IRGRP | S_IROTH/*0700*/) == -1)
			return false;
	}
	return true;
}

bool FileSystemUtil::createDirectories(const char* path)
{
	if (path == NULL)
		return false;

	char *start, *end;
	StringUtil::softTrim(path, start, end);

	if (start == NULL || start == end)
		return false;

	char *buffer = (char *)malloc(end - start + 1);
	AutoFreeGuard afg(buffer);

	char *dst = buffer;
	char *src = start;

	while (src != end)
	{
		*dst++ = *src++;

		if (*(src - 1) == '/')
		{
			*dst = '\0';
			if (createDirectory(buffer) == false)
				return false;
		}
	}

	*dst = '\0';
	return createDirectory(buffer);
}

std::vector<std::string> FileSystemUtil::getFilesInDirectory(const char* directoryPath, bool excludeSubDirectories)
{
	std::vector<std::string> files;
	DIR *dir = opendir(directoryPath);
	if (dir == NULL)
		return files;

	long name_max = pathconf(directoryPath, _PC_NAME_MAX);
	if (name_max == -1)         /* Limit not defined, or error */
		name_max = 255;         /* Take a guess */
	long len = offsetof(struct dirent, d_name) + name_max + 1;
	struct dirent *entry = (struct dirent *)malloc(len);

	AutoFreeGuard afg(entry);

	while (true)
	{
		struct dirent *result;
		if (readdir_r(dir, entry, &result) || !result)
			break;

		if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
			continue;

		if (entry->d_type == DT_REG)
		{
			files.push_back(entry->d_name);
		}
		else if (entry->d_type == DT_DIR && !excludeSubDirectories)
		{
			files.push_back(entry->d_name);
		}
		else if (entry->d_type == DT_LNK || entry->d_type == DT_UNKNOWN)
		{
			std::string path(directoryPath);
			path.append("/").append(entry->d_name);

			struct stat statBuf;
			if(stat(path.c_str(), &statBuf) == 0)
			{
				if (S_ISREG(statBuf.st_mode))
					files.push_back(entry->d_name);
				else if (S_ISDIR(statBuf.st_mode) && !excludeSubDirectories)
					files.push_back(entry->d_name);
			}
		}
	}

	closedir(dir);
	return files;
}
