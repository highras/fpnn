#include <stack>
#include <iomanip>
//#include "StringUtil.h"
#include "FPJson.h"

using namespace fpnn;

//======================================//
//-          Assistant Class           -//
//======================================//
static const char* gc_JsonStringEscaper_HexCharsArray = "0123456789abcdef";

class JsonStringEscaper
{
	typedef void (JsonStringEscaper::* MethodFunc)(std::ostream& os, const char * start, int len, int & idx);
	MethodFunc _callMap[256];

	void hexadecimal(std::ostream& os, const char * start, int len, int & idx)
	{
		unsigned char c = (unsigned char)start[idx];
		os << "\\u00" << *(gc_JsonStringEscaper_HexCharsArray + (c >> 4)) << *(gc_JsonStringEscaper_HexCharsArray + (c & 0xf));
	}
	void quotationMarks(std::ostream& os, const char * start, int len, int & idx)
	{
		os << "\\\"";
	}
	void slash(std::ostream& os, const char * start, int len, int & idx)
	{
		os << "\\\\";
		/*
		if (idx + 1 < len
			&& (start[idx+1] == '"'
				|| start[idx+1] == '\\'
				|| start[idx+1] == '/'
				|| start[idx+1] == 'b'
				|| start[idx+1] == 'f'
				|| start[idx+1] == 'n'
				|| start[idx+1] == 'r'
				|| start[idx+1] == 't'
				|| start[idx+1] == 'u'
				))
		{
			os << "\\"<<start[idx+1];
			idx += 1;
		}
		else
			os << "\\\\";
		*/

	}
	void jsonSpecialChars(std::ostream& os, const char * start, int len, int & idx)
	{
		switch (start[idx])
		{
			case '\b': os << "\\b"; break;
			case '\f': os << "\\f"; break;
			case '\n': os << "\\n"; break;
			case '\r': os << "\\r"; break;
			case '\t': os << "\\t"; break;
		}
	}
	void normal(std::ostream& os, const char * start, int len, int & idx)
	{
		os << start[idx];
	}

public:
	JsonStringEscaper()
	{
		for (int i = 0; i < 256; i++)
		{
			if (i > 0x1f && i < 0x7f)
				_callMap[i] = &JsonStringEscaper::normal;
			else
				_callMap[i] = &JsonStringEscaper::hexadecimal;
		}

		_callMap[(uint8_t)'\\'] = &JsonStringEscaper::slash;
		_callMap[(uint8_t)'"'] = &JsonStringEscaper::quotationMarks;

		_callMap[(uint8_t)'\b'] = &JsonStringEscaper::jsonSpecialChars;
		_callMap[(uint8_t)'\f'] = &JsonStringEscaper::jsonSpecialChars;
		_callMap[(uint8_t)'\n'] = &JsonStringEscaper::jsonSpecialChars;
		_callMap[(uint8_t)'\r'] = &JsonStringEscaper::jsonSpecialChars;
		_callMap[(uint8_t)'\t'] = &JsonStringEscaper::jsonSpecialChars;
	}

	std::string escape(const std::string * const str)
	{
		const uint8_t mark2 = 0xe0;	//-- 1110 0000
		const uint8_t mark3 = 0xf0;	//-- 1111 0000
		const uint8_t mark4 = 0xf8;	//-- 1111 1000
		const uint8_t mark5 = 0xfc;	//-- 1111 1100
		const uint8_t mark6 = 0xfe;	//-- 1111 1110

		const uint8_t utf82 = 0xc0;	//-- 1100 0000
		const uint8_t utf83 = 0xe0;	//-- 1110 0000
		const uint8_t utf84 = 0xf0;	//-- 1111 0000
		const uint8_t utf85 = 0xf8;	//-- 1111 1000
		const uint8_t utf86 = 0xfc;	//-- 1111 1100

		std::ostringstream os;

		int len = (int)str->length();
		const char* p = str->data();
    	for (int i = 0; i < len; i++)
    	{
    		if ((p[i] & 0x80) == 0x00)
    			(this->*_callMap[(unsigned char)(p[i])])(os, p, len, i);
    		else
    		{
    			if ((p[i] & mark2) == utf82 && i + 1 < len)
				{
					os << p[i] << p[i+1];
					i += 1;
				}
				else if ((p[i] & mark3) == utf83 && i + 2 < len)
				{
					os << p[i] << p[i+1] << p[i+2];
					i += 2;
				}
				else if ((p[i] & mark4) == utf84 && i + 3 < len)
				{
					os << p[i] << p[i+1] << p[i+2] << p[i+3];
					i += 3;
				}
				else if ((p[i] & mark5) == utf85 && i + 4 < len)
				{
					os << p[i] << p[i+1] << p[i+2] << p[i+3] << p[i+4];
					i += 4;
				}
				else if ((p[i] & mark6) == utf86 && i + 5 < len)
				{
					os << p[i] << p[i+1] << p[i+2] << p[i+3] << p[i+4] << p[i+5];
					i += 5;
				}
				else
					(this->*_callMap[(unsigned char)(p[i])])(os, p, len, i);
    		}
    	}

    	return os.str();
	}
};

