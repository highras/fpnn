#ifndef FPNN_HTTPPARSER_H_
#define FPNN_HTTPPARSER_H_

#include <memory>
#include <string>
#include <map>
#include "httpcode.h"
#include "FpnnError.h"
#include "ChainBuffer.h"
#include "FPMessage.h"

namespace fpnn{

	class HttpParser;

	typedef std::map<std::string, std::string> StringMap;

	typedef std::shared_ptr<HttpParser> HttpParserPtr;

	class HttpParser
	{
		public:

			HttpParser();
			virtual ~HttpParser();

			bool parseURI(const std::string& uri);
			bool parseCookie(const std::string& cookie);

			bool parseHeader(ChainBuffer* cb, int header_length);
			bool parseBody(ChainBuffer* cb);

			std::string _read_chunked(char* body, int len);

			char* _get_line(char* p);

			bool _chunked;
			int _contentLen;
			bool _post;

			std::string _method;
			std::string _content;

			StringMap _httpInfos;

		public:
			int _headerLength;
			int _contentOffset;
			int _parseOffset;
			bool _headReceived;

		public:
			bool checkHttpHeader(ChainBuffer* cb);
			bool isChunckedContentCompleted(ChainBuffer* cb);
			void reset();
			std::string urldecode(const std::string& uri);
	};

}

#endif
