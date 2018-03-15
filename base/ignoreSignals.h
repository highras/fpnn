#include <signal.h>

//--- Just using for test clients and simply utility clients.
namespace fpnn {
inline void ignoreSignals()
{
	struct sigaction	sa;
	
	sa.sa_handler = SIG_IGN;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;

	sigaction(SIGHUP, &sa, NULL);
	sigaction(SIGXFSZ, &sa, NULL);
	sigaction(SIGPIPE, &sa, NULL);
	//sigaction(SIGINT, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGUSR1, &sa, NULL);
	sigaction(SIGUSR2, &sa, NULL);
}
}
