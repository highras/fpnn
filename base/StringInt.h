#ifndef STRINGINT_H_
#define STRINGINT_H_
#include <unordered_map>
#include <string>
#include <mutex>

typedef std::unordered_map<std::string, uint32_t> StringIntMap;
typedef std::unordered_map<uint32_t, std::string> IntStringMap;

class StringInt{
	private:
		static StringIntMap _siMap;
		static IntStringMap _isMap;
		static std::mutex _mtx;

		static uint32_t _idfeed;
	public:
		static uint32_t get(const std::string& str);
		static std::string get(uint32_t id);
};

#endif
