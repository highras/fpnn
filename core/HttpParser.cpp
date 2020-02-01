#include "HttpParser.h"
#include "FPLog.h"
#include "StringUtil.h"
#include "hex.h"
#include "HttpClient.h"
#include "Config.h"
#include <string.h>
#include <sys/uio.h>
#include <pthread.h>
#include <ctype.h>
#include <iostream>

using namespace fpnn;

HttpParser::HttpParser(){
	reset();
}

HttpParser::~HttpParser(){
}

std::string HttpParser::_read_chunked(char* body, int len)
{
	std::string result;
	char* line = body;
	char* newP = line;
	while (true)
	{
		newP = _get_line(line);
		if (!newP)
			throw FPNN_ERROR_CODE_MSG(FpnnHTTPError, FPNN_EC_CORE_HTTP_ERROR, "Unexpected connection close");
		int c = strtol(line, NULL, 16);
		if (c == 0)
		{
			break;
		}

		result += std::string(newP, c);

		line = newP + c;

		//read CRCL
		newP = _get_line(line);
		if (!newP)
			throw FPNN_ERROR_CODE_MSG(FpnnHTTPError, FPNN_EC_CORE_HTTP_ERROR, "Unexpected connection close");

		line = newP;
	}
	return result;
}

char* HttpParser::_get_line(char* p){
	char* new_p = strchr(p, '\n');
	if(new_p){
		*new_p = '\0';
		if(new_p - p > 0){
			if(*(new_p-1) == '\r')
				*(new_p-1) = '\0';
		}
		++new_p;//move to next line
	}
	return new_p;
}

bool HttpParser::parseHeader(ChainBuffer* cb, int header_length)
{
	char* header_buffer = NULL;
	_headerLength = header_length;

	try{
		header_buffer = (char*)malloc(header_length+1);
		cb->writeTo(header_buffer, header_length, 0);
		header_buffer[header_length] = 0;

		char* line = header_buffer;

		char* newP = _get_line(header_buffer);
		if (!newP)
			throw FPNN_ERROR_CODE_MSG(FpnnHTTPError, FPNN_EC_CORE_HTTP_ERROR, "Can not get one line");

		//now line contain the line which end by \0

		std::vector<std::string> elems;
		StringUtil::split(line, " ", elems);
		if (elems.size() != 3) 
			throw FPNN_ERROR_CODE_FMT(FpnnHTTPError, FPNN_EC_CORE_HTTP_ERROR, "Unknown HTTP protocol-1 (%s)", line);

		if ((elems[0].compare("GET") != 0 && elems[0].compare("POST") != 0) || elems[2].compare(0,7,"HTTP/1.") != 0)
			throw FPNN_ERROR_CODE_FMT(FpnnHTTPError, FPNN_EC_CORE_HTTP_ERROR, "Unknown HTTP protocol-2 (%s)", line);

		if(elems[0].compare("GET") == 0)
			_post = false;

		std::string uri = elems[1];

		elems.clear();
		StringUtil::split(uri, "?", elems);
		if(elems.size() > 2)
			throw FPNN_ERROR_CODE_FMT(FpnnHTTPError, FPNN_EC_CORE_HTTP_ERROR, "Unknown HTTP protocol-3 (%s)", line);
		std::string method = elems[0];
		if(elems.size() == 2)
			uri = elems[1];
		else uri = "";

		//parse method, must be /service/getuser, getuser is method
		elems.clear();
		StringUtil::split(method, "/", elems);
		if(elems.size() != 2)
			throw FPNN_ERROR_CODE_FMT(FpnnHTTPError, FPNN_EC_CORE_HTTP_ERROR, "Invalid FPNN HTTP method path format (%s)", line);

		if(elems[0].compare("service") != 0)
			throw FPNN_ERROR_CODE_FMT(FpnnHTTPError, FPNN_EC_CORE_HTTP_ERROR, "Not Valid FPNN HTTP protocol (%s)", line);

		_method = elems[1];

		//parse uri
		parseURI(uri);

		while (true)
		{
			line = newP;
			newP = _get_line(line);

			char *p = (char *)strchr(line, ':');
			if (!p)
				throw FPNN_ERROR_CODE_FMT(FpnnHTTPError, FPNN_EC_CORE_HTTP_ERROR, "Invalid HTTP Header (%s)", line);

			std::string name = std::string(line, p++ - line);
			std::string value = std::string(p);

			StringUtil::trim(name);
			StringUtil::trim(value);

			if (name == "Content-Length"){
				_contentLen = stoi(value);
				if (_contentLen > Config::_max_recv_package_length)
					throw FPNN_ERROR_CODE_FMT(FpnnHTTPError, FPNN_EC_CORE_HTTP_ERROR, "Content too big (%zd)", _contentLen);
			}
			else if (name == "Transfer-Encoding" && value == "chunked"){
				_chunked = true;
				LOG_INFO("chunked data, method(%s)", _method.c_str());
			}
			else if (name == "Cookie"){
				parseCookie(value);
				if (!newP)// the last one
					break;
				continue;
			}
			_httpInfos["h_"+name ] = value;

			if (!newP)// the last one
				break;

			line = newP;
		}

		if(header_buffer)
			free(header_buffer);
	}
	catch(const FpnnError& ex){
		if(header_buffer)
			free(header_buffer);
		throw ex;
	}
	catch(...){
		if(header_buffer)
			free(header_buffer);
		throw FPNN_ERROR_CODE_MSG(FpnnHTTPError, FPNN_EC_CORE_HTTP_ERROR, "Unknown Error");
	}
	return true;
}

