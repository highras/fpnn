#ifndef HTTPCODE_H_
#define HTTPCODE_H_ 1

#ifdef __cplusplus
extern "C" {
#endif

#define HTTPCODES							\
 HCS(100, CONTINUE, 			"Continue")			\
 HCS(101, SWITCHING_PROTOCOLS, 		"Switching Protocols")		\
									\
 HCS(200, OK, 				"OK")				\
 HCS(201, CREATED,			"Created")			\
 HCS(202, ACCEPTED,			"Accepted")			\
 HCS(203, NON_AUTHORITATIVE_INFORMATION,	"Non-Authoritative Information")\
 HCS(204, NO_CONTENT,			"No Content")			\
 HCS(205, RESET_CONTENT,			"Reset Content")		\
 HCS(206, PARTIAL_CONTENT,		"Partial Content")		\
									\
 HCS(300, MULTIPLE_CHOICES,		"Multiple Choices")		\
 HCS(301, MOVED_PERMANENTLY,		"Moved Permanently")		\
 HCS(302, FOUND,				"Found")			\
 HCS(303, SEE_OTHER,			"See Other")			\
 HCS(304, NOT_MODIFIED,			"Not Modified")			\
 HCS(305, USER_PROXY,			"Use Proxy")			\
 HCS(307, TEMPORARY_REDIRECT,		"Temporary Redirect")		\
									\
 HCS(400, BAD_REQUEST,			"Bad Request")			\
 HCS(401, UNAUTHORIZED,			"Unauthorized")			\
 HCS(402, PAYMENT_REQUIRED,		"Payment Required")		\
 HCS(403, FORBIDDEN,			"Forbidden")			\
 HCS(404, NOT_FOUND,			"Not Found")			\
 HCS(405, METHOD_NOT_ALLOWED,		"Method Not Allowed")		\
 HCS(406, NOT_ACCEPTABLE,		"Not Acceptable")		\
 HCS(407, PROXY_AUTHENTICATION_REQUIRED,	"Proxy Authentication Required")\
 HCS(408, REQUEST_TIMEOUT,		"Request Timeout")		\
 HCS(409, CONFLICT,			"Conflict")			\
 HCS(410, GONE,				"Gone")				\
 HCS(411, LENGTH_REQUIRED,		"Length Required")		\
 HCS(412, PRECONDITION_FAILED,		"Precondition Failed")		\
 HCS(413, REQUEST_ENTITY_TOO_LARGE,	"Request Entity Too Large")	\
 HCS(414, REQUEST_URI_TOO_LONG,		"Request-URI Too Long")		\
 HCS(415, UNSUPPORTED_MEDIA_TYPE,	"Unsupported Media Type")	\
 HCS(416, REQUESTED_RANGE_NOT_SATISFIABLE,"Requested Range Not Satisfiable")\
 HCS(417, EXPECTATION_FAILED,		"Expectation Failed")		\
									\
 HCS(500, INTERNAL_SERVER_ERROR,		"Internal Server Error")	\
 HCS(501, NOT_IMPLEMENTED,		"Not Implemented")		\
 HCS(502, BAD_GATEWAY,			"Bad Gateway")			\
 HCS(503, SERVICE_UNAVAILABLE,		"Service Unavailable")		\
 HCS(504, GATEWAY_TIMEOUT,		"Gateway Timeout")		\
 HCS(505, VERSION_NOT_SUPPPORTED,	"HTTP Version Not Supported")	\
									\
 /* END OF HTTPCODES */


enum {
#define HCS(code, name, desc)	HTTPCODE_##name = code,
	HTTPCODES
#undef HCS
};


const char *httpcode_description(int code);


#ifdef __cplusplus
}
#endif

#endif
