#include <string>
#include <vector>

namespace fpnn {
	std::string formatBytesQuantity(unsigned long long quantity, int outputRankCount = 0);
	void printTable(const std::vector<std::string>& fields, const std::vector<std::vector<std::string>>& rows);
}
