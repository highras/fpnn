#include "FpnnError.h"
#include <stdarg.h>
#include <sstream>

using namespace fpnn;

std::mutex FpnnError::_mutex;

std::string FpnnError::format(const char *fmt, ...){
    std::string s;

    if (fmt){
        va_list ap; 
        va_start(ap, fmt);
		char v[1024];
		vsnprintf(v, sizeof(v), fmt, ap);
        va_end(ap);
		return v;
    }   

    return s;
}

const char* FpnnError::what() const noexcept {
	std::stringstream ss;
	ss << '(' << _code << ')';
	if (_fun && _fun[0])
		ss << " at " << _fun;
	if (_file && _file[0])
		ss << " in " << _file << ':' << _line;
	if (!_message.empty())
		ss << " --- " << _message;

	std::lock_guard<std::mutex> lck(_mutex);
	_what = ss.str();
	return _what.c_str();
}