JsonStringEscaper gc_JsonStringEscaper;

//======================================//
//-        Assistant Functions         -//
//======================================//
std::ostream& fpnn::operator << (std::ostream& os, const Json& node)
{
	return node.output(os);
}

std::ostream& fpnn::operator << (std::ostream& os, const JsonPtr node)
{
	return node->output(os);
}

std::string Json::str()
{
	std::stringstream ss;
	output(ss);

	return ss.str();
}

std::ostream& Json::output(std::ostream& os) const
{
	if (_type == JSON_String)
	{
		std::string * str = (std::string*)_data;
		os << '"' << gc_JsonStringEscaper.escape(str) << '"';
	}
	else if (_type == JSON_Integer)
	{
		intmax_t * data = (intmax_t*)_data;
		os << (*data);
	}
	else if (_type == JSON_UInteger)
	{
		uintmax_t * data = (uintmax_t*)_data;
		os << (*data);
	}
	else if (_type == JSON_Object)
	{
		std::map<std::string, JsonPtr>* data = (std::map<std::string, JsonPtr>*)_data;
		os << "{";
		
		bool first = true;
		for (auto& pair: *data)
		{
			if (first)
				first = false;
			else
				os << ", ";

			os << '"' << gc_JsonStringEscaper.escape(&(pair.first)) << "\":";
			pair.second->output(os);
		}

		os << "}";
	}
	else if (_type == JSON_Array)
	{
		std::list<JsonPtr>* data = (std::list<JsonPtr>*)_data;
		os << "[";
		
		bool first = true;
		for (JsonPtr node: *data)
		{
			if (first)
				first = false;
			else
				os << ", ";

			node->output(os);
		}

		os << "]";
	}
	else if (_type == JSON_Real)
	{
		double * data = (double*)_data;
		os << (*data);
	}
	else if (_type == JSON_Boolean)
	{
		os << ((bool)_data ? "true" : "false");
	}
	else if (_type == JSON_Null)
	{
		os << "null";
	}
	else if (_type == JSON_Uninit)
	{
		os << "null";
	}

	return os;
}

//======================================//
//-          Fetch/get Nodes           -//
//======================================//
std::vector<std::string>& pathSplit(const std::string& s, const std::string& delim, std::vector<std::string> &elems)
{

	std::string::size_type start = 0;
	while (true)
	{
		if (start == s.size())
		{
			elems.push_back("");
			return elems;
		}

		std::string::size_type pos = s.find_first_of(delim, start);
		if (pos == start)
		{
			elems.push_back("");
			start += 1;
		}
		else if (pos != std::string::npos)
		{
			std::string tmp = s.substr(start, pos - start);
			elems.push_back(tmp);
			start = pos + 1;
		}
		else
		{
			std::string tmp = s.substr(start);
			elems.push_back(tmp);
			return elems;
		}
	}
}

JsonPtr Json::getNodeByKey(const std::string& key)
{
	if (_type != JSON_Object)
		return nullptr;

	std::map<std::string, JsonPtr>* data = (std::map<std::string, JsonPtr>*)_data;
	auto it = data->find(key);
	if (it != data->end())
		return it->second;
	else
		return nullptr;
}

JsonPtr Json::getNodeByIndex(int index)
{
	if (_type != JSON_Array)
		return nullptr;

	std::list<JsonPtr>* data = (std::list<JsonPtr>*)_data;
	auto it = data->begin();
	int i = 0;
	for (; it != data->end(); it++, i++)
	{
		if (i == index)
			return *it;
	}

	return nullptr;
}

JsonPtr Json::getNode(const std::string& path, const std::string& delim)
{
	std::vector<std::string> sections;
	pathSplit(path, delim, sections);

	if (sections.empty())
		return getNodeByKey("");

	JsonPtr node = getNodeByKey(sections[0]);
	for (size_t i = 1; i < sections.size() && node != nullptr; i++)
		node = node->getNodeByKey(sections[i]);

	return node;
}

