#ifndef JSONConvert_h_
#define JSONConvert_h_
#include <sstream>
#include <iostream>
#include <string>
#include "document.h"
#include "msgpack.hpp"

namespace fpnn{

	namespace JSONConvert{
		typedef rapidjson::GenericValue<rapidjson::UTF8<> > FPValue;

		std::string Json2Msgpack(const std::string& jbuf);
		std::string Msgpack2Json(const std::string& mbuf);
		std::string Msgpack2Json(const char* buf, size_t n);
		std::string Msgpack2Json(const msgpack::object& obj);

	}

}
#endif