bool HttpParser::parseBody(ChainBuffer* cb){
	int remainLen = cb->length() - _contentOffset;
	char* buffer = NULL;

	try{
		buffer = (char*)malloc(remainLen+1);
		cb->writeTo(buffer, remainLen, _contentOffset);
		buffer[remainLen] = 0;

		if (_contentLen > 0){
			_content = buffer;
		}
		else if (_chunked){
			_content = _read_chunked(buffer, remainLen);
		}
		else{
			throw FPNN_ERROR_CODE_MSG(FpnnHTTPError, FPNN_EC_CORE_HTTP_ERROR, "Content-Len is zero and is not chunked");
		}
		free(buffer);
	}
	catch(const FpnnError& ex){
		free(buffer);
		throw ex;
	}
	catch(...){
		free(buffer);
		throw FPNN_ERROR_CODE_MSG(FpnnHTTPError, FPNN_EC_CORE_HTTP_ERROR, "Unknown Error");
	}
	return true;
}


bool HttpParser::parseURI(const std::string& uri){
	if(uri.size() == 0) return true;
	std::string uri2 = HttpClient::uriDecode2(uri);
	std::vector<std::string> elems;
	StringUtil::split(uri2, "&", elems);
	for(size_t i = 0; i < elems.size(); ++i){
		std::vector<std::string> e2;
		StringUtil::split(elems[i], "=", e2);
		if(e2.size() != 2)
			throw FPNN_ERROR_CODE_FMT(FpnnHTTPError, FPNN_EC_CORE_HTTP_ERROR, "Not Support URI(%s)", uri.c_str());
		_httpInfos["u_"+e2[0] ] = e2[1];
	}
	return true;
}

bool HttpParser::parseCookie(const std::string& cookie){
	if(cookie.size() == 0) return true;
	std::vector<std::string> elems;
	StringUtil::split(cookie, ";", elems);
	for(size_t i = 0; i < elems.size(); ++i){
		std::vector<std::string> e2;
		StringUtil::split(elems[i], "=", e2);
		if(e2.size() != 2)
			throw FPNN_ERROR_CODE_FMT(FpnnHTTPError, FPNN_EC_CORE_HTTP_ERROR, "Not Support Cookie(%s)", cookie.c_str());
		_httpInfos["c_"+e2[0] ] = e2[1];
	}
	return true;
}

//-- Just for "\r\n" and "\n", non for "\r".
bool HttpParser::checkHttpHeader(ChainBuffer* cb){
	while (true)
	{   
		int pos = cb->find('\n', _parseOffset);
		if (pos < 0)
		{   
			_parseOffset = cb->length();
			return false;
		}   

		if (pos)
		{   
			if (cb->memcmp("\r\n\r\n", 4, pos - 3)) 
			{   
				_headReceived = true;
				_contentOffset = pos + 1;
				_headerLength = pos - 3;
				return true;
			}   

			if (cb->memcmp("\n\n", 2, pos - 1)) 
			{   
				_headReceived = true;
				_contentOffset = pos + 1;
				_headerLength = pos - 1;
				return true;
			}

			_parseOffset = pos + 1;
		}
	}
	return false;
}

bool HttpParser::isChunckedContentCompleted(ChainBuffer* cb)
{
    int len = cb->length();
    if (cb->memcmp("0\r\n\r\n", 5, len - 5))
        return true;

    if (cb->memcmp("0\n\n", 3, len - 3))
        return true;

    if (cb->memcmp("0\r\r", 3, len - 3))
        return true;

    return false;
}

void HttpParser::reset()
{   
	_chunked = false;
	_contentLen = 0;
	_post = true;

	_method.clear();
	_content.clear();

	_httpInfos.clear();

	_parseOffset = 0;
	_headerLength = 0;
	_contentOffset = 0;
	_headReceived = false;
}   

std::string HttpParser::urldecode(const std::string& uri){
    size_t len = uri.length();
	std::string ret = ""; 
    const char* curi = uri.c_str();
    for (size_t i = 0; i < len; ++i){
        int c = (unsigned char)curi[i];
        if (c == '%' && (i + 2 < len) && isxdigit(curi[i+1]) && isxdigit(curi[i+2])){
            char dst[1];
            unhexlify(dst, &curi[i+1], 2); 
            i += 2;
            ret.push_back(dst[0]);
        }   
        else ret.push_back(c);
    }   
    return ret;
}