JsonPtr Json::getParentNode(const std::string& path, const std::string& delim, std::string& lastSection, bool& nodeTypeError, bool& nodeNotFound)
{
	nodeTypeError = false;
	nodeNotFound = false;

	std::vector<std::string> sections;
	pathSplit(path, delim, sections);

	if (sections.empty())
	{
		lastSection = "";
		return nullptr;
	}

	JsonPtr node;
	for (int i = 0; i < (int)sections.size() - 1; i++)
	{
		JsonPtr next = (i>0) ? node->getNodeByKey(sections[i]) : getNodeByKey(sections[0]);
		if (next == nullptr)
		{
			enum ElementType nodeType = (i>0) ? node->type() : type();
			if (nodeType == JSON_Object)
			{
				nodeNotFound = true;
				lastSection = sections[i];
				return nullptr;
			}
			else
			{
				nodeTypeError = true;
				lastSection = sections[i];
				return nullptr;
			}
		}
		else
			node = next;
	}

	lastSection = sections[sections.size() - 1];
	return node;
}

JsonPtr Json::createNode(const std::string& path, const std::string& delim, bool throwable)
{
	std::vector<std::string> sections;
	pathSplit(path, delim, sections);

	if (sections.empty())
		sections.push_back("");

	JsonPtr node;
	size_t i = 0;
	for (; i < sections.size(); i++)
	{
		JsonPtr next = (i>0) ? node->getNodeByKey(sections[i]) : getNodeByKey(sections[0]);
		if (next == nullptr)
		{
			enum ElementType nodeType = (i>0) ? node->type() : type();
			if (nodeType == JSON_Object)
				break;				//-- not found
			else if (nodeType == JSON_Uninit)
			{
				if (i < sections.size() - 1)
				{
					if (i > 0)
						node->setDict();
					else
						setDict();
				}
				break;
			}
			else
			{
				if (throwable)
					throw FPNN_ERROR_FMT(FpnnJsonNodeTypeMissMatchError, "Section %d (%s)(base 0) in path is not object.", (int)i, sections[i].c_str());

				return nullptr;
			}
		}
		else
			node = next;
	}

	for (; i < sections.size() - 1; i++)
	{
		node = (i > 0) ? node->dictAddObject(sections[i]) : dictAddObject(sections[0]);
	}

	if (i == sections.size() - 1)
	{
		JsonPtr n(new Json());
		if (i > 0)
			node->addNode(sections[i], n);
		else
			addNode(sections[0], n);

		return n;
	}

	return node;
}

//======================================//
//-     Constructor & destructor       -//
//======================================//
void Json::clean()
{
	switch (_type)
	{
		case JSON_Uninit:
			return;

		case JSON_Object:
		{
			std::map<std::string, JsonPtr>* data = (std::map<std::string, JsonPtr>*)_data;
			delete data;
			break;
		}

		case JSON_Array:
		{
			std::list<JsonPtr>* data = (std::list<JsonPtr>*)_data;
			delete data;
			break;
		}

		case JSON_String:
		{
			std::string * data = (std::string*)_data;
			delete data;
			break;
		}

		case JSON_Integer:
		{
			intmax_t * data = (intmax_t*)_data;
			delete data;
			break;
		}

		case JSON_UInteger:
		{
			uintmax_t * data = (uintmax_t*)_data;
			delete data;
			break;
		}

		case JSON_Real:
		{
			double * data = (double*)_data;
			delete data;
			break;
		}

		case JSON_Boolean:
		case JSON_Null:
			break;
	}

	_type = JSON_Uninit;
	_data = NULL;
}

//======================================//
//-        Baisc set functions         -//
//======================================//
void Json::setNull()
{
	if (_type == JSON_Null)
		return;

	clean();
	_type = JSON_Null;
}

void Json::setBool(bool value)
{
	if (_type != JSON_Boolean)
	{
		clean();
		_type = JSON_Boolean;
	}

	_data = (value ? (void*)1 : 0);
}

void Json::setInt(intmax_t value)
{
	if (_type != JSON_Integer)
	{
		clean();
		_type = JSON_Integer;
		_data = new intmax_t(value);
	}
	else
	{
		intmax_t *data = (intmax_t*)_data;
		*data = value;
	}
}

void Json::setUInt(uintmax_t value)
{
	if (_type != JSON_UInteger)
	{
		clean();
		_type = JSON_UInteger;
		_data = new uintmax_t(value);
	}
	else
	{
		uintmax_t *data = (uintmax_t*)_data;
		*data = value;
	}
}

void Json::setReal(double value)
{
	if (_type != JSON_Real)
	{
		clean();
		_type = JSON_Real;
		_data = new double(value);
	}
	else
	{
		double * data = (double*)_data;
		*data = value;
	}
}

void Json::setString(const char* value)
{
	if (_type != JSON_String)
	{
		clean();
		_type = JSON_String;
		if (value)
			_data = new std::string(value);
		else
			_data = new std::string("");
	}
	else
	{
		std::string * data = (std::string*)_data;
		if (value)
			data->assign(value);
		else
			data->assign("");
	}
}

