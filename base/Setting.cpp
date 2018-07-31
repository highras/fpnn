#include <exception>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include "Setting.h"
#include "StringUtil.h"
#include "binary_abbr.h"
#include "md5.h"
#include "hex.h"
using namespace fpnn;
Setting::MapType Setting::_map;
std::string Setting::_config_file;

void Setting::printInfo(){
#ifdef _DEBUG_PRINT
	for(MapType::iterator it = _map.begin(); it != _map.end(); ++it)
		std::cout<<it->first<<"=>"<<it->second<<std::endl;
#endif
}

Setting::MapType Setting::loadMap(const std::string& file){
        Setting::MapType map;
	std::ifstream fin(file.c_str()); 
	if (fin.is_open()) {
		char line[1024];
		while( fin.getline(line,sizeof(line)) ){
			std::string sLine(line);
			StringUtil::trim( sLine );
			if(sLine.length() == 0 || sLine[0] == '#') continue;

			size_t pos = sLine.find_first_of("=");
			if(pos == std::string::npos) continue;
			std::string name = sLine.substr(0,pos);
			std::string value = sLine.substr(pos+1, sLine.length());

			StringUtil::trim(name);
			StringUtil::trim(value);
			if(name.length() == 0 || value.length() == 0) continue;
			map[name] = value;
		}
		fin.close();
		return map;
	}
	throw std::invalid_argument("Can not open file:" + file);
}

bool Setting::load(const std::string& file){
	_config_file = file;
    _map = loadMap(file);
    return true;
}

const std::string* Setting::_find(const std::string& key, const Setting::MapType& map)
{
	MapType::const_iterator iter = map.find(key);
	if (iter != map.end())
		return &iter->second;
	return NULL;
}

bool Setting::setted(const std::string& name,  const Setting::MapType& map){
	const std::string *v = _find(name, map);
	if(v) return true;
	return false;
}

std::string Setting::getString(const std::string& name, const std::string& dft, const Setting::MapType& map)
{
	const std::string *v = _find(name, map);
	if (v)
	{
		return *v;
	}
	return dft;
}

std::string Setting::getString(const std::vector<std::string>& priority, const std::string& dft, const Setting::MapType& map)
{
	for (auto it = priority.begin(); it != priority.end(); it++)
	{
		const std::string *v = _find(*it, map);
		if (v)
		{
			return *v;
		}
	}
	return dft;
}

intmax_t Setting::getInt(const std::string& name, intmax_t dft, const Setting::MapType& map)
{
	const std::string *v = _find(name, map);
	if (v)
	{
		char *end;
		const char *start = v->c_str();
		intmax_t i = strtoll(start, &end, 0);
		if (end > start)
		{
			if (end[0])
			{
				intmax_t x = binary_abbr(end, &end);
				if (end[0] == 0)
				{
					i *= x;
				}
				else
				{
					double r = strtod(start, &end);
					if (end > start && end[0] == 0)
					{
						i = r > INTMAX_MAX ? INTMAX_MAX
							 : r < INTMAX_MIN ? INTMAX_MIN
							 : (intmax_t)r;
					}
					else
						goto invalid;
				}
			}

			return i;
		}
	}
invalid:
	return dft;
}

intmax_t Setting::getInt(const std::vector<std::string>& priority, intmax_t dft, const Setting::MapType& map)
{
	for (auto it = priority.begin(); it != priority.end(); it++)
	{
		const std::string *v = _find(*it, map);
		if (v)
		{
			char *end;
			const char *start = v->c_str();
			intmax_t i = strtoll(start, &end, 0);
			if (end > start)
			{
				if (end[0])
				{
					intmax_t x = binary_abbr(end, &end);
					if (end[0] == 0)
					{
						i *= x;
					}
					else
					{
						double r = strtod(start, &end);
						if (end > start && end[0] == 0)
						{
							i = r > INTMAX_MAX ? INTMAX_MAX
								 : r < INTMAX_MIN ? INTMAX_MIN
								 : (intmax_t)r;
						}
						else
							goto invalid;
					}
				}

				return i;
			}
		}
	}
invalid:
	return dft;
}

bool Setting::getBool(const std::string& name, bool dft, const Setting::MapType& map)
{
	const std::string *v = _find(name, map);
	if (v)
	{
		const char *str = v->c_str();
		if (isdigit(str[0]) || str[0] == '-' || str[0] == '+')
			return atoi(str);

		if (strcasecmp(str, "true")==0 || strcasecmp(str, "yes")==0
			|| strcasecmp(str, "t")==0 || strcasecmp(str, "y")==0)
			return true;

		if (strcasecmp(str, "false")==0 || strcasecmp(str, "no")==0
			|| strcasecmp(str, "f")==0 || strcasecmp(str, "n")==0)
			return false;
	}
	return dft;
}

