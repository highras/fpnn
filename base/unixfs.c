#include "unixfs.h"
#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <alloca.h>
#include <fcntl.h>
#include <limits.h>
#include <stdlib.h>

static int _mkdir(char *pathname, int len, mode_t mode, uid_t uid, gid_t gid)
{
	int r;

	assert(pathname[len-1] != '/');

	while (len > 0 && pathname[--len] != '/')
		continue;

	if (pathname[len] == '/')
	{
		pathname[len] = 0;

		r = access(pathname, F_OK);
		if (r == -1 && errno == ENOENT)
			r = _mkdir(pathname, len, mode, uid, gid);

		pathname[len] = '/';

		if (r < 0)
			return -1;
	}

	r = mkdir(pathname, mode);

	if (r == 0 || errno == EEXIST)
	{
		if (uid != (uid_t)-1 || gid != (gid_t)-1)
		{
			if (chown(pathname, uid, gid) == -1)
				return 1;
		}
		return 0;
	}

	return -1;
}

int unixfs_mkdir(const char *pathname, mode_t mode, uid_t uid, gid_t gid)
{
	int len;
	int r;

	if (pathname[0] == 0)
		return -1;

	r = mkdir(pathname, mode);

	if (r == 0 || errno == EEXIST)
	{
		if (uid != (uid_t)-1 || gid != (gid_t)-1)
		{
			if (chown(pathname, uid, gid) == -1)
				return 1;
		}
		return 0;
	}

	if (errno != ENOENT)
		return -1;

	len = strlen(pathname);
	if (len > 0 && pathname[len - 1] == '/')
		--len;

	if (len > 0)
	{
		char *path = (char *)malloc(len + 1);
		memcpy(path, pathname, len);
		path[len] = 0;
		r = _mkdir(path, len, mode, uid, gid);
		free(path);
	}

	return r;
}

int unixfs_open(const char *pathname, int flags, mode_t mode)
{
	int fd = open(pathname, flags, mode);
	if (fd == -1 && (flags & O_CREAT) && errno == ENOENT)
	{
		char *p = strrchr(pathname, '/');
		if (p && p > pathname)
		{
			int rc;
			int len = p - pathname;
			char *path = (char *)malloc(len + 1);
			memcpy(path, pathname, len);
			path[len] = 0;
			rc = _mkdir(path, len, 0775, -1, -1);
			free(path);

			if (rc == 0)
			{
				fd = open(pathname, flags, mode);
			}
		}
	}
	return fd;
}

ssize_t unixfs_get_content(const char *pathname, char **p_content, size_t *p_size)
{
	ssize_t r = -1;
	ssize_t fsize = -1;
	char *content = *p_content;
	ssize_t size = *p_size;
	int fd = open(pathname, O_RDONLY);

	if (fd == -1)
		return -1;

	if (content == NULL || size == 0)
	{
		struct stat st;
		fsize = (fstat(fd, &st) == 0) ? st.st_size : 0;
		size = fsize < 256 ? 256 : fsize + 1;
		content = (char *)realloc(content, size);
		if (!content)
			goto error;

		*p_content = content;
		*p_size = size;
	}

	if (fsize > 0)
	{
		r = read(fd, content, fsize);
	}
	else /* if fsize == 0, the file may contain content too, for example /proc/cpuinfo */
	{
		r = read(fd, content, size);
		if (r == size)
		{
			ssize_t delta;
			ssize_t m;

			if (fsize == -1)
			{
				struct stat st;
				fsize = (fstat(fd, &st) == 0) ? st.st_size : 0;
			}

			do {
				if (fsize + 1 > size)
					delta = fsize + 1 - size;
				else
					delta = size < 65536 ? size : 65536;

				size = r + delta;
				content = (char *)realloc(content, size);
				if (!content)
					goto error;

				*p_content = content;
				*p_size = size;

				m = read(fd, content + r, delta);
				if (m < 0)
					goto error;
				r += m;
			} while (m == delta);
		}
	}

	if (r >= 0)
		content[r] = 0;
	close(fd);
	return r;
error:
	close(fd);
	return -1;
}

ssize_t unixfs_put_content(const char *pathname, const void *content, size_t size)
{
	int fd;
	ssize_t n;

	if (size <= 0)
		return -1;

	fd = open(pathname, O_WRONLY | O_CREAT, 0664);
	if (fd == -1)
		return -1;

	n = write(fd, content, size);
	close(fd);
	return n;
}

#ifdef TEST_UNIXFS

int main()
{
	int r;
	ssize_t n;
	char *buf = NULL;
	size_t size = 0;
	n = unixfs_get_content("/proc/cpuinfo", &buf, &size);
	printf("n=%zd\n", n);
	r = unixfs_mkdir("../../../../../../../../../../a/b/c/d/e/", 0777, -1, -1);
	printf("r=%d\n", r);
	return 0;
}

#endif