void Json::setString(const std::string& value)
{
	if (_type != JSON_String)
	{
		clean();
		_type = JSON_String;
		_data = new std::string(value);
	}
	else
	{
		std::string * data = (std::string*)_data;
		data->assign(value);
	}
}

void Json::setArray()
{
	if (_type != JSON_Array)
	{
		clean();
		_type = JSON_Array;
		_data = new std::list<JsonPtr>();
	}
	else
	{
		std::list<JsonPtr>* data = (std::list<JsonPtr>*)_data;
		data->clear();
	}
}

void Json::setDict()
{
	if (_type != JSON_Object)
	{
		clean();
		_type = JSON_Object;
		_data = new std::map<std::string, JsonPtr>();
	}
	else
	{
		std::map<std::string, JsonPtr>* data = (std::map<std::string, JsonPtr>*)_data;
		data->clear();
	}
}

//======================================//
//-       Basic push functions         -//
//======================================//
bool Json::pushNode(JsonPtr node)
{
	if (_type != JSON_Array)
	{
		if (_type == JSON_Uninit)
			setArray();
		else
			return false;
	}

	std::list<JsonPtr>* data = (std::list<JsonPtr>*)_data;
	data->push_back(node);
	return true;
}

bool Json::pushNull()
{
	JsonPtr node(new Json());
	node->setNull();
	return pushNode(node);
}

JsonPtr Json::pushArray()
{
	JsonPtr node(new Json());
	node->setArray();
	return (pushNode(node) ? node : nullptr);
}

JsonPtr Json::pushDict()
{
	JsonPtr node(new Json());
	node->setDict();
	return (pushNode(node) ? node : nullptr);
}

//======================================//
//-      Pathlized push Function       -//
//======================================//
void Json::pushNull(const std::string& path, const std::string& delim)
{
	JsonPtr node = createNode(path, delim, true);
	if (node->pushNull())
		return;

	throw FPNN_ERROR_MSG(FpnnJsonNodeTypeMissMatchError, "Target node is not array.");
}

void Json::pushBool(const std::string& path, bool value, const std::string& delim)
{
	JsonPtr node = createNode(path, delim, true);
	if (node->pushBool(value))
		return;

	throw FPNN_ERROR_MSG(FpnnJsonNodeTypeMissMatchError, "Target node is not array.");
}

void Json::pushReal(const std::string& path, double value, const std::string& delim)
{
	JsonPtr node = createNode(path, delim, true);
	if (node->pushReal(value))
		return;

	throw FPNN_ERROR_MSG(FpnnJsonNodeTypeMissMatchError, "Target node is not array.");
}

void Json::pushInt(const std::string& path, intmax_t value, const std::string& delim)
{
	JsonPtr node = createNode(path, delim, true);
	if (node->pushInt(value))
		return;

	throw FPNN_ERROR_MSG(FpnnJsonNodeTypeMissMatchError, "Target node is not array.");
}

void Json::pushUInt(const std::string& path, uintmax_t value, const std::string& delim)
{
	JsonPtr node = createNode(path, delim, true);
	if (node->pushUInt(value))
		return;

	throw FPNN_ERROR_MSG(FpnnJsonNodeTypeMissMatchError, "Target node is not array.");
}

void Json::pushString(const std::string& path, const char* value, const std::string& delim)
{
	JsonPtr node = createNode(path, delim, true);
	if (node->pushString(value))
		return;

	throw FPNN_ERROR_MSG(FpnnJsonNodeTypeMissMatchError, "Target node is not array.");
}

void Json::pushString(const std::string& path, const std::string& value, const std::string& delim)
{
	JsonPtr node = createNode(path, delim, true);
	if (node->pushString(value))
		return;

	throw FPNN_ERROR_MSG(FpnnJsonNodeTypeMissMatchError, "Target node is not array.");
}

JsonPtr Json::pushArray(const std::string& path, const std::string& delim)
{
	JsonPtr node = createNode(path, delim, true);
	node = node->pushArray();
	if (node) return node;

	throw FPNN_ERROR_MSG(FpnnJsonNodeTypeMissMatchError, "Target node is not array.");
}

JsonPtr Json::pushDict(const std::string& path, const std::string& delim)
{
	JsonPtr node = createNode(path, delim, true);
	node = node->pushDict();
	if (node) return node;

	throw FPNN_ERROR_MSG(FpnnJsonNodeTypeMissMatchError, "Target node is not array.");
}

