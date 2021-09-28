#ifdef __APPLE__
#include "FPTimer.kqueue.cpp"
#else
#include "FPTimer.epoll.cpp"
#endif