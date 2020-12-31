#include <iostream>
#include "FPJson.h"

using namespace std;
using namespace fpnn;

const std::string tupleDemo = R"(
{"aa":["aa", 123, 32.4, 100, "asas"], "bb":{}}
)";

const std::string arrayDemo = R"(
{"strArray":["aa", "bb", "cc"], "intArray":[1, 2, 3, 4, 5]}
)";

const std::string dictDemo = R"(
{"strDict":{"123":"aa", "456":"bb", "789":"cc"}, "intDict":{"aa":1, "bb":2, "cc":3, "dd":4, "ee":5}}
)";

void test_tuple(JsonPtr json, bool compatibleMode)
{
	cout<<"[Test tuple] Pattern unmatched testing. Compatible mode: "<<(compatibleMode ? "True" : "False")<<endl;

	std::tuple<std::string, int, double, uint64_t, std::string, int, std::string> more;
	std::tuple<std::string, uint32_t, float, int> less;
	std::tuple<std::string, uint32_t, std::string, int, int> error;

	cout<<"---- more elements ----------"<<endl;
	try {
		(*json)["aa"].wantTuple(more, compatibleMode);

		cout<<"Element 1: "<<std::get<0>(more)<<endl;
		cout<<"Element 2: "<<std::get<1>(more)<<endl;
		cout<<"Element 3: "<<std::get<2>(more)<<endl;
		cout<<"Element 4: "<<std::get<3>(more)<<endl;
		cout<<"Element 5: "<<std::get<4>(more)<<endl;
		cout<<"Element 6: "<<std::get<5>(more)<<endl;
		cout<<"Element 7: "<<std::get<6>(more)<<endl;

	}
	catch (const FpnnJsonNodeTypeMissMatchError &e)
	{
		cout<<"[Expectant error]: Type miss match."<<endl;
	}

	cout<<"---- less elements ----------"<<endl;
	try {
		(*json)["aa"].wantTuple(less, compatibleMode);

		cout<<"Element 1: "<<std::get<0>(less)<<endl;
		cout<<"Element 2: "<<std::get<1>(less)<<endl;
		cout<<"Element 3: "<<std::get<2>(less)<<endl;
		cout<<"Element 4: "<<std::get<3>(less)<<endl;

	}
	catch (const FpnnJsonNodeTypeMissMatchError &e)
	{
		cout<<"[Expectant error]: Type miss match."<<endl;
	}

	cout<<"---- error elements ----------"<<endl;
	try {
		(*json)["aa"].wantTuple(error, compatibleMode);

		cout<<"Element 1: "<<std::get<0>(error)<<endl;
		cout<<"Element 2: "<<std::get<1>(error)<<endl;
		cout<<"Element 3: "<<std::get<2>(error)<<endl;
		cout<<"Element 4: "<<std::get<3>(error)<<endl;
		cout<<"Element 5: "<<std::get<4>(error)<<endl;

	}
	catch (const FpnnJsonNodeTypeMissMatchError &e)
	{
		cout<<"[Expectant error]: Type miss match."<<endl;
	}
}

void test_tuple()
{
	cout<<endl<<"[Test tuple] Pattern matched testing ..."<<endl;

	std::tuple<std::string, int, double, uint64_t, std::string> matchTuple;
	try {
		JsonPtr json = Json::parse(tupleDemo.c_str());
		cout<<"get json:"<<endl;
		cout<<json<<endl<<endl;

		(*json)["aa"].wantTuple(matchTuple);
		cout<<"Element 1: "<<std::get<0>(matchTuple)<<endl;
		cout<<"Element 2: "<<std::get<1>(matchTuple)<<endl;
		cout<<"Element 3: "<<std::get<2>(matchTuple)<<endl;
		cout<<"Element 4: "<<std::get<3>(matchTuple)<<endl;
		cout<<"Element 5: "<<std::get<4>(matchTuple)<<endl;

		test_tuple(json, false);
		test_tuple(json, true);
			
	}
	catch (const FpnnError &e)
	{
		cout<<"[Error]: "<<e.message()<<" line: "<<e.line()<<", fun: "<<e.fun()<<endl;
	}
}

template <typename K, size_t N>
void test_array(JsonPtr json, const char* key, const char* desc, bool compatibleMode)
{
	cout<<endl<<desc<<endl;

	std::array<K, N> elemArray = (*json)[key].wantArray<K, N>(compatibleMode);
	for (auto& element: elemArray)
		cout<<"Element: "<<element<<endl;
}