//======================================//
//-        Basic add functions         -//
//======================================//
bool Json::addNode(const std::string& key, JsonPtr node)
{
	if (_type != JSON_Object)
	{
		if (_type == JSON_Uninit)
			setDict();
		else
			return false;
	}

	std::map<std::string, JsonPtr>* data = (std::map<std::string, JsonPtr>*)_data;
	auto it = data->find(key);
	if (it != data->end())
		return false;

	(*data)[key] = node;
	return true;
}

/*
bool Json::dictAddNull(const std::string& key)
{
	JsonPtr node(new Json());
	node->setNull();
	return addNode(key, node);
}

JsonPtr Json::dictAddArray(const std::string& key)
{
	JsonPtr node(new Json());
	node->setArray();
	return (addNode(key, node) ? node : nullptr);
}*/

JsonPtr Json::dictAddObject(const std::string& key)
{
	JsonPtr node(new Json());
	node->setDict();
	return (addNode(key, node) ? node : nullptr);
}

//======================================//
//-     Pathlized add Function       -//
//======================================//
void Json::addBool(const std::string& path, bool value, const std::string& delim)
{
	JsonPtr node = createNode(path, delim, true);
	if (node->type() == JSON_Uninit)
		node->setBool(value);
	else
		throw FPNN_ERROR_MSG(FpnnJosnNodeExistError, "Node has existed.");
}

void Json::addReal(const std::string& path, double value, const std::string& delim)
{
	JsonPtr node = createNode(path, delim, true);
	if (node->type() == JSON_Uninit)
		node->setReal(value);
	else
		throw FPNN_ERROR_MSG(FpnnJosnNodeExistError, "Node has existed.");
}

void Json::addInt(const std::string& path, intmax_t value, const std::string& delim)
{
	JsonPtr node = createNode(path, delim, true);
	if (node->type() == JSON_Uninit)
		node->setInt(value);
	else
		throw FPNN_ERROR_MSG(FpnnJosnNodeExistError, "Node has existed.");
}

void Json::addUInt(const std::string& path, uintmax_t value, const std::string& delim)
{
	JsonPtr node = createNode(path, delim, true);
	if (node->type() == JSON_Uninit)
		node->setUInt(value);
	else
		throw FPNN_ERROR_MSG(FpnnJosnNodeExistError, "Node has existed.");
}

void Json::addString(const std::string& path, const char* value, const std::string& delim)
{
	JsonPtr node = createNode(path, delim, true);
	if (node->type() == JSON_Uninit)
		node->setString(value);
	else
		throw FPNN_ERROR_MSG(FpnnJosnNodeExistError, "Node has existed.");
}

void Json::addString(const std::string& path, const std::string& value, const std::string& delim)
{
	JsonPtr node = createNode(path, delim, true);
	if (node->type() == JSON_Uninit)
		node->setString(value);
	else
		throw FPNN_ERROR_MSG(FpnnJosnNodeExistError, "Node has existed.");
}

void Json::addNull(const std::string& path, const std::string& delim)
{
	JsonPtr node = createNode(path, delim, true);
	if (node->type() == JSON_Uninit)
		node->setNull();
	else
		throw FPNN_ERROR_MSG(FpnnJosnNodeExistError, "Node has existed.");
}

JsonPtr Json::addArray(const std::string& path, const std::string& delim)
{
	JsonPtr node = createNode(path, delim, true);
	if (node->type() == JSON_Uninit)
	{
		node->setArray();
		return node;
	}
	else
		throw FPNN_ERROR_MSG(FpnnJosnNodeExistError, "Node has existed.");
}

JsonPtr Json::addDict(const std::string& path, const std::string& delim)
{
	JsonPtr node = createNode(path, delim, true);
	if (node->type() == JSON_Uninit)
	{
		node->setDict();
		return node;
	}
	else
		throw FPNN_ERROR_MSG(FpnnJosnNodeExistError, "Node has existed.");
}

//======================================//
//-         Remove sub-Node            -//
//======================================//
bool Json::remove(int index)
{
	if (_type != JSON_Array)
		return false;

	std::list<JsonPtr>* data = (std::list<JsonPtr>*)_data;
	auto it = data->begin();
	int i = 0;
	for (; it != data->end(); it++, i++)
	{
		if (i == index)
		{
			data->erase(it);
			return true;	
		}
	}

	return true;
}

bool Json::remove(const std::string& path, const std::string& delim)
{
	std::string lastSection;
	bool nodeTypeError, nodeNotFound;
	JsonPtr node = getParentNode(path, delim, lastSection, nodeTypeError, nodeNotFound);

	if (node)
	{
		std::map<std::string, JsonPtr>* data = (std::map<std::string, JsonPtr>*)(node->_data);
		data->erase(lastSection);
		return true;
	}

	if (nodeNotFound)
		return true;

	if (nodeTypeError == false && nodeNotFound == false)
	{
		std::map<std::string, JsonPtr>* data = (std::map<std::string, JsonPtr>*)_data;
		data->erase(lastSection);
		return true;
	}
	
	return false;
}