bool Setting::getBool(const std::vector<std::string>& priority, bool dft, const Setting::MapType& map)
{
	for (auto it = priority.begin(); it != priority.end(); it++)
	{
		const std::string *v = _find(*it, map);
		if (v)
		{
			const char *str = v->c_str();
			if (isdigit(str[0]) || str[0] == '-' || str[0] == '+')
				return atoi(str);

			if (strcasecmp(str, "true")==0 || strcasecmp(str, "yes")==0
				|| strcasecmp(str, "t")==0 || strcasecmp(str, "y")==0)
				return true;

			if (strcasecmp(str, "false")==0 || strcasecmp(str, "no")==0
				|| strcasecmp(str, "f")==0 || strcasecmp(str, "n")==0)
				return false;
		}
	}
	return dft;
}

double Setting::getReal(const std::string& name, double dft, const Setting::MapType& map)
{
	const std::string *v = _find(name, map);
	if (v)
	{
		char *end;
		const char *start = v->c_str();
		double r = strtod(start, &end);
		if (end > start && end[0] == 0)
			return r;
	}
	return dft;
}

double Setting::getReal(const std::vector<std::string>& priority, double dft, const Setting::MapType& map)
{
	for (auto it = priority.begin(); it != priority.end(); it++)
	{
		const std::string *v = _find(*it, map);
		if (v)
		{
			char *end;
			const char *start = v->c_str();
			double r = strtod(start, &end);
			if (end > start && end[0] == 0)
				return r;
		}
	}
	return dft;
}

std::vector<std::string> Setting::getStringList(const std::string& name, const Setting::MapType& map)
{
	std::vector<std::string> result;
	const std::string *v = _find(name, map);
	if (v)
	{
		StringUtil::split(*v, ", \t\r\n", result );
	}
	return result;
}

void Setting::set(const std::string& name, const std::string& value)
{
	_map[name] = value;
}

bool Setting::insert(const std::string& name, const std::string& value)
{
	MapType::iterator iter = _map.find(name);
	if (iter == _map.end())
	{
		_map[name] = value;
		return true;
	}

	return false;
}

bool Setting::update(const std::string& name, const std::string& value)
{
	MapType::iterator iter = _map.find(name);
	if (iter != _map.end())
	{
		iter->second = value;
		return true;
	}

	return false;
}

std::string Setting::getFileMD5(const std::string& file){
	std::ifstream in(file, std::ios::in);
	if (in.is_open()){
		std::istreambuf_iterator<char> beg(in), end;
		std::string configData(beg, end);
		in.close();

		unsigned char digest[16];
		md5_checksum(digest, configData.c_str(), configData.size());
		char hexstr[32 + 1];
		Hexlify(hexstr, digest, sizeof(digest));
		return std::string(hexstr);
	}
	return "";
}

/* name @tcp:localhost:11008^10 */
bool Setting::parseEndpoint(const std::string& endpoint, EndpointST& st) {
	std::vector<std::string> result;
	std::string value = endpoint;
	StringUtil::trim(value);
	StringUtil::split(value,"@", result);
	if(result.size() != 2) return false;
	st.name = result[0]; 
	value = result[1];

	result.clear();
	StringUtil::split(value,":", result);
	if(result.size() != 3) return false;
	st.proto = result[0];
	st.host = result[1];
	if(st.host.length() == 0) st.host = "0.0.0.0";
	value = result[2];
	if(value.length() == 0) return false;

	result.clear();
	StringUtil::split(value, "^", result );
	if(result.size() != 2) return false;
	std::string str = result[0];
	st.port = atoi(str.c_str());
	str = result[1];
	st.timeout = atoi(str.c_str());

#ifdef _DEBUG_PRINT
	std::cout<<"name:"<<st.name<<" proto:"<<st.proto<<" host:"<<st.host<<" port:"<<st.port<<" timeout:"<<st.timeout<<std::endl;
#endif

	return true;
}

/* mysql://dev:dev@127.0.0.1:3306/db^10 */
bool Setting::parseDBSetting(const std::string& mysqlConn, MysqlConnST& st){
	std::vector<std::string> result;
	std::string value = mysqlConn;
	StringUtil::trim(value);
	StringUtil::split(value,"/", result);
	if(result.size() != 4) return false;
	st.proto = result[0]; 
	st.proto.resize(st.proto.length() - 1); //remove the last :

	st.user = result[2];
	st.db = result[3];

	result.clear();
	StringUtil::split(st.user,"@",result);
	if(result.size() != 2) return false;
	st.user = result[0]; 
	st.host = result[1];

	result.clear();
	StringUtil::split(st.user,":", result);
	if(result.size() != 2) return false;
	st.user = result[0]; 
	st.passwd = result[1]; 

	result.clear();
	StringUtil::split(st.host,":",result);
	if(result.size() != 2) return false;
	st.host = result[0]; 
	std::string str = result[1]; 
	st.port = atoi(str.c_str());

	result.clear();
	StringUtil::split(st.db,"^",result);
	if(result.size() != 2) return false;
	st.db = result[0]; 
	str = result[1]; 
	st.timeout = atoi(str.c_str());
#ifdef _DEBUG_PRINT
	std::cout<<"proto:"<<st.proto<<" user:"<<st.user<<" passwd:"<<st.passwd<<" host:"<<st.host<<" port:"<<st.port<<" DB:"<<st.db<<" timeout:"<<st.timeout<<std::endl;
#endif
	return true;
}