void test_array()
{
	cout<<endl<<"[Test array] ..."<<endl;

	try {

		JsonPtr json = Json::parse(arrayDemo.c_str());
		cout<<"get json:"<<endl;
		cout<<json<<endl<<endl;

		cout<<endl<<"[Test array] Length matched testing ..."<<endl;

		test_array<std::string, 3>(json, "strArray", "--- string array: length matched version.", false);
		test_array<int, 5>(json, "intArray", "--- int array: length matched version.", false);

		cout<<endl<<"[Test array] Length miss matched ** without ** compatible testing ..."<<endl;

		try {
			test_array<std::string, 5>(json, "strArray", "--- string array: more length version.", false);
		}
		catch (const FpnnJsonNodeTypeMissMatchError &e)
		{
			cout<<"[Expectant error]: Type miss match."<<endl;
		}
		try {
			test_array<std::string, 2>(json, "strArray", "--- string array: less length version.", false);
		}
		catch (const FpnnJsonNodeTypeMissMatchError &e)
		{
			cout<<"[Expectant error]: Type miss match."<<endl;
		}

		try {
			test_array<int, 7>(json, "intArray", "--- int array: more length version.", false);
		}
		catch (const FpnnJsonNodeTypeMissMatchError &e)
		{
			cout<<"[Expectant error]: Type miss match."<<endl;
		}

		try {
			test_array<int, 3>(json, "intArray", "--- int array: less length version.", false);
		}
		catch (const FpnnJsonNodeTypeMissMatchError &e)
		{
			cout<<"[Expectant error]: Type miss match."<<endl;
		}

		cout<<endl<<"[Test array] Length miss matched ** with ** compatible testing ..."<<endl;

		test_array<std::string, 5>(json, "strArray", "--- string array: more length version.", true);
		test_array<std::string, 2>(json, "strArray", "--- string array: less length version.", true);

		test_array<int, 7>(json, "intArray", "--- int array: more length version.", true);
		test_array<int, 3>(json, "intArray", "--- int array: less length version.", true);

	}
	catch (const FpnnError &e)
	{
		cout<<"[Error]: "<<e.message()<<" line: "<<e.line()<<", fun: "<<e.fun()<<endl;
	}
}

template <typename K>
void test_queue(JsonPtr json, const char* key, const char* desc)
{
	cout<<endl<<desc<<endl;

	std::queue<K> elemQueue = (*json)[key].wantQueue<K>();
	while (elemQueue.size())
	{
		cout<<"Element: "<<elemQueue.front()<<endl;
		elemQueue.pop();
	}
}

void test_queue()
{
	cout<<endl<<"[Test queue] ..."<<endl;

	try {

		JsonPtr json = Json::parse(arrayDemo.c_str());
		cout<<"get json:"<<endl;
		cout<<json<<endl<<endl;

		test_queue<std::string>(json, "strArray", "--- string queue:");
		test_queue<int>(json, "intArray", "--- int queue:");
	}
	catch (const FpnnError &e)
	{
		cout<<"[Error]: "<<e.message()<<" line: "<<e.line()<<", fun: "<<e.fun()<<endl;
	}
}

template <typename K>
void test_deque(JsonPtr json, const char* key, const char* desc)
{
	cout<<endl<<desc<<endl;

	std::deque<K> elemQueue = (*json)[key].wantDeque<K>();
	while (elemQueue.size())
	{
		cout<<"Element: "<<elemQueue.front()<<endl;
		elemQueue.pop_front();
	}
}

void test_deque()
{
	cout<<endl<<"[Test deque] ..."<<endl;

	try {

		JsonPtr json = Json::parse(arrayDemo.c_str());
		cout<<"get json:"<<endl;
		cout<<json<<endl<<endl;

		test_deque<std::string>(json, "strArray", "--- string deque:");
		test_deque<int>(json, "intArray", "--- int deque:");
	}
	catch (const FpnnError &e)
	{
		cout<<"[Error]: "<<e.message()<<" line: "<<e.line()<<", fun: "<<e.fun()<<endl;
	}
}

template <typename K>
void test_list(JsonPtr json, const char* key, const char* desc)
{
	cout<<endl<<desc<<endl;

	std::list<K> elemList = (*json)[key].wantList<K>();
	for (auto& element: elemList)
		cout<<"Element: "<<element<<endl;
}

void test_list()
{
	cout<<endl<<"[Test list] ..."<<endl;

	try {

		JsonPtr json = Json::parse(arrayDemo.c_str());
		cout<<"get json:"<<endl;
		cout<<json<<endl<<endl;

		test_list<std::string>(json, "strArray", "--- string list:");
		test_list<int>(json, "intArray", "--- int list:");
	}
	catch (const FpnnError &e)
	{
		cout<<"[Error]: "<<e.message()<<" line: "<<e.line()<<", fun: "<<e.fun()<<endl;
	}
}

