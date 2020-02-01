#include <string>
#include <vector>

namespace fpnn {
	std::string formatBytesQuantity(unsigned long long quantity, int outputRankCount = 0);
	std::string visibleBinaryBuffer(const void* memory, size_t size, const std::string& delim = " ");
	void printTable(const std::vector<std::string>& fields, const std::vector<std::vector<std::string>>& rows);
}
