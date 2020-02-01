#include <iostream>
#include "FPJson.h"

using namespace std;
using namespace fpnn;

vector<bool> boolvec{true, false, true};
vector<int> intvec{23, 232, 33, 34};
vector<double> doublevec{23.3, 23e2, 3e3, .34};
vector<string> stringvec{"wewe", "dsd", "efs", "dd"};
map<string, bool> boolmap{{"bool1", false}, {"bool2", true}, {"bool3", false}};
map<string, int> intmap{{"int1", 232}, {"int2", 121}, {"int3", 2323}};
map<string, double> doublemap{{"double1", 23.2}, {"double2", 12e1}, {"double3", 2.32e3}};
map<string, string> stringmap{{"string1", "dwed"}, {"string2", "ddsd"}, {"string3", "dwee"}};
auto tupleDemo = std::make_tuple(10,"a", "sdsd", 232, .34);
array<int, 4> arrayDemo{12, 34, 56, 78};
deque<int> dequeDemo{23, 23,43};
list<int> listDemo{2563, 2783,483};
set<int> setDemo{223, 2322, 323, 324};
unordered_set<int> usetDemo{2123, 23212, 3213, 3114};
unordered_map<string, int> umapDemo{{"uint1", 232}, {"uint2", 121}, {"uint3", 2323}};

void advancedSetTest()
{
	cout<<"----------------- advancedSetTest ---------------------"<<endl;
	{
		Json json;
		json.setArray(12, 32., 34e5, "sas", intvec, "sas", listDemo, stringmap, tupleDemo, arrayDemo, dequeDemo, setDemo, usetDemo, umapDemo);
		cout<<json<<endl;
	}
	{
		Json json;
		json.setArray(tupleDemo);
		cout<<json<<endl;
	}
	{
		Json json;
		json.setArray(arrayDemo);
		cout<<json<<endl;
	}

	{
		Json json;
		json.setArray(dequeDemo);
		cout<<json<<endl;
	}
	{
		Json json;
		json.setArray(boolvec);
		cout<<json<<endl;
		json.setArray(intvec);
		cout<<json<<endl;
		json.setArray(doublevec);
		cout<<json<<endl;
		json.setArray(stringvec);
		cout<<json<<endl;
	}
	{
		Json json;
		json.setArray(listDemo);
		cout<<json<<endl;
	}
	{
		Json json;
		json.setArray(setDemo);
		cout<<json<<endl;
	}

	{
		Json json;
		json.setArray(usetDemo);
		cout<<json<<endl;
	}
	{
		Json json;
		json.setDict(boolmap);
		cout<<json<<endl;
		json.setDict(intmap);
		cout<<json<<endl;
		json.setDict(doublemap);
		cout<<json<<endl;
		json.setDict(stringmap);
		cout<<json<<endl;
	}
	{
		Json json;
		json.setDict(umapDemo);
		cout<<json<<endl;
	}
}

void advancedAssignTest()
{
	cout<<"----------------- advancedAssignTest ---------------------"<<endl;
	{
		Json json;
		json = tupleDemo;
		cout<<json<<endl;
	}
	{
		Json json;
		json = arrayDemo;
		cout<<json<<endl;
	}

	{
		Json json;
		json = dequeDemo;
		cout<<json<<endl;
	}
	{
		Json json;
		json = boolvec;
		cout<<json<<endl;
		json = intvec;
		cout<<json<<endl;
		json = doublevec;
		cout<<json<<endl;
		json = stringvec;
		cout<<json<<endl;
	}
	{
		Json json;
		json = listDemo;
		cout<<json<<endl;
	}
	{
		Json json;
		json = setDemo;
		cout<<json<<endl;
	}

	{
		Json json;
		json = usetDemo;
		cout<<json<<endl;
	}
	{
		Json json;
		json = boolmap;
		cout<<json<<endl;
		json = intmap;
		cout<<json<<endl;
		json = doublemap;
		cout<<json<<endl;
		json = stringmap;
		cout<<json<<endl;
	}
	{
		Json json;
		json = umapDemo;
		cout<<json<<endl;
	}
}


