#ifndef unix_user_h_
#define unix_user_h_

#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif


int unix_uid2user(uid_t uid, char *user, int size);
int unix_user2uid(const char *user, uid_t *uid, gid_t *gid);

int unix_gid2group(gid_t gid, char *group, int size);
int unix_group2gid(const char *group, gid_t *gid);

/* setegroup() and seteuid() */
int unix_seteusergroup(const char *user, const char *group);


#ifdef __cplusplus
}
#endif

#endif
