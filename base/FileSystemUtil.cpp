#include <fstream>
#include <iostream>
#include <queue>
#include <utime.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <stddef.h>	//-- offsetof macro.
#include <fcntl.h>	//-- fileLocker
#include <errno.h>
#include <sys/file.h>	//-- fileLocker
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

bool FileSystemUtil::appendFileContent(const std::string& file, const std::string& content){
	std::ofstream out(file, std::ofstream::binary | std::ofstream::app);
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

bool FileSystemUtil::createDirectory(const std::string& path){
	return createDirectory(path.c_str());
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

bool FileSystemUtil::createDirectories(const std::string& path){
	return createDirectories(path.c_str());
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

std::vector<std::string> FileSystemUtil::getFilesInDirectory(const std::string& directoryPath, bool excludeSubDirectories){
	return getFilesInDirectory(directoryPath.c_str(), excludeSubDirectories);
}

std::vector<std::string> FileSystemUtil::getFilesInDirectory(const char* directoryPath, bool excludeSubDirectories)
{
	std::vector<std::string> files;
	if(directoryPath == NULL) return files;
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

		std::string path(directoryPath);
		path.append("/").append(entry->d_name);
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

std::vector<std::string> FileSystemUtil::getFilesInDirectories(const std::string directoryPath){
	return getFilesInDirectories(directoryPath.c_str());
}

std::vector<std::string> FileSystemUtil::getFilesInDirectories(const char* directoryPath)
{
	std::vector<std::string> files;
	if(directoryPath == NULL) return files;

	std::queue<std::string> pathes;
	pathes.push(directoryPath);

	while(!pathes.empty()){
		std::string path = pathes.front();
		pathes.pop();

		DIR *dir = opendir(path.c_str());
		if (dir == NULL) continue;

		long name_max = pathconf(path.c_str(), _PC_NAME_MAX);
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

			std::string pathlink(path);
			pathlink.append("/").append(entry->d_name);
			if (entry->d_type == DT_REG)
			{
				files.push_back(pathlink);
			}
			else if (entry->d_type == DT_DIR)
			{
				pathes.push(pathlink);
			}
			else if (entry->d_type == DT_LNK || entry->d_type == DT_UNKNOWN)
			{

				struct stat statBuf;
				if(stat(pathlink.c_str(), &statBuf) == 0)
				{
					if (S_ISREG(statBuf.st_mode))
						files.push_back(pathlink);
					else if (S_ISDIR(statBuf.st_mode)){
						pathes.push(pathlink);
					}
				}
			}
		}

		closedir(dir);
	}
	return files;
}

std::vector<std::string> FileSystemUtil::findFilesInDirectories(const std::string& directoryPath, const std::string& name){
	return findFilesInDirectories(directoryPath.c_str(), name.c_str());
}

std::vector<std::string> FileSystemUtil::findFilesInDirectories(const char* directoryPath, const char* name)
{
	std::vector<std::string> files;
	if(directoryPath == NULL || name == NULL) return files;

	std::queue<std::string> pathes;
	pathes.push(directoryPath);

	while(!pathes.empty()){
		std::string path = pathes.front();
		pathes.pop();

		DIR *dir = opendir(path.c_str());
		if (dir == NULL) continue;

		long name_max = pathconf(path.c_str(), _PC_NAME_MAX);
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

			std::string pathlink(path);
			pathlink.append("/").append(entry->d_name);
			if (entry->d_type == DT_REG)
			{
				if(strstr(entry->d_name, name) != NULL)
					files.push_back(pathlink);
			}
			else if (entry->d_type == DT_DIR)
			{
				pathes.push(pathlink);
			}
			else if (entry->d_type == DT_LNK || entry->d_type == DT_UNKNOWN)
			{

				struct stat statBuf;
				if(stat(pathlink.c_str(), &statBuf) == 0)
				{
					if (S_ISREG(statBuf.st_mode)){
						if(strstr(entry->d_name, name) != NULL)
							files.push_back(pathlink);
					}
					else if (S_ISDIR(statBuf.st_mode)){
						pathes.push(pathlink);
					}
				}
			}
		}

		closedir(dir);
	}
	return files;
}

bool FileSystemUtil::deleteFilesInDirectories(const std::string& directoryPath){
	return deleteFilesInDirectories(directoryPath.c_str());
}

bool FileSystemUtil::deleteFilesInDirectories(const char* directoryPath)
{
	if(directoryPath == NULL) return false;
	DIR *dir = opendir(directoryPath);
	if (dir == NULL) return false;

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

		std::string pathlink(directoryPath);
		pathlink.append("/").append(entry->d_name);
		if (entry->d_type == DT_REG)
		{
			unlink(pathlink.c_str());
		}
		else if (entry->d_type == DT_DIR)
		{
			deleteFilesInDirectories(pathlink.c_str());
		}
		else if (entry->d_type == DT_LNK || entry->d_type == DT_UNKNOWN)
		{
			struct stat statBuf;
			if(stat(pathlink.c_str(), &statBuf) == 0)
			{
				if (S_ISREG(statBuf.st_mode))
					unlink(pathlink.c_str());
				else if (S_ISDIR(statBuf.st_mode)){
					deleteFilesInDirectories(pathlink.c_str());
				}
			}

			unlink(pathlink.c_str());
		}

	}
	closedir(dir);
	rmdir(directoryPath);
	return true;
}

bool FileSystemUtil::deleteFilesInDirectory(const std::string& directoryPath){
	return deleteFilesInDirectory(directoryPath.c_str());
}

bool FileSystemUtil::deleteFilesInDirectory(const char* directoryPath)
{
	if(directoryPath == NULL) return false;
	DIR *dir = opendir(directoryPath);
	if (dir == NULL)
		return false;

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

		std::string pathlink(directoryPath);
		pathlink.append("/").append(entry->d_name);
		if (entry->d_type == DT_REG)
		{
			unlink(pathlink.c_str());
		}
		else if (entry->d_type == DT_LNK || entry->d_type == DT_UNKNOWN)
		{
			struct stat statBuf;
			if(stat(pathlink.c_str(), &statBuf) == 0)
			{
				if (S_ISREG(statBuf.st_mode))
					unlink(pathlink.c_str());
			}
		}
	}

	closedir(dir);
	rmdir(directoryPath);
	return true;;
}

