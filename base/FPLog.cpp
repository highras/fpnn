#include <pthread.h>
#include <errno.h>
#include <assert.h>
#include <arpa/inet.h>
#include <sys/syscall.h>
#include <string.h>
#include <signal.h>
#include <iostream>
#include <fstream>
#include "FPLog.h"
#include "ServerInfo.h"
#include "msec.h"
#include "TimeUtil.h"

using namespace fpnn;

FPLogLevel fpnn::fpLogLevel(FP_LEVEL_ERROR);
FPLogBasePtr FPLog::_logger(new FPStreamLog(std::cout));
int32_t FPLog::_pid = 0;
std::string FPLog::_routeName = "";
std::string FPLog::_serverName = "";
std::string FPLog::_localIP4 = "";
int64_t FPLogBase::_queueLimitTime = 0;

void FPLogBase::release() {
    {
        std::unique_lock<std::mutex> lck(_mutex);
        _willExit = true;
        _condition.notify_all();
    }
    _flushThread.join();
}

void FPLogBase::push(FPLogEntry* log) {
    std::unique_lock<std::mutex> lck(_mutex);
    if (_queue.size() < _maxQueueSize) {
        _queue.push(log);
        _condition.notify_one();
    } else {
        int64_t now = slack_real_sec();
        if (now - _queueLimitTime > FPLOG_QUEUE_LIMIT_LOG_INTERVAL) {
            if (_localIP4.empty())
                _localIP4 = ServerInfo::getServerLocalIP4();
            
            snprintf(log->_body, FPLOG_BUF_SIZE, "[%s]~[ERROR]~[]~[%s]~[]: FPLog Queue Limit", TimeUtil::getDateTimeMS().c_str(), _localIP4.c_str());

            FPLogEntry* frontLog = _queue.front();
            if (frontLog)
                delete frontLog;
            _queue.pop();

            _queue.push(log);
            _queueLimitTime = now;
            _condition.notify_one();
        } else
            delete log;
    }
}

void FPLogBase::consume() {
    while (true) {
        FPLogEntry* log = nullptr;
        {
            std::unique_lock<std::mutex> lck(_mutex);
            while (_queue.size() == 0) {
                if (_willExit)
                    return;
                _condition.wait(lck);
            }
            log = _queue.front();
            _queue.pop();
        }
        if (log) {
            flush(log);
        }
    }
}

void FPStreamLog::flush(FPLogEntry* log) {
    _out << log->_body << std::endl;
    delete log;
}

void FPPipeLogBase::ignoreSignal() {
    struct sigaction sa;
    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGPIPE, &sa, NULL);
}

bool FPPipeLogBase::writeBytes(int32_t socket, void* buffer, uint32_t length) {
    char *ptr = (char*)buffer;
    while (length > 0) {
        int32_t i = send(socket, ptr, length, 0);
        if (i <= 0) return false;
        ptr += i;
        length -= i;
    }
    return true;
}

template <typename K>
bool FPPipeLogBase::readBytes(int32_t socket, uint32_t x, K* buffer) {
    int32_t bytesRead = 0;
    int32_t result;
    while (bytesRead < int32_t(x)) {
        result = read(socket, buffer + bytesRead, x - bytesRead);
        if (result <= 0)
            return false;
        bytesRead += result;
    }
    return true;
}

