#include "unix_user.h"
#include <pwd.h>
#include <grp.h>
#include <string.h>
#include <unistd.h>

#define BSIZE	1024

int unix_uid2user(uid_t uid, char *user, int size)
{
	char buf[BSIZE];
	struct passwd pwbuf, *pw;

	getpwuid_r(uid, &pwbuf, buf, sizeof(buf), &pw);
	if (!pw)
		return -1;

	strncpy(user, pw->pw_name, size);
	return 0;
}

int unix_user2uid(const char *user, uid_t *uid, gid_t *gid)
{
	char buf[BSIZE];
	struct passwd pwbuf, *pw;

	getpwnam_r(user, &pwbuf, buf, sizeof(buf), &pw);
	if (!pw)
		return -1;

	if (uid)
		*uid = pw->pw_uid;
	if (gid)
		*gid = pw->pw_gid;
	return 0;
}


int unix_gid2group(gid_t gid, char *group, int size)
{
	char buf[BSIZE];
	struct group grbuf, *gr;

	getgrgid_r(gid, &grbuf, buf, sizeof(buf), &gr);
	if (!gr)
		return -1;

	strncpy(group, gr->gr_name, size);
	return 0;
}

int unix_group2gid(const char *group, gid_t *gid)
{
	char buf[BSIZE];
	struct group grbuf, *gr;

	getgrnam_r(group, &grbuf, buf, sizeof(buf), &gr);
	if (!gr)
		return -1;

	if (gid)
		*gid = gr->gr_gid;
	return 0;
}

int unix_seteusergroup(const char *user, const char *group)
{
	char buf[BSIZE];
	uid_t uid = -1;
	gid_t gid = -1;
	int r = 0;

	if (user && user[0])
	{
		struct passwd pwbuf, *pw;

		getpwnam_r(user, &pwbuf, buf, sizeof(buf), &pw);
		if (!pw)
			r = -1;
		else
		{
			uid = pw->pw_uid;
			gid = pw->pw_gid;
		}
	}

	if (group && group[0])
	{
		struct group grbuf, *gr;

		getgrnam_r(group, &grbuf, buf, sizeof(buf), &gr);
		if (!gr)
			r = -1;
		else
			gid = gr->gr_gid;
	}

	if (gid != (gid_t)-1)
	{
		if (setegid(gid) < 0)
			r = -1;
	}

	if (uid != (gid_t)-1)
	{
		if (seteuid(uid) < 0)
			r = -1;
	}

	return r;
}

