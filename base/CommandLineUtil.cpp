#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <inttypes.h>
#include "CommandLineUtil.h"

using namespace fpnn;

//========================================//
//-         Command Line Parser          -//
//========================================//
std::map<std::string, std::string> CommandLineParser::_recognizedParams;
std::vector<std::string> CommandLineParser::_unrecognizedParams;

void CommandLineParser::init(int argc, const char* const * argv, int beginIndex)
{
	char* lastKey = NULL;

	for (; beginIndex < argc; beginIndex++)
	{
		char* start = (char *)argv[beginIndex];
		char* pos = start;
		while (*pos == '-')
			pos++;

		int count = (int)(pos - start);
		if (count == 1 || count == 2)
		{
			lastKey = pos;
			_recognizedParams[pos] = "";
			continue;
		}

		if (lastKey)
		{
			_recognizedParams[lastKey] = start;
			lastKey = NULL;
			continue;
		}

		_unrecognizedParams.push_back(start);
	}
}

std::string CommandLineParser::getString(const std::string& sign, const std::string& dft)
{
	auto iter = _recognizedParams.find(sign);
	if (iter != _recognizedParams.end())
		return iter->second;
	else
		return dft;
}

intmax_t CommandLineParser::getInt(const std::string& sign, intmax_t dft)
{
	std::string value = getString(sign);
	if (value.length())
	{
		char *start = (char*)value.c_str();
		char *endptr;

		errno = 0;
		intmax_t i = strtoimax(start, &endptr, 10);
		
		if (errno == 0 && *endptr == '\0')
			return i;
	}
	return dft;
}

bool CommandLineParser::getBool(const std::string& sign, bool dft)
{
	std::string value = getString(sign);
	if (value.length())
	{
		if (strcasecmp(value.c_str(), "true") == 0 || strcasecmp(value.c_str(), "yes") == 0 || value == "1")
			return true;
		
		if (strcasecmp(value.c_str(), "false") == 0 || strcasecmp(value.c_str(), "no") == 0 || value == "0")
			return false;
	}
	return dft;
}

double CommandLineParser::getReal(const std::string& sign, double dft)
{
	std::string value = getString(sign);
	if (value.length())
	{
		char *start = (char*)value.c_str();
		char *endptr;

		errno = 0;
		double d = strtod(start, &endptr);
		
		if (errno == 0 && *endptr == '\0')
			return d;
	}
	return dft; 
}

bool CommandLineParser::exist(const std::string& sign)
{
	auto iter = _recognizedParams.find(sign);
	if (iter != _recognizedParams.end())
		return true;
	else
		return false;
}

std::vector<std::string> CommandLineParser::getRestParams()
{
	return _unrecognizedParams;
}
