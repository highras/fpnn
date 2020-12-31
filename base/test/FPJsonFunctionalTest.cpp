#include <iostream>
#include <math.h>
#include "FPJson.h"

using namespace std;
using namespace fpnn;

void assignSignleElement()
{
	cout<<"-----------[ Build Signle Element ]---------------\n";
	{
		Json n, b, i, r, s, cs, a, d;
		n.setNull();        cout<<"Json must be null: ["<<n<<"]\n";
		b.setBool(true);    cout<<"Json must be true: ["<<b<<"]\n";
		b.setBool(false);   cout<<"Json must be false: ["<<b<<"]\n";
		b.setBool(200);   cout<<"Json must be true: ["<<b<<"]\n";
		b.setBool(-1);   cout<<"Json must be true: ["<<b<<"]\n";
		b.setBool(0);   cout<<"Json must be false: ["<<b<<"]\n";
		b.setBool(NULL);   cout<<"Json must be false: ["<<b<<"]\n";

		i.setInt(232);   cout<<"Json must be 232: ["<<i<<"]\n";
		i.setInt(0);   cout<<"Json must be 0: ["<<i<<"]\n";
		i.setInt(-23323);   cout<<"Json must be -23323: ["<<i<<"]\n";

		r.setReal(NAN);   cout<<"Json must be NAN: ["<<r<<"]\n";
		r.setReal(INFINITY);   cout<<"Json must be INFINITY: ["<<r<<"]\n";
		r.setReal(232);   cout<<"Json must be 232: ["<<r<<"]\n";
		r.setReal(.232);   cout<<"Json must be .232: ["<<r<<"]\n";
		r.setReal(32.);   cout<<"Json must be 32.: ["<<r<<"]\n";
		r.setReal(.34e3);   cout<<"Json must be .34e3: ["<<r<<"]\n";
		r.setReal(.35E3);   cout<<"Json must be .35E3: ["<<r<<"]\n";
		r.setReal(36e3);   cout<<"Json must be 36e3: ["<<r<<"]\n";

		s.setString(std::string("dsd"));   cout<<"Json must be \"dsd\": ["<<s<<"]\n";
		s.setString(std::string());   cout<<"Json must be empty: ["<<s<<"]\n";

		cs.setString("ssdd");   cout<<"Json must be \"ssdd\": ["<<cs<<"]\n";
		cs.setString(NULL);   cout<<"Json must be empty: ["<<cs<<"]\n";
		cs.setString("");   cout<<"Json must be empty: ["<<cs<<"]\n";

		a.setArray();   cout<<"Json must be array: ["<<a<<"]\n";
		d.setDict();   cout<<"Json must be object: ["<<d<<"]\n";

		cout<<"-----------[ Assign Signle Element ]---------------\n";

		d = false;   cout<<"Json must be false: ["<<d<<"]\n";
		d = true;   cout<<"Json must be true: ["<<d<<"]\n";
		d = 32;   cout<<"Json must be 32: ["<<d<<"]\n";
		d = 0;   cout<<"Json must be 0: ["<<d<<"]\n";
		// d = NULL;   cout<<"Json must be NULL: ["<<d<<"]\n";
		d = nullptr;   cout<<"Json must be nullptr: ["<<d<<"]\n";
		d = .35E3;   cout<<"Json must be .35E3: ["<<d<<"]\n";
		d = "";   cout<<"Json must be empty: ["<<d<<"]\n";
		d = "sw223";   cout<<"Json must be \"sw223\": ["<<d<<"]\n";
		d = std::string();   cout<<"Json must be empty: ["<<d<<"]\n";
		d = std::string("wqqwq");   cout<<"Json must be \"wqqwq\": ["<<d<<"]\n";
	}

	cout<<"-----------[ Push Signle Element ]---------------\n";
	{
		Json node;

		node.push(32);
		node.push(32.);
		node.push(.32);
		node.push(36e3);
		node.push(NAN);
		node.push(INFINITY);
		node.pushNull();
		node.push(0);
		//node.push(NULL);
		node.push(nullptr);
		node.push(false);
		node.push(true);
		node.push("");
		node.push("sdssds");
		node.push(std::string());
		node.push(std::string("wewwwew"));
		JsonPtr arr = node.pushArray();
		JsonPtr dict = node.pushDict();

		cout<<arr<<endl;
		cout<<dict<<endl;
		cout<<node<<endl;
		cout<<node.str()<<endl;
	}

	cout<<"-----------[ add & remove Signle Element ]---------------\n";
	{
		Json node;

		node.add("323", 323);
		node.add("23.", 23.);			//-- {"23":{"":23}}
		node.add(".234", .234);			//-- {"":{"234":0.234}}
		node.add("36e3", 36e3);
		node.add("NAN", NAN);
		node.add("INFINITY", INFINITY);
		node.addNull("null");
		//node.add("NULL", NULL);
		node.add("nullptr", nullptr);
		node.add("true", true);
		node.add("false", false);
		cout<<node<<endl;

		node.remove("");
		cout<<node<<endl;

		node.add("", "");
		node.add("dsds", "dsds");
		cout<<node<<endl;

		node.remove("");
		cout<<node<<endl;

		node.add(std::string(), std::string());
		node.add(std::string("wewwwew"), std::string("wewwwew"));
		node.addArray("array");
		node.addDict("object");

		cout<<node<<endl;
	}

	cout<<"-----------[ check & fetch Signle Element ]---------------\n";
	{
		Json node;

		node.add("323", 323);
		node.add("23.", 23.);
		JsonPtr na = node.addArray("array");
		JsonPtr no = node.addDict("object");
		na->push(12);
		na->push("sas");
		no->add("aa", "aa");
		no->addNull("bb");

		node["array"][7] = 128;

		cout<<node<<endl;
		//-- type checking 
		cout<<"node array[4] must be uninit: "<<((node["array"][4].type() == Json::JSON_Uninit) ? "true" : "false")<<endl;
		cout<<"node object.bb must be null: "<<((node["object.bb"].type() == Json::JSON_Null) ? "true" : "false")<<endl;
		cout<<"node object must be object: "<<((node.type("object") == Json::JSON_Object) ? "true" : "false")<<endl;
		cout<<"node object.aa must be string: "<<((node.type("object.aa") == Json::JSON_String) ? "true" : "false")<<endl;

		//-- isNull() checking
		cout<<"node object.bb must be null: "<<(node["object.bb"].isNull() ? "true" : "false")<<endl;
		cout<<"node object.bb must be null: "<<(node.isNull("object.bb") ? "true" : "false")<<endl;

		//-- exit() checking
		cout<<"node object.aa must be exist: "<<(node.exist("object.aa") ? "true" : "false")<<endl;
		cout<<"node object.dd exist? : "<<(node.exist("object.dd") ? "true" : "false")<<endl;
	}
}

