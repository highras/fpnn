#include <sstream>
#include "StringInt.h"

StringIntMap StringInt::_siMap;
IntStringMap StringInt::_isMap;
std::mutex StringInt::_mtx;

uint32_t StringInt::_idfeed(0);

uint32_t StringInt::get(const std::string& str){
	std::lock_guard<std::mutex> lck(_mtx);
	auto it = _siMap.find(str);
	if (it == _siMap.end()){
		_siMap.insert(make_pair(str, ++_idfeed));
		_isMap.insert(make_pair(_idfeed, str));
		it = _siMap.find(str);
	}
	return it->second;
}

std::string StringInt::get(uint32_t id){
	std::lock_guard<std::mutex> lck(_mtx);
	auto it = _isMap.find(id);
	if (it != _isMap.end())
		return it->second;
	std::ostringstream ss;
	ss << "Can not get String of ID:" << id;
	throw std::runtime_error(ss.str());
}
