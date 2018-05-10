#include "StringUtil.h"
#include <string.h>
#include <sstream>
using namespace fpnn;

char * StringUtil::rtrim(char *s){
	int len;
	char *p;

	if (!s)
		return s;

	len = strlen(s);
	p = &s[len];

	for (--p; p >= s; --p){
		if (!isspace(*(unsigned char *)p))
			break;
	}
	*++p = 0;
	return s;
}

char * StringUtil::ltrim(char *s){
	if (!s)
		return (char *)s;

	int c;
	for (; (c = *(unsigned char *)s++) != 0; ){
		if (!isspace(c))
			break;
	}   
	return (char *)--s;
}

char * StringUtil::trim(char *s){
	s = ltrim(s);
	return rtrim(s);
}

void StringUtil::softTrim(const char* path, char* &start, char* &end)
{
	start = (char *)path;
	if (!start)
	{
		end = NULL;
		return;
	}

	while (start && *start != 0 && isspace(*start++));

	end = start;
	if (*start == 0)
		return;
	
	start -= 1;
	
	char *forward = end;
	while (*forward != 0)
		if (!isspace(*forward++))
			end = forward;
}

std::string& StringUtil::rtrim(std::string& s){
	if(s.empty()) return s;
	size_t size = s.find_last_not_of("\n\r\t ");
	if(size == std::string::npos) size = 0;
	else size += 1;
	return s.erase(size);
}

std::string& StringUtil::ltrim(std::string& s){
	if(s.empty()) return s;
	return s.erase(0, s.find_first_not_of("\n\r\t "));
}

std::string& StringUtil::trim(std::string& s){
	s = ltrim(s);
	return rtrim(s);
}

bool StringUtil::replace(std::string& str, const std::string& from, const std::string& to) {
    size_t start_pos = str.find(from);
    if(start_pos == std::string::npos)
        return false;
    str.replace(start_pos, from.length(), to);
    return true;
}

std::vector<std::string>& StringUtil::split(const std::string& s, const std::string& delim, std::vector<std::string> &elems) {
	if(s.empty()) return elems;
	
	std::string::size_type pos_begin = s.find_first_not_of(delim);
	std::string::size_type comma_pos = 0;
	std::string tmp;
	while (pos_begin != std::string::npos){
		comma_pos = s.find_first_of(delim, pos_begin);
		if (comma_pos != std::string::npos){
			tmp = s.substr(pos_begin, comma_pos - pos_begin);
			pos_begin = comma_pos + 1;
		}
		else{
			tmp = s.substr(pos_begin);
			pos_begin = comma_pos;
		}
		if (!tmp.empty()){
			elems.push_back(tmp);
			tmp.clear();
		}
	}

	return elems;
}

std::set<std::string>& StringUtil::split(const std::string &s, const std::string& delim, std::set<std::string> &elems){
	std::vector<std::string> e;
	e = split(s, delim, e);
	for(size_t i = 0; i < e.size(); ++i){
		elems.insert(e[i]);
	}
	return elems;
}

std::unordered_set<std::string>& StringUtil::split(const std::string &s, const std::string& delim, std::unordered_set<std::string> &elems){
	std::vector<std::string> e;
	e = split(s, delim, e);
	for(size_t i = 0; i < e.size(); ++i){
		elems.insert(e[i]);
	}
	return elems;
}

std::string StringUtil::join(const std::set<std::string> &v, const std::string& delim)
{
	std::string ret;
	for (auto& s: v)
	{
		if(ret.length())
			ret += delim;
		
		ret += s;
	}
	return ret;
}

std::string StringUtil::join(const std::vector<std::string> &v, const std::string& delim)
{
	std::string ret;
	for (size_t i = 0; i < v.size(); ++i)
	{
		if(i != 0)
			ret += delim;
		
		ret += v[i];
	}
	return ret;
}

std::string StringUtil::join(const std::map<std::string, std::string> &v, const std::string& delim)
{
	std::string ret;
	for (auto& s: v)
	{
		if(ret.length())
			ret += delim;
		
		ret += s.first + ":" + s.second;
	}
	return ret;
}

/*std::string StringUtil::join(const std::vector<uint32_t> &v, const std::string& delim)
{
	std::string ret;
	for (size_t i = 0; i < v.size(); ++i)
	{
		if(i != 0)
			ret += delim;
		
		ret += std::to_string(v[i]);
	}
	return ret;
}*/

#ifdef TEST_STRING_UTIL
//g++ -g -DTEST_STRING_UTIL StringUtil.cpp -std=c++11
#include <sstream>
#include <iostream>
using namespace std;
using namespace StringUtil;

template<typename UNSIGNED_INTEGER_TYPE>
void testCharMarkMap()
{
	const UNSIGNED_INTEGER_TYPE digit = 0x1;
	const UNSIGNED_INTEGER_TYPE alphaTable = 0x2;
	const UNSIGNED_INTEGER_TYPE whiteChar = 0x4;
	const UNSIGNED_INTEGER_TYPE printable = 0x8;

	CharMarkMap<UNSIGNED_INTEGER_TYPE> cmm;

	cmm.init("1234567890", digit | printable);
	cmm.init("abcdefghigklmnopqrstuvwxyz", alphaTable | printable);
	cmm.init(" \t", whiteChar | printable);
	cmm.init("\r\n\b", whiteChar);

	cout<<"check 6 is digit reault "<<cmm.check('6', digit)<<endl;
	cout<<"check 6 is printable reault "<<cmm.check('6', printable)<<endl;
	cout<<"check 6 is printable & digit reault "<<cmm.check('6', digit | printable)<<endl;
	cout<<"check 6 is alphaTable reault "<<cmm.check('6', alphaTable)<<endl;
	cout<<"check a is whiteChar "<<cmm.check('a', whiteChar)<<endl;
}

int main(int argc, char **argv)
{
	char s[] = "\t \naaabb \n";	
	string ss = "\t \naaabb \n";	
	char se[] = "\t \n \n\r";	
	string sse = "\t \n \n\r";	
	char* s2 = trim(s);
	ss = trim(ss);
	char* se2 = trim(se);
	sse = trim(sse);
	cout<<"s=>"<<s2<<"  len:"<<strlen(s2)<<endl;
	cout<<"se=>"<<se2<<"  len:"<<strlen(se2)<<endl;
	cout<<"ss=>"<<ss<<"  len:"<<ss.size()<<endl;
	cout<<"sse=>"<<sse<<"  len:"<<sse.size()<<endl;

	string sp = "\n abc  \r de ff \t ";
	vector<string> elems;
	elems = split(sp, "\r\n\t ", elems);
	cout<<"split:"<<endl;
	for(size_t i = 0; i < elems.size(); ++i){
		cout<<elems[i]<<endl;
	}
	string sp2 = "/usr////local/bin/";
	vector<string> elems2;
	elems2 = split(sp2, " /", elems2);
	cout<<"split2:"<<endl;
	for(size_t i = 0; i < elems2.size(); ++i){
		cout<<elems2[i]<<endl;
	}
	cout<<"end"<<endl;

	string dp = "1,2,3,4,5,";
	set<int32_t> de;
	de = split(dp, ",", de);
	cout<<"de:"<<endl;
	for(auto it = de.begin(); it != de.end(); ++it){
		cout<<*it<<endl;
	}
	cout<<"end"<<endl;

	cout<<"testCharMarkMap<uint8_t>():"<<endl;
	testCharMarkMap<uint8_t>();
	cout<<"testCharMarkMap<uint32_t>():"<<endl;
	testCharMarkMap<uint32_t>();

	return 0;
}

#endif