template <typename T>
void printVector(const char * intro, const std::vector<T>& vec)
{
	cout<<intro<<endl;
	for (auto t: vec)
		cout<<t<<endl;
}

template <typename T>
void printMap(const char * intro, const std::map<std::string, T>& m)
{
	cout<<intro<<endl;
	for (auto& p: m)
		cout<<p.first<<" : "<<p.second<<endl;
}

void fetchSignleElement()
{
	Json node;
	node.addNull("aaa.bbb.null");
	node.add("....", "empty_objects");
	node.add("aaa.bbb.false", false);
	node.add("aaa.bbb.true", true);
	node.add("aaa.bbb.int", -34);
	node.add("aaa.bbb.real", -3.45e4);
	node.add("aaa.bbb.string", "cstring");

	node["demo.idx.int"] = 99;
	node["demo.idx.bool"] = true;
	node["demo.idx.real"] = 99.99;
	node["demo.idx.string"] = "demo string";

	node["demo.idx.array"][3]["ss.int"] = 99;	

	JsonPtr db, di, dr, ds;
	db = node.addDict("demo-dict-bool", "-");
	di = node.addDict("demo-dict-int", "-");
	dr = node.addDict("demo-dict-real", "-");
	ds = node.addDict("demo-dict-string", "-");
	for (int i = 0; i < 6; i++)
	{
		string key = std::string("key-").append(std::to_string(i));
		db->add(key, (bool)(i % 2));
		di->add(key, i);
		dr->add(key, i * 0.1);
		ds->add(key, std::string("number: ").append(std::to_string(i)));

		node["demo.array.bool"][i] = (bool)(i % 2);
		node["demo.array.int"][i] = i;
		node["demo.array.real"][i] = i * 0.1;
		node["demo.array.string"][i] = std::string("number: ").append(std::to_string(i));
	}

	cout<<"-----------[ Fetch demo Json ]---------------\n";
	cout<<"Json:"<<endl<<node<<endl;
	JsonPtr tmp;
	cout<<"-----------[ get exist Element ]---------------\n";
	{
		tmp = node.getNode("aaa.bbb.null");
		cout<<"node aaa.bbb.null is "<<(tmp->isNull() ? "null" : "un-null")<<endl;
		tmp = node.getNode("....");
		cout<<"node .... is "<<tmp->getString("<node dose not exist>")<<endl;
		tmp = node.getNode("aaa.bbb.false");
		cout<<"node aaa.bbb.false is "<<tmp->getBool(true)<<endl;
		tmp = node.getNode("aaa.bbb.true");
		cout<<"node aaa.bbb.true is "<<tmp->getBool()<<endl;
		tmp = node.getNode("aaa.bbb.int");
		cout<<"node aaa.bbb.int is "<<tmp->getInt()<<endl;
		tmp = node.getNode("aaa.bbb.real");
		cout<<"node aaa.bbb.real is "<<tmp->getReal()<<endl;
		tmp = node.getNode("aaa.bbb.string");
		cout<<"node aaa.bbb.string is "<<tmp->getString()<<endl;

		cout<<endl;

		cout<<"node aaa.bbb.null is "<<(node.isNull("aaa.bbb.null") ? "null" : "un-null")<<endl;
		cout<<"node .... is "<<node.getStringAt("....")<<endl;
		cout<<"node aaa.bbb.false is "<<node.getBool("aaa.bbb.false", true)<<endl;
		cout<<"node aaa.bbb.true is "<<node.getBool("aaa.bbb.true")<<endl;
		cout<<"node aaa.bbb.int is "<<node.getInt("aaa.bbb.int")<<endl;
		cout<<"node aaa.bbb.real is "<<node.getReal("aaa.bbb.real")<<endl;
		cout<<"node aaa.bbb.string is "<<node.getStringAt("aaa_bbb-string", "", "-_")<<endl;

		cout<<endl<<"get demo.array.string node list:"<<endl;
		tmp = node.getNode("demo.array.string");
		const std::list<JsonPtr> * const mlist = tmp->getList();
		for (const auto& json: *mlist)
			cout<<"array node demo.array.string value: "<<json<<endl;

		cout<<endl<<"get demo.array.string node list <2>:"<<endl;
		const std::list<JsonPtr> * const mlist2 = node.getList("demo.array.string");
		for (const auto& json: *mlist2)
			cout<<"array node demo.array.string value: "<<json<<endl;

		cout<<endl<<"get demo.dict.string node dict:"<<endl;
		tmp = node.getNode("demo.dict.string");
		const std::map<std::string, JsonPtr> * const mDict = tmp->getDict();
		for (const auto& jsonPair: *mDict)
			cout<<"dict node demo.dict.string key: "<<jsonPair.first<<", value: "<<jsonPair.second<<endl;

		cout<<endl<<"get demo.dict.string node dict <2>:"<<endl;
		const std::map<std::string, JsonPtr> * const mDict2 = node.getDict("demo.dict.string");
		for (const auto& jsonPair: *mDict2)
			cout<<"dict node demo.dict.string key: "<<jsonPair.first<<", value: "<<jsonPair.second<<endl;
	}

	cout<<"-----------[ get not exist or error Element ]---------------\n";

	{
		tmp = node.getNode("aaa.noo.null");
		cout<<"node aaa.noo.null is "<<(tmp ? "existed" : "not existed")<<endl;

		cout<<"node aaa.aka.null is "<<(node.isNull("aaa.aka.null") ? "null" : "un-null")<<endl;
		cout<<"node ..a.. is "<<node.getStringAt("..a..", "<not exist>")<<endl;
		cout<<"node aaa.kok.false is "<<node.getBool("aaa.kok.false", true)<<endl;
		cout<<"node aaa.bbc.true is "<<node.getBool("aaa.bbc.true")<<endl;
		cout<<"node aaa.bio.int is "<<node.getInt("aaa.bio.int")<<endl;
		cout<<"node aaa.bio.int is "<<node.getInt("aaa.bio.int", 30)<<endl;
		cout<<"node aaa.atc.real is "<<node.getReal("aaa.atc.real")<<endl;
		cout<<"node aaa.atc.real is "<<node.getReal("aaa.atc.real", 23.45)<<endl;
		cout<<"node aaa.atc.string is "<<node.getStringAt("aaa_atc-string", "<not exist>", "-_")<<endl;

		cout<<endl<<"get dna.array.string node list:"<<endl;
		const std::list<JsonPtr> * const mlist = node.getList("dna.array.string");
		cout<<"dna.array.string is "<<(mlist ? "exist" : "not exist")<<endl;

		cout<<endl<<"get dna.dict.string node dict:"<<endl;
		const std::map<std::string, JsonPtr> * const mDict = node.getDict("dna.dict.string");
		cout<<"dna.dict.string is "<<(mDict ? "exist" : "not exist")<<endl;

	}

	cout<<"-----------[ want exist Element ]---------------\n";

	{
		cout<<"node .... is "<<node.wantString("....")<<endl;
		cout<<"node aaa.bbb.false is "<<node.wantBool("aaa.bbb.false")<<endl;
		cout<<"node aaa.bbb.true is "<<node.wantBool("aaa.bbb.true")<<endl;
		cout<<"node aaa.bbb.int is "<<node.wantInt("aaa.bbb.int")<<endl;
		cout<<"node aaa.bbb.real is "<<node.wantReal("aaa.bbb.real")<<endl;
		cout<<"node aaa.bbb.string is "<<node.wantStringAt("aaa.bbb.string")<<endl;

		{
			tmp = node.getNode("demo.array.bool");
			std::vector<bool> vb = tmp->wantBoolVector(); printVector("demo.array.bool", vb);
			tmp = node.getNode("demo.array.real");
			std::vector<double> vd = tmp->wantRealVector(); printVector("demo.array.real", vd);
			tmp = node.getNode("demo.array.int");
			std::vector<intmax_t> vi = tmp->wantIntVector(); printVector("demo.array.int", vi);
			tmp = node.getNode("demo.array.string");
			std::vector<std::string> vs = tmp->wantStringVector(); printVector("demo.array.string", vs);

			tmp = node.getNode("demo.dict.bool");
			std::map<std::string, bool> mb = tmp->wantBoolDict(); printMap("demo.dict.bool", mb);
			tmp = node.getNode("demo.dict.real");
			std::map<std::string, double> md = tmp->wantRealDict(); printMap("demo.dict.real", md);
			tmp = node.getNode("demo.dict.int");
			std::map<std::string, intmax_t> mi = tmp->wantIntDict(); printMap("demo.dict.int", mi);
			tmp = node.getNode("demo.dict.string");
			std::map<std::string, std::string> ms = tmp->wantStringDict(); printMap("demo.dict.string", ms);
		}

		{
			std::vector<bool> vb = node.wantBoolVector("demo.array.bool"); printVector("demo.array.bool", vb);
			std::vector<double> vd = node.wantRealVector("demo.array.real"); printVector("demo.array.real", vd);
			std::vector<intmax_t> vi = node.wantIntVector("demo.array.int"); printVector("demo.array.int", vi);
			std::vector<std::string> vs = node.wantStringVector("demo.array.string"); printVector("demo.array.string", vs);

			std::map<std::string, bool> mb = node.wantBoolDict("demo.dict.bool"); printMap("demo.dict.bool", mb);
			std::map<std::string, double> md = node.wantRealDict("demo.dict.real"); printMap("demo.dict.real", md);
			std::map<std::string, intmax_t> mi = node.wantIntDict("demo.dict.int"); printMap("demo.dict.int", mi);
			std::map<std::string, std::string> ms = node.wantStringDict("demo.dict.string"); printMap("demo.dict.string", ms);
		}

		{
			bool b = node["demo.idx.bool"];          cout<<"node demo.idx.bool is "<<b<<endl;
			float f = node["demo.idx.real"];         cout<<"node demo.idx.real (float) is "<<f<<endl;
			double d = node["demo.idx.real"];        cout<<"node demo.idx.real (double) is "<<d<<endl;
			long double ld = node["demo.idx.real"];  cout<<"node demo.idx.real (long double) is "<<ld<<endl;
			string str = node["demo.idx.string"];    cout<<"node demo.idx.string is "<<str<<endl;

			intmax_t imax = node["demo.idx.array"][3]["ss.int"];
			cout<<"node[\"demo.idx.array\"][3][\"ss.int\"] is "<<imax<<endl;
			
			short s = node["demo.idx.int"];                 cout<<"node demo.idx.int (short) is "<<s<<endl;
			unsigned short us = node["demo.idx.int"];       cout<<"node demo.idx.int (ushort) is "<<us<<endl;
			int i = node["demo.idx.int"];                   cout<<"node demo.idx.int (int) is "<<i<<endl;
			unsigned int ui = node["demo.idx.int"];         cout<<"node demo.idx.int (uint) is "<<ui<<endl;
			long l = node["demo.idx.int"];                  cout<<"node demo.idx.int (long) is "<<l<<endl;
			unsigned long ul = node["demo.idx.int"];        cout<<"node demo.idx.int (ulong) is "<<ul<<endl;
			long long ll = node["demo.idx.int"];            cout<<"node demo.idx.int (longlong) is "<<ll<<endl;
			unsigned long long ull = node["demo.idx.int"];  cout<<"node demo.idx.int (ulonglong) is "<<ull<<endl;
		}
	}

	cout<<"-----------[ want not exist or error Element ]---------------\n";
	{
		{
			try{
				tmp = node.getNode("aaa.bbb.real");
				cout<<tmp->wantString()<<endl;
			} catch (const FpnnLogicError& ex) { cout<<"want string at node aaa.bbb.real error: "<<ex.what()<<endl; }

			try{
				cout<<node.wantString("aaa.bbb.int")<<endl;
			} catch (const FpnnLogicError& ex) { cout<<"want string at node aaa.bbb.int error: "<<ex.what()<<endl; }
			try{
				cout<<node.wantStringAt("aaa.bbb.int")<<endl;
			} catch (const FpnnLogicError& ex) { cout<<"want string at node aaa.bbb.int error: "<<ex.what()<<endl; }

			try{
				cout<<node.wantBool("aaa.bbb.int")<<endl;
			} catch (const FpnnLogicError& ex) { cout<<"want bool at node aaa.bbb.int error: "<<ex.what()<<endl; }
			try{
				cout<<node.wantInt("aaa.bbb.string")<<endl;
			} catch (const FpnnLogicError& ex) { cout<<"want int at node aaa.bbb.string error: "<<ex.what()<<endl; }
			try{
				cout<<node.wantReal("aaa.bbb.string")<<endl;
			} catch (const FpnnLogicError& ex) { cout<<"want real at node aaa.bbb.string error: "<<ex.what()<<endl; }

			try{
				cout<<node.wantReal("aaa.akt.string")<<endl;
			} catch (const FpnnLogicError& ex) { cout<<"want real at node aaa.bbb.string error: "<<ex.what()<<endl; }
		}

		{
			try{
				tmp = node.getNode("demo.array.int");
				std::vector<bool> vb = tmp->wantBoolVector(); printVector("demo.array.int", vb);
			} catch (const FpnnLogicError& ex) { cout<<"want bool vector at node demo.array.int error: "<<ex.what()<<endl; }
			try{
				tmp = node.getNode("demo.array.string");
				std::vector<double> vd = tmp->wantRealVector(); printVector("demo.array.string", vd);
			} catch (const FpnnLogicError& ex) { cout<<"want real vector at node aaa.bbb.string error: "<<ex.what()<<endl; }
			try{
				tmp = node.getNode("demo.array.string");
				std::vector<intmax_t> vi = tmp->wantIntVector(); printVector("demo.array.string", vi);
			} catch (const FpnnLogicError& ex) { cout<<"want int vector at node aaa.bbb.string error: "<<ex.what()<<endl; }
			try{
				tmp = node.getNode("demo.array.int");
				std::vector<std::string> vs = tmp->wantStringVector(); printVector("demo.array.int", vs);
			} catch (const FpnnLogicError& ex) { cout<<"want string vector at demo.array.int error: "<<ex.what()<<endl; }

			try{
				tmp = node.getNode("demo.dict.int");
				std::map<std::string, bool> mb = tmp->wantBoolDict(); printMap("demo.dict.int", mb);
			} catch (const FpnnLogicError& ex) { cout<<"want bool map at node aaa.bbb.int error: "<<ex.what()<<endl; }
			try{
				tmp = node.getNode("demo.dict.string");
				std::map<std::string, double> md = tmp->wantRealDict(); printMap("demo.dict.string", md);
			} catch (const FpnnLogicError& ex) { cout<<"want real map at node aaa.bbb.string error: "<<ex.what()<<endl; }
			try{
				tmp = node.getNode("demo.dict.string");
				std::map<std::string, intmax_t> mi = tmp->wantIntDict(); printMap("demo.dict.string", mi);
			} catch (const FpnnLogicError& ex) { cout<<"want int map at node aaa.bbb.string error: "<<ex.what()<<endl; }
			try{
				tmp = node.getNode("demo.dict.int");
				std::map<std::string, std::string> ms = tmp->wantStringDict(); printMap("demo.dict.int", ms);
			} catch (const FpnnLogicError& ex) { cout<<"want string map at node aaa.bbb.int error: "<<ex.what()<<endl; }
		}

		{
			try{
				std::vector<bool> vb = node.wantBoolVector("demo.array.int"); printVector("demo.array.int", vb);
			} catch (const FpnnLogicError& ex) { cout<<"want bool vector at node aaa.bbb.int error: "<<ex.what()<<endl; }
			try{
				std::vector<double> vd = node.wantRealVector("demo.array.string"); printVector("demo.array.string", vd);
			} catch (const FpnnLogicError& ex) { cout<<"want real vector at node aaa.bbb.string error: "<<ex.what()<<endl; }
			try{
				std::vector<intmax_t> vi = node.wantIntVector("demo.array.string"); printVector("demo.array.string", vi);
			} catch (const FpnnLogicError& ex) { cout<<"want int vector at node aaa.bbb.string error: "<<ex.what()<<endl; }
			try{
				std::vector<std::string> vs = node.wantStringVector("demo.array.int"); printVector("demo.array.int", vs);
			} catch (const FpnnLogicError& ex) { cout<<"want string vector at node aaa.bbb.int error: "<<ex.what()<<endl; }

			try{
				std::map<std::string, bool> mb = node.wantBoolDict("demo.dict.int"); printMap("demo.dict.int", mb);
			} catch (const FpnnLogicError& ex) { cout<<"want bool map at node aaa.bbb.int error: "<<ex.what()<<endl; }
			try{
				std::map<std::string, double> md = node.wantRealDict("demo.dict.string"); printMap("demo.dict.string", md);
			} catch (const FpnnLogicError& ex) { cout<<"want real map at node aaa.bbb.string error: "<<ex.what()<<endl; }
			try{
				std::map<std::string, intmax_t> mi = node.wantIntDict("demo.dict.string"); printMap("demo.dict.string", mi);
			} catch (const FpnnLogicError& ex) { cout<<"want inr map at node aaa.bbb.string error: "<<ex.what()<<endl; }
			try{
				std::map<std::string, std::string> ms = node.wantStringDict("demo.dict.int"); printMap("demo.dict.int", ms);
			} catch (const FpnnLogicError& ex) { cout<<"want string map at node aaa.bbb.int error: "<<ex.what()<<endl; }
		}

		{
			try{
				bool b = node["demo.idx.int"];          cout<<"node demo.idx.int is "<<b<<endl;
			} catch (const FpnnLogicError& ex) { cout<<"want bool at node aaa.bbb.int error: "<<ex.what()<<endl; }
			try{
				float f = node["demo.idx.bool"];         cout<<"node demo.idx.bool (float) is "<<f<<endl;
			} catch (const FpnnLogicError& ex) { cout<<"want float at node aaa.bbb.bool error: "<<ex.what()<<endl; }
			try{
				double d = node["demo.idx.bool"];        cout<<"node demo.idx.bool (double) is "<<d<<endl;
			} catch (const FpnnLogicError& ex) { cout<<"want double at node aaa.bbb.bool error: "<<ex.what()<<endl; }
			try{
				long double ld = node["demo.idx.bool"];  cout<<"node demo.idx.bool (long double) is "<<ld<<endl;
			} catch (const FpnnLogicError& ex) { cout<<"want long double at node aaa.bbb.bool error: "<<ex.what()<<endl; }
			try{
				string str = node["demo.idx.bool"];    cout<<"node demo.idx.bool is "<<str<<endl;
			} catch (const FpnnLogicError& ex) { cout<<"want string at node aaa.bbb.bool error: "<<ex.what()<<endl; }
			
			try{
				short s = node["demo.idx.bool"];                 cout<<"node demo.idx.bool (short) is "<<s<<endl;
			} catch (const FpnnLogicError& ex) { cout<<"want short at node aaa.bbb.string error: "<<ex.what()<<endl; }
			try{
				unsigned short us = node["demo.idx.bool"];       cout<<"node demo.idx.bool (ushort) is "<<us<<endl;
			} catch (const FpnnLogicError& ex) { cout<<"want ushort at node aaa.bbb.bool error: "<<ex.what()<<endl; }
			try{
				int i = node["demo.idx.bool"];                   cout<<"node demo.idx.bool (int) is "<<i<<endl;
			} catch (const FpnnLogicError& ex) { cout<<"want int at node aaa.bbb.bool error: "<<ex.what()<<endl; }
			try{
				unsigned int ui = node["demo.idx.bool"];         cout<<"node demo.idx.bool (uint) is "<<ui<<endl;
			} catch (const FpnnLogicError& ex) { cout<<"want uint at node aaa.bbb.bool error: "<<ex.what()<<endl; }
			try{
				long l = node["demo.idx.bool"];                  cout<<"node demo.idx.bool (long) is "<<l<<endl;
			} catch (const FpnnLogicError& ex) { cout<<"want long at node aaa.bbb.bool error: "<<ex.what()<<endl; }
			try{
				unsigned long ul = node["demo.idx.bool"];        cout<<"node demo.idx.bool (ulong) is "<<ul<<endl;
			} catch (const FpnnLogicError& ex) { cout<<"want ulong at node aaa.bbb.bool error: "<<ex.what()<<endl; }
			try{
				long long ll = node["demo.idx.bool"];            cout<<"node demo.idx.bool (longlong) is "<<ll<<endl;
			} catch (const FpnnLogicError& ex) { cout<<"want long long at node aaa.bbb.bool error: "<<ex.what()<<endl; }
			try{
				unsigned long long ull = node["demo.idx.bool"];  cout<<"node demo.idx.bool (ulonglong) is "<<ull<<endl;
			} catch (const FpnnLogicError& ex) { cout<<"want ulonglong at node aaa.bbb.bool error: "<<ex.what()<<endl; }
		}
	}

}

int main()
{
	assignSignleElement();
	fetchSignleElement();
	return 0;
}
