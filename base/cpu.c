#include "cpu.h"
#include <unistd.h>
#include <errno.h>


int cpu_count(void)
{
	int n;

#if defined (_SC_NPROCESSORS_ONLN)
	n = (int) sysconf(_SC_NPROCESSORS_ONLN);
#elif defined (_SC_NPROC_ONLN)
	n = (int) sysconf(_SC_NPROC_ONLN);
#elif defined (HPUX)
#include <sys/mpctl.h>
	n = mpctl(MPC_GETNUMSPUS, 0, 0);
#else
	n = -1;
	errno = ENOSYS;
#endif

	return n;
}

