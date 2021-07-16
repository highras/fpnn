#ifndef FPLOG_H_
#define FPLOG_H_
#include <time.h>
#include <stdarg.h>
#include <stdio.h>
#include <sys/time.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <strings.h>
#include <string>
#include <ostream>
#include <memory>
#include <string>
#include <algorithm>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <thread>
#include <arpa/inet.h>
#include <stdarg.h>
#include <mutex>
#include <sys/un.h>
#include <pthread.h>
#include <sys/syscall.h>

namespace fpnn {

#define FPLOG_BUF_SIZE 4096
#define FPLOG_DEFAULT_MAX_QUEUE_SIZE	500000
#define FPLOG_QUEUE_LIMIT_LOG_INTERVAL  60

inline pid_t getttid()
{
#ifdef __APPLE__
uint64_t tid64;
pthread_threadid_np(NULL, &tid64);
return (pid_t)tid64;
#else
return syscall(SYS_gettid);
#endif
}

typedef enum {FP_LEVEL_FATAL=0, FP_LEVEL_ERROR=1, FP_LEVEL_WARN=2, FP_LEVEL_INFO=3, FP_LEVEL_DEBUG=4} FPLogLevel;
extern FPLogLevel fpLogLevel;

struct FPLogEntry {
    FPLogEntry(): _route(nullptr), _body(nullptr) {

    }
    ~FPLogEntry() {
        if (_route)
            free(_route);
        if (_body)
            free(_body);
    }
    int32_t _type;
    char* _route;
    char* _body;
};

class FPLogBase;
typedef std::shared_ptr<FPLogBase> FPLogBasePtr;

class FPLogBase {
public:
    FPLogBase(size_t maxQueueSize = FPLOG_DEFAULT_MAX_QUEUE_SIZE): _willExit(false), _maxQueueSize(maxQueueSize) {
        _flushThread = std::thread(&FPLogBase::consume, this);
    };
    virtual ~FPLogBase() {
		release();
    };

    void push(FPLogEntry* log);
    void consume();
    void release();

    virtual void flush(FPLogEntry* log) {}

    static int64_t _queueLimitTime;
protected:
    bool _willExit;

private:
    std::queue<FPLogEntry*> _queue;
    std::mutex _mutex;
    std::condition_variable _condition;
    std::thread _flushThread;
    size_t _maxQueueSize;
	std::string _localIP4;
};

class FPStreamLog : public FPLogBase {
public:
    FPStreamLog(std::ostream& out, size_t maxQueueSize = FPLOG_DEFAULT_MAX_QUEUE_SIZE): FPLogBase(maxQueueSize), _out(out.rdbuf()) {}
    void flush(FPLogEntry* log);

private:
    std::ostream _out;
};

#define FPLOG_ROUTE_SIZE_LENGTH 1
#define FPLOG_ATTR_SIZE_LENGTH 2
#define FPLOG_BODY_SIZE_LENGTH 4
#define FPLOG_PID_SIZE_LENGTH 4

class FPPipeLogBase : public FPLogBase {
public:
    FPPipeLogBase(const std::string& route, size_t maxQueueSize = FPLOG_DEFAULT_MAX_QUEUE_SIZE): 
        FPLogBase(maxQueueSize), _routeDebug(nullptr), _routeInfo(nullptr), _routeWarn(nullptr), _routeError(nullptr) {
        ignoreSignal();
        _initRoute(route);
    }
    ~FPPipeLogBase() {
        close(_socket_fd);
        if (_routeDebug)
            delete _routeDebug;
        if (_routeInfo)
            delete _routeInfo;
        if (_routeWarn)
            delete _routeWarn;
        if (_routeError)
            delete _routeError;
        if (_routeFatal)
            delete _routeFatal;
    }
    void ignoreSignal();
    void flush(FPLogEntry* log);
    bool writeBytes(int32_t socket, void* buffer, uint32_t length);
    template <typename K>
    bool readBytes(int32_t socket, uint32_t x, K* buffer);
protected:
    virtual bool _initSocket() { return true; };
    void _initRoute(const std::string& route);

    char* _routeDebug;
    int8_t _routeDebugSize;
    char* _routeInfo;
    int8_t _routeInfoSize;
    char* _routeWarn;
    int8_t _routeWarnSize;
    char* _routeError;
    int8_t _routeErrorSize;
    char* _routeFatal;
    int8_t _routeFatalSize;
    int16_t _attrSize;
    int32_t _socket_fd;
    std::mutex _sockLock;
};

class FPTcpLog : public FPPipeLogBase {
public:
    FPTcpLog(const std::string& route, const std::string& host, int32_t port, size_t maxQueueSize = FPLOG_DEFAULT_MAX_QUEUE_SIZE): 
        FPPipeLogBase(route, maxQueueSize), _host(host), _port(port) {
        _initSocket();
    }

private:
    bool _initSocket();

    std::string _host;
    int32_t _port;
    struct sockaddr_in _server_addr;
};

class FPSockLog : public FPPipeLogBase {
public:
    FPSockLog(const std::string& route, const std::string& socketFile, size_t maxQueueSize = FPLOG_DEFAULT_MAX_QUEUE_SIZE): 
        FPPipeLogBase(route, maxQueueSize), _socketFile(socketFile) {
        _initSocket();
    }

private:
    bool _initSocket();