void advancedPushTest()
{
	cout<<"----------------- advancedPushTest ---------------------"<<endl;
	{
		Json json;
		json.push(tupleDemo);
		cout<<json<<endl;
	}
	{
		Json json;
		json.push(arrayDemo);
		cout<<json<<endl;
	}

	{
		Json json;
		json.push(dequeDemo);
		cout<<json<<endl;
	}
	{
		Json json;
		json.push(boolvec);
		cout<<json<<endl;
		json.push(intvec);
		cout<<json<<endl;
		json.push(doublevec);
		cout<<json<<endl;
		json.push(stringvec);
		cout<<json<<endl;
	}
	{
		Json json;
		json.push(listDemo);
		cout<<json<<endl;
	}
	{
		Json json;
		json.push(setDemo);
		cout<<json<<endl;
	}

	{
		Json json;
		json.push(usetDemo);
		cout<<json<<endl;
	}
	{
		Json json;
		json.push(boolmap);
		cout<<json<<endl;
		json.push(intmap);
		cout<<json<<endl;
		json.push(doublemap);
		cout<<json<<endl;
		json.push(stringmap);
		cout<<json<<endl;
	}
	{
		Json json;
		json.push(umapDemo);
		cout<<json<<endl;
	}
}

void advancedMergeTest()
{
	cout<<"----------------- advancedMergeTest ---------------------"<<endl;
	{
		Json json;
		json.fill(23002, 32003);
		json.merge(tupleDemo);
		cout<<json<<endl;
	}
	{
		Json json;
		json.fill(23002, 32003);
		json.merge(arrayDemo);
		cout<<json<<endl;
	}

	{
		Json json;
		json.fill(23002, 32003);
		json.merge(dequeDemo);
		cout<<json<<endl;
	}
	{
		Json json;
		json.fill(23002, 32003);
		json.merge(boolvec);
		cout<<json<<endl;
		json.merge(intvec);
		cout<<json<<endl;
		json.merge(doublevec);
		cout<<json<<endl;
		json.merge(stringvec);
		cout<<json<<endl;
	}
	{
		Json json;
		json.fill(23002, 32003);
		json.merge(listDemo);
		cout<<json<<endl;
	}
	{
		Json json;
		json.fill(23002, 32003);
		json.merge(setDemo);
		cout<<json<<endl;
	}

	{
		Json json;
		json.fill(23002, 32003);
		json.merge(usetDemo);
		cout<<json<<endl;
	}
	{
		Json json;
		json.add("aaa", 23002);
		json.add("bbb", 32003);
		json.merge(boolmap);
		cout<<json<<endl;
		json.merge(intmap);
		cout<<json<<endl;
		json.merge(doublemap);
		cout<<json<<endl;
		json.merge(stringmap);
		cout<<json<<endl;
	}
	{
		Json json;
		json.add("aaa", 23002);
		json.add("bbb", 32003);
		json.merge(umapDemo);
		cout<<json<<endl;
	}
}

void advancedPathPushTest()
{
	cout<<"----------------- advancedPathPushTest ---------------------"<<endl;
	string path = "aaa.bbb";
	{
		Json json;
		json.push(path, tupleDemo);
		cout<<json<<endl;
	}
	{
		Json json;
		json.push(path, arrayDemo);
		cout<<json<<endl;
	}

	{
		Json json;
		json.push(path, dequeDemo);
		cout<<json<<endl;
	}
	{
		Json json;
		json.push(path, boolvec);
		cout<<json<<endl;
		json.push(path, intvec);
		cout<<json<<endl;
		json.push(path, doublevec);
		cout<<json<<endl;
		json.push(path, stringvec);
		cout<<json<<endl;
	}
	{
		Json json;
		json.push(path, listDemo);
		cout<<json<<endl;
	}
	{
		Json json;
		json.push(path, setDemo);
		cout<<json<<endl;
	}

	{
		Json json;
		json.push(path, usetDemo);
		cout<<json<<endl;
	}
	{
		Json json;
		json.push(path, boolmap);
		cout<<json<<endl;
		json.push(path, intmap);
		cout<<json<<endl;
		json.push(path, doublemap);
		cout<<json<<endl;
		json.push(path, stringmap);
		cout<<json<<endl;
	}
	{
		Json json;
		json.push(path, umapDemo);
		cout<<json<<endl;
	}
}

