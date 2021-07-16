#ifndef FpnnError_h_
#define FpnnError_h_
#include <mutex>
#include <string>
#include <stdexcept>

namespace fpnn {

#define FPNN_ERROR(FPErr)                            \
	FPErr(__FILE__, __FUNCTION__, __LINE__)

#define FPNN_ERROR_CODE(FPErr, code)                     \
        FPErr(__FILE__, __FUNCTION__, __LINE__, (code))

#define FPNN_ERROR_CODE_MSG(FPErr, code, message)                \
        FPErr(__FILE__, __FUNCTION__, __LINE__, (code), (message))

#define FPNN_ERROR_CODE_FMT(FPErr, code, ...)                \
        FPErr(__FILE__, __FUNCTION__, __LINE__, (code), FpnnError::format(__VA_ARGS__))

#define FPNN_ERROR_MSG(FPErr, message)					\
	FPErr(__FILE__, __FUNCTION__, __LINE__, 0, (message))

#define FPNN_ERROR_FMT(FPErr, ...)                   \
        FPErr(__FILE__, __FUNCTION__, __LINE__, 0, FpnnError::format(__VA_ARGS__))

class FpnnError: public std::exception
{
public:
	FpnnError(const char *file, const char* fun, int32_t line, int32_t code = 0, const std::string& msg = "")
		 : _file(file), _fun(fun), _line(line), _code(code), _message(msg)
	{}

	virtual ~FpnnError() noexcept 				{}

	virtual FpnnError* clone() const			{ return new FpnnError(*this); }
	virtual void do_throw() const				{ throw *this; }

	virtual const char* what() const noexcept;

	const char* file() const noexcept 			{ return _file; }
	const char* fun() const noexcept 			{ return _fun; }
	int32_t line() const noexcept 				{ return _line; }
	int32_t code() const noexcept 				{ return _code; }
	const std::string& message() const noexcept 	{ return _message; }

	static std::string format(const char *fmt, ...); 

	static std::mutex _mutex;

protected:
	const char* const _file;
	const char* const _fun;
	int32_t _line;
	int32_t _code;
	std::string _message;
	mutable std::string _what;
};


#define FPNN_(BASE, DERIVED)						\
class DERIVED: public BASE {						\
public:									\
	DERIVED(const char *file, const char*fun, int32_t line, int32_t code = 0,	\
		const std::string& msg = "" 				\
		) : BASE(file, fun, line, code, msg) {} 			\
									\
	virtual FpnnError* clone() const { return new DERIVED(*this); }	\
	virtual void do_throw() const { throw *this; }			\
};

FPNN_(FpnnError,			FpnnProtoError)
FPNN_(FpnnError,			FpnnCoreError)
FPNN_(FpnnError,            FpnnHTTPError)


FPNN_(FpnnError,			FpnnLogicError)
FPNN_(FpnnLogicError, 		FpnnAssertError)
FPNN_(FpnnLogicError, 		FpnnOutRangeError)
FPNN_(FpnnLogicError, 		FpnnArgumentError)

FPNN_(FpnnError,			FpnnSyscallError)
FPNN_(FpnnError,			FpnnMemoryError)
FPNN_(FpnnError,			FpnnUnsupportedError)
FPNN_(FpnnError,			FpnnEncryptError)


enum {
	FPNN_EC_OK							= 0,

	//for proto
	FPNN_EC_PROTO_UNKNOWN_ERROR			= 10001,
	FPNN_EC_PROTO_NOT_SUPPORTED			= 10002,
	FPNN_EC_PROTO_INVALID_PACKAGE		= 10003,
	FPNN_EC_PROTO_JSON_CONVERT			= 10004,
	FPNN_EC_PROTO_STRING_KEY			= 10005,
	FPNN_EC_PROTO_MAP_VALUE				= 10006,
	FPNN_EC_PROTO_METHOD_TYPE			= 10007,
	FPNN_EC_PROTO_PROTO_TYPE			= 10008,
	FPNN_EC_PROTO_KEY_NOT_FOUND			= 10009,
	FPNN_EC_PROTO_TYPE_CONVERT			= 10010,
	FPNN_EC_PROTO_FILE_SIGN				= 10011,
	FPNN_EC_PROTO_FILE_NOT_EXIST		= 10012,

	//for core
	FPNN_EC_CORE_UNKNOWN_ERROR			= 20001,
	FPNN_EC_CORE_CONNECTION_CLOSED		= 20002,
	FPNN_EC_CORE_TIMEOUT				= 20003,
	FPNN_EC_CORE_UNKNOWN_METHOD			= 20004,
	FPNN_EC_CORE_ENCODING				= 20005,
	FPNN_EC_CORE_DECODING				= 20006,
	FPNN_EC_CORE_SEND_ERROR				= 20007,
	FPNN_EC_CORE_RECV_ERROR				= 20008,
	FPNN_EC_CORE_INVALID_PACKAGE		= 20009,
	FPNN_EC_CORE_HTTP_ERROR				= 20010,
	FPNN_EC_CORE_WORK_QUEUE_FULL		= 20011,
	FPNN_EC_CORE_INVALID_CONNECTION		= 20012,
	FPNN_EC_CORE_FORBIDDEN				= 20013,
	FPNN_EC_CORE_SERVER_STOPPING		= 20014,
	FPNN_EC_CORE_CANCELLED				= 20015,

	//for other
	FPNN_EC_ZIP_COMPRESS				= 30001,
	FPNN_EC_ZIP_DECOMPRESS				= 30002,
};

}

#endif