void FPPipeLogBase::flush(FPLogEntry* log) {
    char* route = nullptr;
    int8_t routeSize;
    int32_t level = log->_type;
    if (level == FP_LEVEL_DEBUG) {
        route = _routeDebug;
        routeSize = _routeDebugSize;
    } else if (level == FP_LEVEL_INFO) {
        route = _routeInfo;
        routeSize = _routeInfoSize;
    } else if (level == FP_LEVEL_WARN) {
        route = _routeWarn;
        routeSize = _routeWarnSize;
    } else if (level == FP_LEVEL_ERROR) {
        route = _routeError;
        routeSize = _routeErrorSize;
    } else if (level == FP_LEVEL_FATAL) {
        route = _routeFatal;
        routeSize = _routeFatalSize;
    } else if (level == -1) {
        route = log->_route;
        routeSize = strlen(log->_route); 
    } else {
        fprintf(stderr, "FPLog: log level error\n");
        delete log;
        return;
    }
    int32_t bodySize = strlen(log->_body);
    while (true) {
        if (_willExit) {
            break;
        }
        std::lock_guard<std::mutex> lck(_sockLock); 
        if ( writeBytes(_socket_fd, &routeSize, FPLOG_ROUTE_SIZE_LENGTH) &&
             writeBytes(_socket_fd, route, routeSize) &&
             writeBytes(_socket_fd, &_attrSize, FPLOG_ATTR_SIZE_LENGTH) &&
             writeBytes(_socket_fd, &FPLog::_pid, FPLOG_PID_SIZE_LENGTH) &&
             writeBytes(_socket_fd, &bodySize, FPLOG_BODY_SIZE_LENGTH) &&
             writeBytes(_socket_fd, log->_body, bodySize)) {
            
            int8_t ret;
            bool res = readBytes(_socket_fd, 1, &ret);
            if (!res || ret != '1') {
                close(_socket_fd);
                _initSocket();
                usleep(200000);
                continue;
            }
            break;
        } else {
            close(_socket_fd);
            _initSocket();
            usleep(200000);
            continue;
        }
    }
    delete log;
}

void FPPipeLogBase::_initRoute(const std::string& route) {
  
    std::string key = route + ".debug"; 
    _routeDebug = new char[key.length() + 1];
    std::copy(key.begin(), key.end(), _routeDebug);
    _routeDebug[key.length()] = 0;
    _routeDebugSize = strlen(_routeDebug);

    key = route + ".info"; 
    _routeInfo = new char[key.length() + 1];
    std::copy(key.begin(), key.end(), _routeInfo);
    _routeInfo[key.length()] = 0;
    _routeInfoSize = strlen(_routeInfo);

    key = route + ".warn"; 
    _routeWarn = new char[key.length() + 1];
    std::copy(key.begin(), key.end(), _routeWarn);
    _routeWarn[key.length()] = 0;
    _routeWarnSize = strlen(_routeWarn);

    key = route + ".error"; 
    _routeError = new char[key.length() + 1];
    std::copy(key.begin(), key.end(), _routeError);
    _routeError[key.length()] = 0;
    _routeErrorSize = strlen(_routeError);
    
    key = route + ".fatal"; 
    _routeFatal = new char[key.length() + 1];
    std::copy(key.begin(), key.end(), _routeFatal);
    _routeFatal[key.length()] = 0;
    _routeFatalSize = strlen(_routeFatal);

    _attrSize = 0;
}

bool FPTcpLog::_initSocket() {
    if ((_socket_fd = socket(AF_INET,SOCK_STREAM,0)) < 0) {
        fprintf(stderr, "FPLog create socket error: %s (errno: %d)\n)", strerror(errno), errno);
        return false;
    }
    memset(&_server_addr, 0, sizeof(_server_addr));
    _server_addr.sin_family = AF_INET;
    _server_addr.sin_port = htons(_port);
    if(inet_pton(AF_INET, _host.c_str(), &_server_addr.sin_addr) <=0) {
        fprintf(stderr, "FPLog inet_pton error for %s\n", _host.c_str());
        return false;
    }
    if(connect(_socket_fd, (struct sockaddr*)&_server_addr, sizeof(_server_addr)) < 0) {
        fprintf(stderr, "FPLog connect error: %s (errno: %d)\n", strerror(errno), errno);
        return false;
    }
    return true;
}

bool FPSockLog::_initSocket() {
    if ((_socket_fd = socket(PF_UNIX, SOCK_STREAM,0)) < 0) {
        fprintf(stderr, "FPLog create socket error: %s (errno: %d)\n)", strerror(errno), errno);
        return false;
    }
    memset(&_server_addr, 0, sizeof(_server_addr));
    _server_addr.sun_family = AF_UNIX;
    snprintf(_server_addr.sun_path, UNIX_PATH_MAX, _socketFile.c_str());
    if(connect(_socket_fd, (struct sockaddr*)&_server_addr, sizeof(_server_addr)) < 0) {
        fprintf(stderr, "FPLog connect error: %s (errno: %d)\n", strerror(errno), errno);
        return false;
    }
    return true;
}