void advancedPathAddTest()
{
	cout<<"----------------- advancedPathAddTest ---------------------"<<endl;
	string path = "aaa.bbb";
	{
		Json json;
		json.add(path, tupleDemo);
		cout<<json<<endl;
	}
	{
		Json json;
		json.add(path, arrayDemo);
		cout<<json<<endl;
	}

	{
		Json json;
		json.add(path, dequeDemo);
		cout<<json<<endl;
	}
	{
		Json json;
		json.add(path + ".bool", boolvec);
		cout<<json<<endl;
		json.add(path + ".int", intvec);
		cout<<json<<endl;
		json.add(path + ".double", doublevec);
		cout<<json<<endl;
		json.add(path + ".string", stringvec);
		cout<<json<<endl;
	}
	{
		Json json;
		json.add(path, listDemo);
		cout<<json<<endl;
	}
	{
		Json json;
		json.add(path, setDemo);
		cout<<json<<endl;
	}

	{
		Json json;
		json.add(path, usetDemo);
		cout<<json<<endl;
	}
	{
		Json json;
		json.add(path + ".bool", boolmap);
		cout<<json<<endl;
		json.add(path + ".int", intmap);
		cout<<json<<endl;
		json.add(path + ".double", doublemap);
		cout<<json<<endl;
		json.add(path + ".string", stringmap);
		cout<<json<<endl;
	}
	{
		Json json;
		json.add(path, umapDemo);
		cout<<json<<endl;
	}
}

void advancedPathMergeTest()
{
	cout<<"----------------- advancedPathMergeTest ---------------------"<<endl;
	string path = "aaa.bbb";
	{
		Json json;
		json.fillTo(path, 23002, 32003);
		json.merge(path, tupleDemo);
		cout<<json<<endl;
	}
	{
		Json json;
		json.fillTo(path, 23002, 32003);
		json.merge(path, arrayDemo);
		cout<<json<<endl;
	}

	{
		Json json;
		json.fillTo(path, 23002, 32003);
		json.merge(path, dequeDemo);
		cout<<json<<endl;
	}
	{
		Json json;
		json.fillTo(path, 23002, 32003);
		json.merge(path, boolvec);
		cout<<json<<endl;
		json.merge(path, intvec);
		cout<<json<<endl;
		json.merge(path, doublevec);
		cout<<json<<endl;
		json.merge(path, stringvec);
		cout<<json<<endl;
	}
	{
		Json json;
		json.fillTo(path, 23002, 32003);
		json.merge(path, listDemo);
		cout<<json<<endl;
	}
	{
		Json json;
		json.fillTo(path, 23002, 32003);
		json.merge(path, setDemo);
		cout<<json<<endl;
	}

	{
		Json json;
		json.fillTo(path, 23002, 32003);
		json.merge(path, usetDemo);
		cout<<json<<endl;
	}
	{
		Json json;
		json.add(path + ".aaa", 23002);
		json.add(path + ".bbb", 32003);
		json.merge(path, boolmap);
		cout<<json<<endl;
		json.merge(path, intmap);
		cout<<json<<endl;
		json.merge(path, doublemap);
		cout<<json<<endl;
		json.merge(path, stringmap);
		cout<<json<<endl;
	}
	{
		Json json;
		json.add(path + ".aaa", 23002);
		json.add(path + ".bbb", 32003);
		json.merge(path, umapDemo);
		cout<<json<<endl;
	}
}

void specialMergeAndPushTest()
{
	cout<<"----------------- specialMergeAndPushTest ---------------------"<<endl;
	{
		Json json;
		json.fill(12, 32., 34e5, "sas", intvec, "sas", listDemo, stringmap, tupleDemo, arrayDemo, dequeDemo, setDemo, usetDemo, umapDemo);
		cout<<json<<endl;
	}
	{
		Json json;
		json.fillTo("aaa.bbb.", 12, 32., 34e5, "sas", intvec, "sas", listDemo, stringmap, tupleDemo, arrayDemo, dequeDemo, setDemo, usetDemo, umapDemo);
		cout<<json<<endl;
	}
}

int main()
{
	advancedSetTest();
	advancedAssignTest();
	advancedPushTest();
	advancedMergeTest();
	advancedPathPushTest();
	advancedPathAddTest();
	advancedPathMergeTest();
	specialMergeAndPushTest();

	return 0;
}
