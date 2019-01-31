#include <errno.h>
#include <math.h>
#include <inttypes.h>
#include <stack>
#include "StringUtil.h"
#include "FPJson.h"

namespace fpnn {

class JsonParser
{
	typedef JsonPtr (JsonParser::* MethodFunc)();
	MethodFunc _callMap[256];

	StringUtil::CharsChecker _numberChars;

	std::stack<JsonPtr> _nodeStack;

	std::string _key;
	bool _wantKey;
	bool _wantValue;
	bool _wantComma;
	bool _wantSemicolon;

	char * _pos;

	JsonPtr enterObject()
	{
		if (_wantKey || _wantSemicolon || _wantComma)
			throw FPNN_ERROR_MSG(FpnnJosnInvalidContentError, "Json parser: content error, '{' at improper place.");

		JsonPtr node(new Json());
		node->setDict();
		insertNode(node);
		_nodeStack.push(node);

		_wantKey = true;
		_wantValue = false;
		_wantComma = false;
		_wantSemicolon = false;

		_pos++;
		return nullptr;
	}

	void fillKey(std::string& key)
	{
		_key.swap(key);

		_wantKey = false;
		_wantValue = false;
		_wantComma = false;
		_wantSemicolon = true;

	}

	void fillValue(JsonPtr node)
	{
		if (_nodeStack.top()->addNode(_key, node) == false)
			throw FPNN_ERROR_FMT(FpnnJosnInvalidContentError, "Json parser: content error, reduplicated key: %s.", _key.c_str());
		
		_wantKey = false;
		_wantValue = false;
		_wantComma = true;
		_wantSemicolon = false;
	}

	JsonPtr exitObject()
	{
		if (_wantSemicolon || _wantValue || _nodeStack.empty() || _nodeStack.top()->type() != Json::JSON_Object)
			throw FPNN_ERROR_MSG(FpnnJosnInvalidContentError, "Json parser: content error, '}' at improper place.");

		_pos++;
		return finishNode();
	}

	JsonPtr enterArray()
	{
		if (_wantKey || _wantSemicolon || _wantComma)
			throw FPNN_ERROR_MSG(FpnnJosnInvalidContentError, "Json parser: content error, '[' at improper place.");

		JsonPtr node(new Json());
		node->setArray();
		insertNode(node);
		_nodeStack.push(node);

		_wantKey = false;
		_wantValue = false;
		_wantComma = false;
		_wantSemicolon = false;

		_pos++;
		return nullptr;
	}

	JsonPtr exitArray()
	{
		if (_wantSemicolon || _wantValue || _nodeStack.empty() || _nodeStack.top()->type() != Json::JSON_Array)
			throw FPNN_ERROR_MSG(FpnnJosnInvalidContentError, "Json parser: content error, ']' at improper place.");

		_pos++;
		return finishNode();
	}

	JsonPtr finishNode()
	{
		if (_nodeStack.size() == 1)
		{
			JsonPtr root = _nodeStack.top();
			_nodeStack.pop();
			return root;
		}
		else
		{
			_nodeStack.pop();
			
			_wantKey = false;
			_wantValue = false;
			_wantComma = true;
			_wantSemicolon = false;
		}
		
		return nullptr;
	}

	JsonPtr insertNode(JsonPtr node)
	{
		if (_nodeStack.empty())
			return node;
	
		if (_wantValue)
			fillValue(node);
		else
		{
			_nodeStack.top()->pushNode(node);
			_wantComma = true;
		}
		
		return nullptr;
	}

	std::string fetchString()
	{
		_pos++;
		char *_start = _pos;
		bool slash = false;

		while (true)
		{
			if (*_pos == 0)
				throw FPNN_ERROR_MSG(FpnnJosnInvalidContentError, "Json parser: content error, uncompleted string.");

			else if (*_pos == '\\')
				slash = !slash;
			else if (*_pos == '"' && slash == false)
			{
				std::string str(_start, _pos - _start);
				_pos++;
				return str;
			}
			else
				slash = false;

			_pos++;
		}
	}

	JsonPtr processString()
	{
		if(_wantComma || _wantSemicolon)
				throw FPNN_ERROR_MSG(FpnnJosnInvalidContentError, "Json parser: content error.");

		std::string str = fetchString();
		
		if (_wantKey)
		{
			fillKey(str);
			return nullptr;
		}
		else
		{
			JsonPtr node(new Json(str));
			return insertNode(node);
		}
	}