FPLogBasePtr FPLog::init(std::ostream& out, const std::string& route, const std::string& level, const std::string& serverName, int32_t pid, size_t maxQueueSize) {
    FPLog::_pid = pid;
    FPLog::_routeName = route;
    FPLog::_serverName = serverName;
	FPLog::_localIP4 = ServerInfo::getServerLocalIP4();
    setLevel(level);
    _logger.reset(new FPStreamLog(out, maxQueueSize));
    return _logger;
}

FPLogBasePtr FPLog::init(const std::string& endpoint, const std::string& route, const std::string& level, const std::string& serverName, int32_t pid, size_t maxQueueSize) {
    FPLog::_pid = pid;
    FPLog::_routeName = route;
    FPLog::_serverName = serverName;
	FPLog::_localIP4 = ServerInfo::getServerLocalIP4();
    setLevel(level);
    if (endpoint == "std::cerr") {
        _logger.reset(new FPStreamLog(std::cerr, maxQueueSize));
        return _logger;
    } else if (endpoint == "std::cout") {
        _logger.reset(new FPStreamLog(std::cout, maxQueueSize));
        return _logger;
    } else if (endpoint.substr(0, 6) == "tcp://") {
        std::string end = endpoint.substr(6);
        size_t s = end.find_last_of(":");
        std::string host = end.substr(0, s);
        int32_t port = atoi(end.substr(s + 1).c_str());
        _logger.reset(new FPTcpLog(route, host, port, maxQueueSize));
        return _logger; 
    } else if (endpoint.substr(0, 7) == "unix://") {
        std::string sock = endpoint.substr(7);
        _logger.reset(new FPSockLog(route, sock, maxQueueSize));
        return _logger; 
    } else {
        fprintf(stderr, "FPLog init error, parse endpoint fail: %s\n", endpoint.c_str());
    }
    return FPLogBasePtr();
}

void FPLog::setLevel(const std::string& logLevel) {
    std::string l(logLevel);
    std::transform(l.begin(), l.end(), l.begin(), toupper);
    FPLogLevel level = FP_LEVEL_ERROR;
    if (l == "WARN")
        level = FP_LEVEL_WARN;
    else if (l == "INFO")
        level = FP_LEVEL_INFO;
    else if (l == "DEBUG")
        level = FP_LEVEL_DEBUG;
    else if (l == "FATAL")
        level = FP_LEVEL_FATAL;
    fpLogLevel = level; 
}

std::string FPLog::getLevel(){
    if(fpLogLevel == FP_LEVEL_WARN)
    {
        return "WARN";
    }
    else if(fpLogLevel == FP_LEVEL_INFO){
        return "INFO";
    }
    else if(fpLogLevel == FP_LEVEL_DEBUG){
        return "DEBUG";
    } else if (fpLogLevel == FP_LEVEL_FATAL) {
        return "FATAL";
    }
    return "ERROR";
}

void FPLog::log(FPLogLevel curLevel, const char* fileName, int32_t line, const char* funcName, const char* tag, const char* format, ...) {
    FPLogEntry* log = new FPLogEntry();
    if (!log) return;
    static const char* dbgLevelStr[]={"FATAL","ERROR","WARN","INFO","DEBUG"};
    static pid_t pid=0;
    if(pid==0) pid=getpid();
    log->_body = (char*)malloc(FPLOG_BUF_SIZE);
	if(!log->_body) {
        delete log;
        return;
    }
    int32_t s = snprintf(log->_body, FPLOG_BUF_SIZE, "[%s]~[%s]~[%d(%ld)]~[%s@%s@%s@%s:%d]~[%s]: ", 
        TimeUtil::getDateTimeMS().c_str(),
        dbgLevelStr[curLevel],
        pid,syscall(SYS_gettid),
        _localIP4.c_str(), _serverName.c_str(), funcName, fileName, line, tag);
    s = std::min(FPLOG_BUF_SIZE, s);
    if (s > 0) {
        va_list va;
        va_start(va, format);
        int32_t vs = vsnprintf(log->_body + s, FPLOG_BUF_SIZE - s, format, va);
        va_end(va);
        if (vs > 0) {
            log->_type = curLevel;
            _logger->push(log);
        } else
            delete log;
    } else
        delete log;
}

