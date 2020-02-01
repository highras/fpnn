#include <iostream>
#include <sstream>
#include "FormattedPrint.h"

std::string fpnn::formatBytesQuantity(unsigned long long quantity, int outputRankCount)
{
	const char sign[8] = {'Z', 'E', 'P', 'T', 'G', 'M', 'K', 'B'};
	int sections[8];

	if (quantity == 0)
		return "0 B";

	for (int i = 0; i < 8; i++)
	{
		unsigned long long upLevel = quantity / 1024;
		sections[7 - i] = (int)(quantity - upLevel * 1024);
		quantity = upLevel;
	}

	if (outputRankCount < 1 || outputRankCount > 8)
		outputRankCount = 8;

	std::string res;
	for (int i = 0; i < 8; i++)
	{
		if (sections[i] == 0)
			continue;

		if (res.size())
			res.append(" ");

		res.append(std::to_string(sections[i])).append(" ").append(1, sign[i]);
		outputRankCount -= 1;

		if (outputRankCount == 0)
			break;
	}

	if (res[res.length() - 1] != 'B')
		res.append("B");

	return res;
}

std::string fpnn::visibleBinaryBuffer(const void* memory, size_t size, const std::string& delim)
{
	std::stringstream ss;
	const char* data = (char*)memory;

	for (size_t i = 0; i < size; i++)
	{
		if (i > 0 && i % 8 == 0)
			ss<<delim;

		char c = data[i];
		if (c < '!' || c > '~')
			ss<<'.';
		else
			ss<<c;
	}

	return ss.str();
}

void fpnn::printTable(const std::vector<std::string>& fields, const std::vector<std::vector<std::string>>& rows)
{
	std::vector<size_t> fieldLens(fields.size(), 0);

	for (size_t i = 0; i < fields.size(); ++i)
		if (fields[i].length() > fieldLens[i])
			fieldLens[i] = fields[i].length();

	for (size_t i = 0; i < rows.size(); ++i)
		for(size_t j = 0; j < rows[i].size(); ++j)
			if (rows[i][j].length() > fieldLens[j])
				fieldLens[j] = rows[i][j].length();

	//-- top
	std::cout<<"+";
	for (size_t i = 0; i < fieldLens.size(); i++)
		std::cout<<std::string(fieldLens[i] + 2, '-')<<'+';
	std::cout<<std::endl;

	//-- fiels
	std::cout<<"|";
	for(size_t i = 0; i < fields.size(); ++i)
	{
		std::cout<<' '<<fields[i];
		if (fields[i].length() < fieldLens[i])
			std::cout<<std::string(fieldLens[i] - fields[i].length(), ' ');

		std::cout<<" |";
	}
	std::cout<<std::endl;

	//-- separator
	std::cout<<"+";
	for (size_t i = 0; i < fieldLens.size(); i++)
		std::cout<<std::string(fieldLens[i] + 2, '=')<<'+';
	std::cout<<std::endl;

	//-- data
	for (size_t i = 0; i < rows.size(); ++i)
	{
		std::cout<<"|";
		for(size_t j = 0; j < rows[i].size(); ++j)
		{
			std::cout<<' '<<rows[i][j];
			if (rows[i][j].length() < fieldLens[j])
				std::cout<<std::string(fieldLens[j] - rows[i][j].length(), ' ');

			std::cout<<" |";
		}
		std::cout<<std::endl;
	}

	//-- tail line
	std::cout<<"+";
	for (size_t i = 0; i < fieldLens.size(); i++)
		std::cout<<std::string(fieldLens[i] + 2, '-')<<'+';
	std::cout<<std::endl;

	std::cout<<rows.size()<<" rows in results."<<std::endl;
}
