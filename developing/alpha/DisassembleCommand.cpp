#include <ctype.h>
#include "DisassembleCommand.h"

DisassembleCommand::DisassembleCommand(const char* begin, size_t len)
{
	_cmdLen = len;
	_idx = 0;
	_pos = (char*)begin;
}

void DisassembleCommand::fetchString()
{
	char* start = _pos;
	int len = 0;

	while ((_idx < _cmdLen) && (isspace(*_pos) == 0))
	{
		len++;
		_pos++;
		_idx++;
	}

	if (len)
	{
		std::string section(start, len);
		_elem.push_back(section);
	}
}

bool DisassembleCommand::fetchQuotString()
{
	char quot = *_pos;
	bool tranSense = false;

	_pos++;
	_idx++;

	if (_idx >= _cmdLen)
		return false;

	char* start = _pos;
	int len = 0;

	while (_idx < _cmdLen)
	{
		if ((*_pos == quot) && !tranSense)
		{
			_pos++;
			_idx++;
			
			if (len)
			{
				std::string section(start, len);
				_elem.push_back(section);
			}
			else
				_elem.push_back("");

			return true;
		}

		if (*_pos == '\\')
			tranSense = true;
		else
			tranSense = false;

		len++;
		_pos++;
		_idx++;
	}

	return false;
}

bool DisassembleCommand::fetchBlock()
{
	char* start = _pos;
	int len = 0;
	int depth = 0;

	char quot = 0;
	bool tranSense = false;

	_pos++;
	_idx++;
	len++;

	while (_idx < _cmdLen)
	{
		if (*_pos == '"' || *_pos == '\'')
		{
			if (quot == 0)
				quot = *_pos;
			else if (quot == *_pos && !tranSense)
				quot = 0;
		}

		else if (*_pos == '{' && quot == 0)
			depth++;

		else if (*_pos == '}' && quot == 0)
		{
			if (depth)
				depth--;
			else
			{
				_pos++;
				_idx++;
				len++;
				
				std::string section(start, len);
				_elem.push_back(section);

				return true;
			}	
		}

		if (*_pos == '\\')
			tranSense = true;
		else
			tranSense = false;

		len++;
		_pos++;
		_idx++;
	}

	return false;
}

bool DisassembleCommand::disassemble()
{
	while (_idx < _cmdLen)
	{
		if (isspace(*_pos) != 0)
		{
			_pos++;
			_idx++;
		}
		else if (*_pos == '{')
		{
			if (!fetchBlock())
				return false;
		}
		else if (*_pos == '\'' || *_pos == '"')
		{
			if (!fetchQuotString())
				return false;
		}
		else
			fetchString();
	}
	return true;
}

std::vector<std::string> DisassembleCommand::disassemble(const char* begin, size_t len)
{
	DisassembleCommand actor(begin, len);
	actor.disassemble();
	return actor._elem;
}