    std::string _socketFile;
    struct sockaddr_un _server_addr;
};

class FPLog {
public:
    FPLog() {
        FPLog::_pid = 0;
        FPLog::_routeName = "unknown";
    }
    ~FPLog() {}
    static FPLogBasePtr init(std::ostream& out, const std::string& route, const std::string& level = "ERROR", const std::string& serverName = "unknown", int32_t pid = 0, size_t maxQueueSize = FPLOG_DEFAULT_MAX_QUEUE_SIZE);
    static FPLogBasePtr init(const std::string& endpoint, const std::string& route, const std::string& level = "ERROR", const std::string& serverName = "unknown", int32_t pid = 0, size_t maxQueueSize = FPLOG_DEFAULT_MAX_QUEUE_SIZE);
    static void setLevel(const std::string& logLevel);
    static std::string getLevel();
    static FPLogBasePtr instance() { return _logger; }
    static void log(FPLogLevel curLevel, const char* fileName, int32_t line, const char* funcName, const char* tag, const char* format, ...);
    static void logRoute(const std::string& route, const char* fileName, int32_t line, const char* funcName, const char* tag, const char* format, ...);
    static void logTag(const std::string& tag, const std::string& body);

public:
    static int32_t _pid;
private:
    static std::string _routeName;
    static std::string _serverName;
	static std::string _localIP4;
    static FPLogBasePtr _logger;
};

#define LOG_INFO( fmt, args...) if(fpnn::fpLogLevel>=fpnn::FP_LEVEL_INFO) {\
                                    fpnn::FPLog::log(fpnn::FP_LEVEL_INFO,__FILE__,__LINE__,__func__, "", fmt, ##args);}

#define LOG_ERROR( fmt, args...) if(fpnn::fpLogLevel>=fpnn::FP_LEVEL_ERROR) {\
                                    fpnn::FPLog::log(fpnn::FP_LEVEL_ERROR,__FILE__,__LINE__,__func__, "", fmt, ##args);}

#define LOG_WARN( fmt, args...)  if(fpnn::fpLogLevel>=fpnn::FP_LEVEL_WARN) {\
                                    fpnn::FPLog::log(fpnn::FP_LEVEL_WARN,__FILE__,__LINE__,__func__, "", fmt, ##args);}

#define LOG_DEBUG( fmt, args...) if(fpnn::fpLogLevel>=fpnn::FP_LEVEL_DEBUG) {\
                                    fpnn::FPLog::log(fpnn::FP_LEVEL_DEBUG,__FILE__,__LINE__,__func__, "", fmt, ##args);}

#define LOG_FATAL( fmt, args...) if(fpnn::fpLogLevel>=fpnn::FP_LEVEL_FATAL) {\
                                    fpnn::FPLog::log(fpnn::FP_LEVEL_FATAL,__FILE__,__LINE__,__func__, "", fmt, ##args);}


#define XLOG_INFO( tag, fmt, args...) if(fpnn::fpLogLevel>=fpnn::FP_LEVEL_INFO) {\
                                    fpnn::FPLog::log(fpnn::FP_LEVEL_INFO,__FILE__,__LINE__,__func__, tag, fmt, ##args);}

#define XLOG_ERROR( tag, fmt, args...) if(fpnn::fpLogLevel>=fpnn::FP_LEVEL_ERROR) {\
                                    fpnn::FPLog::log(fpnn::FP_LEVEL_ERROR,__FILE__,__LINE__,__func__, tag, fmt, ##args);}

#define XLOG_WARN( tag, fmt, args...)  if(fpnn::fpLogLevel>=fpnn::FP_LEVEL_WARN) {\
                                    fpnn::FPLog::log(fpnn::FP_LEVEL_WARN,__FILE__,__LINE__,__func__, tag, fmt, ##args);}

#define XLOG_DEBUG( tag, fmt, args...) if(fpnn::fpLogLevel>=fpnn::FP_LEVEL_DEBUG) {\
                                    fpnn::FPLog::log(fpnn::FP_LEVEL_DEBUG,__FILE__,__LINE__,__func__, tag, fmt, ##args);}

#define XLOG_FATAL( tag, fmt, args...) if(fpnn::fpLogLevel>=fpnn::FP_LEVEL_FATAL) {\
                                    fpnn::FPLog::log(fpnn::FP_LEVEL_FATAL,__FILE__,__LINE__,__func__, tag, fmt, ##args);}

//log it, do not consider fpLogLevel
#define UXLOG( tag, fmt, args...) fpnn::FPLog::log(fpnn::FP_LEVEL_INFO,__FILE__,__LINE__,__func__, tag, fmt, ##args);

// log BI data, no format string added before the log 
#define TLOG(tag, body) fpnn::FPLog::logTag(tag, body);

// use for stat log, the final route_name is: "rtmGated.stat"
#define LOG_STAT(tag, fmt, args...) fpnn::FPLog::logRoute("stat",__FILE__,__LINE__,__func__,tag, fmt, ##args);

// can specify the route_name, if LOG_ROUTE("xxx", ...) the final route_name is: "rtmGated.xxx"
#define LOG_ROUTE(route, tag, fmt, args...) fpnn::FPLog::logRoute(route,__FILE__,__LINE__,__func__,tag, fmt, ##args);

#define LOG_SAMPLE(tag, fmt, args...) fpnn::FPLog::logRoute("sample",__FILE__,__LINE__,__func__,tag, fmt, ##args);

}

#endif

