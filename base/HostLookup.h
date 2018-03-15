#ifndef HOSTLOOKUP_H_
#define HOSTLOOKUP_H_
#include <string>
#include <mutex>
#include <unordered_map>

namespace fpnn {
class HostLookup 
{
public:
	static std::string get(const std::string& host);
	static std::string add(const std::string& host);
private:
	static std::string getAddrInfo(const std::string& host);

};
}
#endif