//======================================//
//-         Member checking            -//
//======================================//
bool Json::isNull(const std::string& path, const std::string& delim) noexcept
{
	JsonPtr node = getNode(path, delim);
	return (node && node->isNull());
}

enum Json::ElementType Json::type(const std::string& path, const std::string& delim)
{
	JsonPtr node = getNode(path, delim);
	if (node)
		return node->type();
	
	throw FPNN_ERROR_MSG(FpnnJosnNodeNotExistError, "Node is not exist.");
}

//======================================//
//-         Baisc get Function         -//
//======================================//
bool Json::getBool(bool dft) const
{
	if (_type == JSON_Boolean)
		return (bool)_data;

	return dft;
}

intmax_t Json::getInt(intmax_t dft) const
{
	if (_type == JSON_Integer)
		return *((intmax_t*)_data);

	if (_type == JSON_UInteger)
	{
		intmax_t v = (intmax_t)(*((uintmax_t*)_data));
		if (v >= 0)
			return v;
	}

	return dft;
}

uintmax_t Json::getUInt(uintmax_t dft) const
{
	if (_type == JSON_UInteger)
		return *((uintmax_t*)_data);

	if (_type == JSON_Integer)
	{
		intmax_t v = *((intmax_t*)_data);
		if (v >= 0)
			return (uintmax_t)v;
	}

	return dft;
}

double Json::getReal(double dft) const
{
	if (_type == JSON_Real)
		return *((double*)_data);

	return dft;
}

std::string Json::getString(const std::string& dft) const
{
	if (_type == JSON_String)
		return *((std::string*)_data);

	return dft;
}

const std::list<JsonPtr> * const Json::getList() const
{
	if (_type == JSON_Array)
		return (std::list<JsonPtr>*)_data;

	return NULL;
}

const std::map<std::string, JsonPtr> * const Json::getDict() const
{
	if (_type == JSON_Object)
		return (std::map<std::string, JsonPtr>*)_data;

	return NULL;
}

//======================================//
//-        Baisc want Function         -//
//======================================//
bool Json::wantBool() const
{
	if (_type == JSON_Boolean)
		return (bool)_data;

	throw FPNN_ERROR_MSG(FpnnJsonNodeTypeMissMatchError, "Node type miss match.");
}

intmax_t Json::wantInt() const
{
	if (_type == JSON_Integer)
		return *((intmax_t*)_data);

	if (_type == JSON_UInteger)
	{
		intmax_t v = (intmax_t)(*((uintmax_t*)_data));
		if (v >= 0)
			return v;
	}

	throw FPNN_ERROR_MSG(FpnnJsonNodeTypeMissMatchError, "Node type miss match.");
}

uintmax_t Json::wantUInt() const
{
	if (_type == JSON_UInteger)
		return *((uintmax_t*)_data);

	if (_type == JSON_Integer)
	{
		intmax_t v = *((intmax_t*)_data);
		if (v >= 0)
			return (uintmax_t)v;
	}

	throw FPNN_ERROR_MSG(FpnnJsonNodeTypeMissMatchError, "Node type miss match.");
}

double Json::wantReal() const
{
	if (_type == JSON_Real)
		return *((double*)_data);

	throw FPNN_ERROR_MSG(FpnnJsonNodeTypeMissMatchError, "Node type miss match.");
}

std::string Json::wantString() const
{
	if (_type == JSON_String)
		return *((std::string*)_data);

	throw FPNN_ERROR_MSG(FpnnJsonNodeTypeMissMatchError, "Node type miss match.");
}


std::vector<bool> Json::wantBoolVector() const
{
	if (_type != JSON_Array)
		throw FPNN_ERROR_MSG(FpnnJsonNodeTypeMissMatchError, "Node type miss match.");

	std::list<JsonPtr>* data = (std::list<JsonPtr>*)_data;
	std::vector<bool> res;
	res.reserve(data->size());

	for (auto& node: *data)
		res.push_back(node->wantBool());

	return res;
}

std::vector<double> Json::wantRealVector() const
{
	if (_type != JSON_Array)
		throw FPNN_ERROR_MSG(FpnnJsonNodeTypeMissMatchError, "Node type miss match.");

	std::list<JsonPtr>* data = (std::list<JsonPtr>*)_data;
	std::vector<double> res;
	res.reserve(data->size());

	for (auto& node: *data)
		res.push_back(node->wantReal());

	return res;
}