template <typename K>
void test_vector(JsonPtr json, const char* key, const char* desc)
{
	cout<<endl<<desc<<endl;

	std::vector<K> elemVector = (*json)[key].wantVector<K>();
	for (auto& element: elemVector)
		cout<<"Element: "<<element<<endl;
}

void test_vector()
{
	cout<<endl<<"[Test vector] ..."<<endl;

	try {

		JsonPtr json = Json::parse(arrayDemo.c_str());
		cout<<"get json:"<<endl;
		cout<<json<<endl<<endl;

		test_vector<std::string>(json, "strArray", "--- string vector:");
		test_vector<int>(json, "intArray", "--- int vector:");
	}
	catch (const FpnnError &e)
	{
		cout<<"[Error]: "<<e.message()<<" line: "<<e.line()<<", fun: "<<e.fun()<<endl;
	}
}

template <typename K>
void test_set(JsonPtr json, const char* key, const char* desc)
{
	cout<<endl<<desc<<endl;

	std::set<K> elemSet = (*json)[key].wantSet<K>();
	for (auto& element: elemSet)
		cout<<"Element: "<<element<<endl;
}

void test_set()
{
	cout<<endl<<"[Test set] ..."<<endl;

	try {

		JsonPtr json = Json::parse(arrayDemo.c_str());
		cout<<"get json:"<<endl;
		cout<<json<<endl<<endl;

		test_set<std::string>(json, "strArray", "--- string set:");
		test_set<int>(json, "intArray", "--- int set:");
	}
	catch (const FpnnError &e)
	{
		cout<<"[Error]: "<<e.message()<<" line: "<<e.line()<<", fun: "<<e.fun()<<endl;
	}
}

template <typename K>
void test_unordered_set(JsonPtr json, const char* key, const char* desc)
{
	cout<<endl<<desc<<endl;

	std::unordered_set<K> elemSet = (*json)[key].wantUnorderedSet<K>();
	for (auto& element: elemSet)
		cout<<"Element: "<<element<<endl;
}

void test_unordered_set()
{
	cout<<endl<<"[Test unordered set] ..."<<endl;

	try {

		JsonPtr json = Json::parse(arrayDemo.c_str());
		cout<<"get json:"<<endl;
		cout<<json<<endl<<endl;

		test_unordered_set<std::string>(json, "strArray", "--- string unordered set:");
		test_unordered_set<int>(json, "intArray", "--- int unordered set:");
	}
	catch (const FpnnError &e)
	{
		cout<<"[Error]: "<<e.message()<<" line: "<<e.line()<<", fun: "<<e.fun()<<endl;
	}
}

template <typename K>
void test_dict(JsonPtr json, const char* key, const char* desc)
{
	cout<<endl<<desc<<endl;

	std::map<std::string, K> elemMap = (*json)[key].wantDict<K>();
	for (auto& pp: elemMap)
		cout<<"Element: "<<pp.first<<":"<<pp.second<<endl;
}

void test_dict()
{
	cout<<endl<<"[Test map] ..."<<endl;

	try {

		JsonPtr json = Json::parse(dictDemo.c_str());
		cout<<"get json:"<<endl;
		cout<<json<<endl<<endl;

		test_dict<std::string>(json, "strDict", "--- string map:");
		test_dict<int>(json, "intDict", "--- int map:");
	}
	catch (const FpnnError &e)
	{
		cout<<"[Error]: "<<e.message()<<" line: "<<e.line()<<", fun: "<<e.fun()<<endl;
	}
}

template <typename K>
void test_unordered_dict(JsonPtr json, const char* key, const char* desc)
{
	cout<<endl<<desc<<endl;

	std::unordered_map<std::string, K> elemMap = (*json)[key].wantUnorderedDict<K>();
	for (auto& pp: elemMap)
		cout<<"Element: "<<pp.first<<":"<<pp.second<<endl;
}

void test_unordered_dict()
{
	cout<<endl<<"[Test unordered map] ..."<<endl;

	try {

		JsonPtr json = Json::parse(dictDemo.c_str());
		cout<<"get json:"<<endl;
		cout<<json<<endl<<endl;

		test_unordered_dict<std::string>(json, "strDict", "--- string unordered map:");
		test_unordered_dict<int>(json, "intDict", "--- int unordered map:");
	}
	catch (const FpnnError &e)
	{
		cout<<"[Error]: "<<e.message()<<" line: "<<e.line()<<", fun: "<<e.fun()<<endl;
	}
}

int main()
{
	test_tuple();
	test_array();
	test_queue();
	test_deque();
	test_list();
	test_vector();
	test_set();
	test_unordered_set();
	test_dict();
	test_unordered_dict();

	return 0;
}