void FPLog::logRoute(const std::string& route, const char* fileName, int32_t line, const char* funcName, const char* tag, const char* format, ...) {
    FPLogEntry* log = new FPLogEntry();
    if (!log) return;
    static pid_t pid=0;
    if(pid==0) pid=getpid();
    log->_body = (char*)malloc(FPLOG_BUF_SIZE);
	if(!log->_body) {
        delete log;
        return;
    }
    int32_t s = snprintf(log->_body, FPLOG_BUF_SIZE, "[%s]~[%s]~[%d(%ld)]~[%s@%s@%s@%s:%d]~[%s]: ", 
        TimeUtil::getDateTimeMS().c_str(),
        "INFO",
        pid,syscall(SYS_gettid),
        _localIP4.c_str(), _serverName.c_str(), funcName, fileName, line, tag);
    s = std::min(FPLOG_BUF_SIZE, s);
    if (s > 0) {
        va_list va;
        va_start(va, format);
        int32_t vs = vsnprintf(log->_body + s, FPLOG_BUF_SIZE - s, format, va);
        va_end(va);
        if (vs > 0) {
            log->_type = -1;
            std::string routeNew = FPLog::_routeName + "." + route;
            log->_route = (char*)malloc(routeNew.length() + 1);
            strcpy(log->_route, routeNew.c_str());
            _logger->push(log);
        } else
            delete log;
    } else
        delete log;
}

void FPLog::logTag(const std::string& tag, const std::string& body) {
    FPLogEntry* log = new FPLogEntry();
    if (!log) return;
    log->_body = (char*)malloc(body.length() + 1);
    log->_route = (char*)malloc(tag.length() + 1);
	if(!log->_body || !log->_route) {
        delete log;
        return;
    }
    strcpy(log->_body, body.c_str());
    strcpy(log->_route, tag.c_str());
    log->_type = -1;
    _logger->push(log);
}

// g++ -std=c++11 -o fplog_test -g -DTEST_FP_LOG FPLog.cpp -pthread -L. -L../base -lfpbase -lcurl -I. 
#ifdef TEST_FP_LOG
using namespace std;

int main(int argc, char **argv) {

    //ofstream file;
    //file.open("test.log");
    //FPLog::init(file, "fpnn.test", "DEBUG");

    //FPLog::init(std::cerr, "fpnn.test", "DEBUG");
    //FPLog::init("std::cerr", "fpnn.test", "DEBUG");
    //FPLog::init("std::cout", "fpnn.test", "DEBUG");
    //FPLog::init("tcp://127.0.0.1:9999", "fpnn.test", "DEBUG");
    FPLog::init("unix:///mnt/home/jianjun.zhao/code/infra-log-server/logAgent/fplog.sock", "rtmGated", "DEBUG", "test_server_name");

    //FPLog::setLevel("DEBUG");

    int i = 1;

    while (true) {
        LOG_DEBUG("%d: test debug", i++);
        LOG_INFO("%d: test info", i++);
        LOG_WARN("%d: test warn", i++);
        LOG_ERROR("%d: test error", i++);
        LOG_FATAL("%d: test fatal", i++);
        
        XLOG_DEBUG("test_tag", "%d: test debug", i++);
        XLOG_INFO("test_tag", "%d: test info", i++);
        XLOG_WARN("test_tag", "%d: test warn", i++);
        XLOG_ERROR("test_tag", "%d: test error", i++);
        XLOG_FATAL("test_tag", "%d: test fatal", i++);


        LOG_STAT("xxx", "%s test LOG_STAT", "aaa");

        sleep(1);
    }

    return 0;
}

#endif