std::vector<intmax_t> Json::wantIntVector() const
{
	if (_type != JSON_Array)
		throw FPNN_ERROR_MSG(FpnnJsonNodeTypeMissMatchError, "Node type miss match.");

	std::list<JsonPtr>* data = (std::list<JsonPtr>*)_data;
	std::vector<intmax_t> res;
	res.reserve(data->size());

	for (auto& node: *data)
		res.push_back(node->wantInt());

	return res;
}

std::vector<std::string> Json::wantStringVector() const
{
	if (_type != JSON_Array)
		throw FPNN_ERROR_MSG(FpnnJsonNodeTypeMissMatchError, "Node type miss match.");

	std::list<JsonPtr>* data = (std::list<JsonPtr>*)_data;
	std::vector<std::string> res;
	res.reserve(data->size());

	for (auto& node: *data)
		res.push_back(node->wantString());

	return res;
}

std::map<std::string, bool> Json::wantBoolDict() const
{
	if (_type != JSON_Object)
		throw FPNN_ERROR_MSG(FpnnJsonNodeTypeMissMatchError, "Node type miss match.");

	std::map<std::string, JsonPtr>* data = (std::map<std::string, JsonPtr>*)_data;
	std::map<std::string, bool> res;

	for (auto& npair: *data)
		res[npair.first] = npair.second->wantBool();

	return res;
}

std::map<std::string, double> Json::wantRealDict() const
{
	if (_type != JSON_Object)
		throw FPNN_ERROR_MSG(FpnnJsonNodeTypeMissMatchError, "Node type miss match.");

	std::map<std::string, JsonPtr>* data = (std::map<std::string, JsonPtr>*)_data;
	std::map<std::string, double> res;

	for (auto& npair: *data)
		res[npair.first] = npair.second->wantReal();

	return res;
}

std::map<std::string, intmax_t> Json::wantIntDict() const
{
	if (_type != JSON_Object)
		throw FPNN_ERROR_MSG(FpnnJsonNodeTypeMissMatchError, "Node type miss match.");

	std::map<std::string, JsonPtr>* data = (std::map<std::string, JsonPtr>*)_data;
	std::map<std::string, intmax_t> res;

	for (auto& npair: *data)
		res[npair.first] = npair.second->wantInt();

	return res;
}

std::map<std::string, std::string> Json::wantStringDict() const
{
	if (_type != JSON_Object)
		throw FPNN_ERROR_MSG(FpnnJsonNodeTypeMissMatchError, "Node type miss match.");

	std::map<std::string, JsonPtr>* data = (std::map<std::string, JsonPtr>*)_data;
	std::map<std::string, std::string> res;

	for (auto& npair: *data)
		res[npair.first] = npair.second->wantString();

	return res;
}

//======================================//
//-       Pathlized get Function       -//
//======================================//
bool Json::getBool(const std::string& path, bool dft, const std::string& delim)
{
	JsonPtr node = getNode(path, delim);
	if (node)
		return node->getBool(dft);
	else
		return dft;
}

intmax_t Json::getInt(const std::string& path, intmax_t dft, const std::string& delim)
{
	JsonPtr node = getNode(path, delim);
	if (node)
		return node->getInt(dft);
	else
		return dft;
}

uintmax_t Json::getUInt(const std::string& path, uintmax_t dft, const std::string& delim)
{
	JsonPtr node = getNode(path, delim);
	if (node)
		return node->getUInt(dft);
	else
		return dft;
}

double Json::getReal(const std::string& path, double dft, const std::string& delim)
{
	JsonPtr node = getNode(path, delim);
	if (node)
		return node->getReal(dft);
	else
		return dft;
}

std::string Json::getStringAt(const std::string& path, const std::string& dft, const std::string& delim)
{
	JsonPtr node = getNode(path, delim);
	if (node)
		return node->getString(dft);
	else
		return dft;
}

/*
	JsonPtr Json::getNode(const std::string& path, const std::string& delim);
	Implemented at "Fetch/get Nodes" part.
*/
	
const std::list<JsonPtr> * const Json::getList(const std::string& path, const std::string& delim)
{
	JsonPtr node = getNode(path, delim);
	if (node)
		return node->getList();
	else
		return NULL;
}

const std::map<std::string, JsonPtr> * const Json::getDict(const std::string& path, const std::string& delim)
{
	JsonPtr node = getNode(path, delim);
	if (node)
		return node->getDict();
	else
		return NULL;
}

//======================================//
//-      Pathlized want Function       -//
//======================================//
bool Json::wantBool(const std::string& path, const std::string& delim)
{
	JsonPtr node = getNode(path, delim);
	if (node)
		return node->wantBool();
	
	throw FPNN_ERROR_MSG(FpnnJosnNodeNotExistError, "Target node doesn't exist.");
}