	JsonPtr processComma()
	{
		if (_wantComma == false)
			throw FPNN_ERROR_MSG(FpnnJosnInvalidContentError, "Json parser: content error, ',' at error position.");

		_wantComma = false;
		if (_nodeStack.top()->type() == Json::JSON_Object)
			_wantKey = true;

		_pos++;
		return nullptr;
	}

	JsonPtr processSemicolon()
	{
		if (_wantSemicolon == false)
			throw FPNN_ERROR_MSG(FpnnJosnInvalidContentError, "Json parser: content error, ':' at error position.");

		_wantSemicolon = false;
		_wantValue = true;
		_pos++;
		return nullptr;
	}

	JsonPtr generalProcess();

	JsonPtr processNumber()
	{
		int dot = 0;
		int e = 0;
		char *_start = _pos;
		while (*_pos != 0 && _numberChars[*_pos])
		{
			if (*_pos == '.')
				dot++;
			else if (*_pos == 'E' || *_pos == 'e')
				e++;

			_pos++;
		}

		if (_start == _pos)
		{
			if (strncasecmp(_pos, "nan", 3) == 0)
			{
				_pos += 3;
				JsonPtr node(new Json(NAN));
				return insertNode(node);
			}
			if (strncasecmp(_pos, "Infinity", 8) == 0)
			{
				_pos += 8;
				JsonPtr node(new Json(INFINITY));
				return insertNode(node);
			}
			if (strncasecmp(_pos, "inf", 3) == 0)
			{
				_pos += 3;
				JsonPtr node(new Json(INFINITY));
				return insertNode(node);
			}
			throw FPNN_ERROR_MSG(FpnnJosnInvalidContentError, "Json parser: content error.");
		}

		if (e > 1)
			throw FPNN_ERROR_MSG(FpnnJosnInvalidContentError, "Json parser: content error, invalid number.");

		bool realType = (dot > 0|| e > 0);
		double d = 0;
		intmax_t i = 0;
		char *endptr = nullptr;

		errno = 0;
		if (realType)
			d = strtod(_start, &endptr);
		else
			i = strtoimax(_start, &endptr, 10);
		
		if (errno || endptr != _pos)
			throw FPNN_ERROR_MSG(FpnnJosnInvalidContentError, "Json parser: content error, invalid number or huge number.");

		JsonPtr node;
		if (realType)
			node.reset(new Json(d));
		else
			node.reset(new Json(i));

		return insertNode(node);
	}

public:
	JsonParser(): _numberChars("0123456789+-.eE"), _pos(0)
	{
		_wantKey = false;
		_wantValue = false;
		_wantComma = false;
		_wantSemicolon = false;

		for (int i = 0; i < 256; i++)
			_callMap[i] = &JsonParser::generalProcess;

		_callMap[(uint8_t)'{'] = &JsonParser::enterObject;
		_callMap[(uint8_t)'}'] = &JsonParser::exitObject;
		_callMap[(uint8_t)'['] = &JsonParser::enterArray;
		_callMap[(uint8_t)']'] = &JsonParser::exitArray;
		_callMap[(uint8_t)','] = &JsonParser::processComma;
		_callMap[(uint8_t)':'] = &JsonParser::processSemicolon;
		_callMap[(uint8_t)'"'] = &JsonParser::processString;
	}

	JsonPtr parse(const char* json);
};

JsonPtr JsonParser::generalProcess()
{
	if(_wantKey || _wantComma || _wantSemicolon)
		throw FPNN_ERROR_MSG(FpnnJosnInvalidContentError, "Json parser: content error.");

	if (strncmp(_pos, "true", 4) == 0)
	{
		_pos += 4;
		JsonPtr node(new Json(true));
		return insertNode(node);
	}

	else if (strncmp(_pos, "false", 5) == 0)
	{
		_pos += 5;
		JsonPtr node(new Json(false));
		return insertNode(node);
	}

	else if (strncmp(_pos, "null", 4) == 0)
	{
		_pos += 4;
		JsonPtr node(new Json(true));
		node->setNull();
		return insertNode(node);
	}
	else
		return processNumber();
}

JsonPtr JsonParser::parse(const char* json)
{
	_pos = (char *)json;
	JsonPtr root;

	while (*_pos != 0)
	{
		while (isspace(*_pos) != 0)
			_pos++;

		if (root || *_pos == 0) break;
		root = (this->*_callMap[(unsigned char)(*_pos)])();
	}

	if (*_pos != 0)
		throw FPNN_ERROR_MSG(FpnnJosnInvalidContentError, "Json parser: content error. Maybe multi-jsons.");

	return root;
}

JsonPtr Json::parse(const char* data) throw(FpnnJosnInvalidContentError)
{
	JsonParser parser;
	return parser.parse(data);
}

}
