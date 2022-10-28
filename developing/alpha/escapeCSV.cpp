#include <string>
#include "ChainBuffer.h"

void escapeForCSV(const std::string& src, ChainBuffer& buffer)
{
	size_t i = 0;
	size_t begin = 0;
	bool quotation = false;
	for (; i < src.length(); i++)
	{
		if (src[i] == '"')
		{
			if (quotation == false)
			{
				buffer.append("\"", 1);
				quotation = true;
			}

			buffer.append(src.data() + begin, i - begin);
			buffer.append("\"\"", 2);
			begin = i + 1;
		}
		else if ((src[i] == ',' || src[i] == '\r' || src[i] == '\n') && quotation == false)
		{
			buffer.append("\"", 1);
			quotation = true;
		}
	}

	buffer.append(src.data() + begin, i - begin);
	if (quotation)
		buffer.append("\"", 1);
}