FileLocker::FileLocker(const char *lock_file): _fd(0)
{
	_lock_file = strdup(lock_file);
	if (_lock_file)
		lock(true);
}

FileLocker::~FileLocker()
{
	if (_fd)
	{
		close(_fd);
		unlink(_lock_file);
		free(_lock_file);
	}
}

bool FileLocker::canRelock(int fd)
{
	struct flock finfo;
	if (fcntl(fd, F_GETLK, &finfo) == -1)
		return false;
		
	int pid = (int)finfo.l_pid;
	char buf[32];
	sprintf(buf, "/proc/%d", pid);
	
	struct stat dir_stat;
	if (stat(buf, &dir_stat) == -1)
	{
		if (errno != EACCES)
			return true;
	}
	return false;
}

bool FileLocker::lock(bool may_relock)
{
	_fd = open(_lock_file, O_CREAT|O_WRONLY|O_TRUNC, S_IRWXU);
	if (_fd == -1)
	{
		_fd = 0;
		free(_lock_file);
		return false;
	}
	
	if (flock(_fd, LOCK_EX|LOCK_NB) == -1)
	{
		bool relock = may_relock ? canRelock(_fd) : false;
		close(_fd);
		
		if (relock)
		{
			unlink(_lock_file);
			return lock(false);
		}
		
		_fd = 0;
		free(_lock_file);
		return false;
	}
	
	return true;
}
