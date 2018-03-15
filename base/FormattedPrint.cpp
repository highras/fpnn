#include <iostream>
#include "FormattedPrint.h"

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