intmax_t Json::wantInt(const std::string& path, const std::string& delim)
{
	JsonPtr node = getNode(path, delim);
	if (node)
		return node->wantInt();
	
	throw FPNN_ERROR_MSG(FpnnJosnNodeNotExistError, "Target node doesn't exist.");
}

uintmax_t Json::wantUInt(const std::string& path, const std::string& delim)
{
	JsonPtr node = getNode(path, delim);
	if (node)
		return node->wantUInt();
	
	throw FPNN_ERROR_MSG(FpnnJosnNodeNotExistError, "Target node doesn't exist.");
}

double Json::wantReal(const std::string& path, const std::string& delim)
{
	JsonPtr node = getNode(path, delim);
	if (node)
		return node->wantReal();
	
	throw FPNN_ERROR_MSG(FpnnJosnNodeNotExistError, "Target node doesn't exist.");
}

std::string Json::wantString(const std::string& path, const std::string& delim)
{
	JsonPtr node = getNode(path, delim);
	if (node)
		return node->wantString();
	
	throw FPNN_ERROR_MSG(FpnnJosnNodeNotExistError, "Target node doesn't exist.");
}

Json& Json::operator [] (const char * path)
{
	JsonPtr node = createNode(path, "./", true);
	return *(node.get());
}

Json& Json::operator [] (const std::string& path)
{
	JsonPtr node = createNode(path, "./", true);
	return *(node.get());
}

Json& Json::operator [] (int index)
{
	if (_type == JSON_Uninit)
		setArray();

	if (_type != JSON_Array)
		throw FPNN_ERROR_MSG(FpnnJsonNodeTypeMissMatchError, "Current node is not an array.");

	std::list<JsonPtr>* data = (std::list<JsonPtr>*)_data;
	auto it = data->begin();
	int i = 0;
	for (; it != data->end(); it++, i++)
	{
		if (i == index)
		{
			JsonPtr node = *it;
			return *(node.get());
		}
	}

	for (; i < index; i++)
	{
		JsonPtr node(new Json());
		pushNode(node);
	}

	JsonPtr node(new Json());
	pushNode(node);
	return *(node.get());
}

std::vector<bool> Json::wantBoolVector(const std::string& path, const std::string& delim)
{
	JsonPtr node = getNode(path, delim);
	if (node)
		return node->wantBoolVector();
	
	throw FPNN_ERROR_MSG(FpnnJosnNodeNotExistError, "Target node doesn't exist.");
}

std::vector<double> Json::wantRealVector(const std::string& path, const std::string& delim)
{
	JsonPtr node = getNode(path, delim);
	if (node)
		return node->wantRealVector();
	
	throw FPNN_ERROR_MSG(FpnnJosnNodeNotExistError, "Target node doesn't exist.");
}

std::vector<intmax_t> Json::wantIntVector(const std::string& path, const std::string& delim)
{
	JsonPtr node = getNode(path, delim);
	if (node)
		return node->wantIntVector();
	
	throw FPNN_ERROR_MSG(FpnnJosnNodeNotExistError, "Target node doesn't exist.");
}

std::vector<std::string> Json::wantStringVector(const std::string& path, const std::string& delim)
{
	JsonPtr node = getNode(path, delim);
	if (node)
		return node->wantStringVector();
	
	throw FPNN_ERROR_MSG(FpnnJosnNodeNotExistError, "Target node doesn't exist.");
}


std::map<std::string, bool> Json::wantBoolDict(const std::string& path, const std::string& delim)
{
	JsonPtr node = getNode(path, delim);
	if (node)
		return node->wantBoolDict();
	
	throw FPNN_ERROR_MSG(FpnnJosnNodeNotExistError, "Target node doesn't exist.");
}

std::map<std::string, double> Json::wantRealDict(const std::string& path, const std::string& delim)
{
	JsonPtr node = getNode(path, delim);
	if (node)
		return node->wantRealDict();
	
	throw FPNN_ERROR_MSG(FpnnJosnNodeNotExistError, "Target node doesn't exist.");
}

std::map<std::string, intmax_t> Json::wantIntDict(const std::string& path, const std::string& delim)
{
	JsonPtr node = getNode(path, delim);
	if (node)
		return node->wantIntDict();
	
	throw FPNN_ERROR_MSG(FpnnJosnNodeNotExistError, "Target node doesn't exist.");
}

std::map<std::string, std::string> Json::wantStringDict(const std::string& path, const std::string& delim)
{
	JsonPtr node = getNode(path, delim);
	if (node)
		return node->wantStringDict();
	
	throw FPNN_ERROR_MSG(FpnnJosnNodeNotExistError, "Target node doesn't exist.");
}
