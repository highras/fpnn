#ifndef UNIXFS_H_
#define UNIXFS_H_

#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif


/* Recursively create directory, return 0 if success.
   1 if directory creation successful but chown() failed .
  -1 if error.
 */ 
int unixfs_mkdir(const char *pathname, mode_t mode, uid_t owner, gid_t group);


/* Same as open(2), except unixfs_open() may create the non-existing 
   directory component in the pathname if flags include O_CREAT.
 */
int unixfs_open(const char *pathname, int flags, mode_t mode);


/* Read the file content to *p_content and return the length.
   *p_content should be a malloc()ed buffer or NULL,
   *n should be the size of *p_content if *p_content is malloc()ed.
   A trailing '\0' is appended to the *p_content.
   If the file size greater than size of *p_content, *p_content is realloc()ed
   and *n is set to the new size. 
   The caller should call free() to free the *p_content after usage.
 */
ssize_t unixfs_get_content(const char *pathname, char **p_content, size_t *n);


ssize_t unixfs_put_content(const char *pathname, const void *content, size_t size);


#ifdef __cplusplus
}
#endif

#endif
