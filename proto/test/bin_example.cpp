#include "FPReader.h"
#include "FPWriter.h"
#include "FPMessage.h"
#include "JSONConvert.h"
#include "msec.h"
#include <sys/time.h>
#include <unistd.h>
#include <iostream>     // std::cout
#include <fstream>

using namespace std;
using namespace fpnn;

string readFile(const string& name){
	string data;
	ifstream is (name.c_str(), ifstream::binary);
	if (is) {
		// get length of file:
		is.seekg (0, is.end);
		int length = is.tellg();
		is.seekg (0, is.beg);

		char * buffer = new char [length];

		// read data as a block:
		is.read (buffer,length);

		is.close();

		data = string(buffer, length);
		delete[] buffer;
	}
	return data;
}

int main(int argc, char* argv[]){
	map<int, int> intMap;
	intMap[1]=1;
	intMap[2]=2;
	map<string, string> stringMap;
	stringMap["one"]="one";
	stringMap["two"]="two";

	string a = readFile("./bin/a");
	string b = readFile("./bin/b");
	string c = readFile("./bin/c");
	string d = readFile("./bin/d");
	string e = readFile("./bin/e");

	FPQWriter qw(13, "TESTMAP");
	qw.param("int", intMap);
	qw.param("stringa", a);
	qw.param("stringb", b);
	qw.param("stringc", c);
	qw.param("stringd", d);
	qw.param("stringe", e);
	qw.param("string", stringMap);
	qw.paramBinary("bina", a.data(), a.size());
	qw.paramBinary("binb", b.data(), b.size());
	qw.paramBinary("binc", c.data(), c.size());
	qw.paramBinary("bind", d.data(), d.size());
	qw.paramBinary("bine", e.data(), e.size());
	qw.param("string2", stringMap);

	FPQuestPtr quest = qw.take();
	FPQReader qr(quest);

	string ba = qr.get("bina", string());
	string sa = qr.get("stringa", string());
	cout<<"a len: " << a.size() <<endl;
	cout<<"ba len: " << ba.size() <<endl;
	cout<<"sa len: " << sa.size() <<endl;
	cout<<"ba == a: " << (ba == a ? "yes" : "no") <<endl;
	cout<<"sa == a: " << (sa == a ? "yes" : "no") <<endl<<endl;

	string bb = qr.get("binb", string());
	string sb = qr.get("stringb", string());
	cout<<"b len: " << b.size() <<endl;
	cout<<"bb len: " << bb.size() <<endl;
	cout<<"sb len: " << sb.size() <<endl;
	cout<<"bb == b: " << (bb == b ? "yes" : "no") <<endl;
	cout<<"sb == b: " << (sb == b ? "yes" : "no") <<endl<<endl;

	string bc = qr.get("binc", string());
	string sc = qr.get("stringc", string());
	cout<<"c len: " << c.size() <<endl;
	cout<<"bc len: " << bc.size() <<endl;
	cout<<"sc len: " << sc.size() <<endl;
	cout<<"bc == c: " << (bc == c ? "yes" : "no") <<endl;
	cout<<"sc == c: " << (sc == c ? "yes" : "no") <<endl<<endl;

	string bd = qr.get("bind", string());
	string sd = qr.get("stringd", string());
	cout<<"d len: " << d.size() <<endl;
	cout<<"bd len: " << bd.size() <<endl;
	cout<<"sd len: " << sd.size() <<endl;
	cout<<"bd == d " << (bd == d ? "yes" : "no") <<endl;
	cout<<"sd == d " << (sd == d ? "yes" : "no") <<endl<<endl;

	string be = qr.get("bine", string());
	string se = qr.get("stringe", string());
	cout<<"e len: " << e.size() <<endl;
	cout<<"be len: " << be.size() <<endl;
	cout<<"se len: " << se.size() <<endl;
	cout<<"be == e: " << (be == e ? "yes" : "no") <<endl;
	cout<<"se == e: " << (se == e ? "yes" : "no") <<endl<<endl;

	int32_t start = time(NULL);
	for(size_t i = 0; i < 2000; ++i){
		FPQWriter qw(5, "TESTSting");
		qw.param("stringa", a);
		qw.param("stringb", b);
		qw.param("stringc", c);
		qw.param("stringd", d);
		qw.param("stringe", e);
		FPQuestPtr quest = qw.take();
		FPQReader qr(quest);
		string sa = qr.get("stringa", string());
		string sb = qr.get("stringb", string());
		string sc = qr.get("stringc", string());
		string sd = qr.get("stringd", string());
		string se = qr.get("stringe", string());
	}
	int32_t end = time(NULL);
	cout<<"String test COST:"<<end - start<<endl;

	start = time(NULL);
	for(size_t i = 0; i < 2000; ++i){
		FPQWriter qw(5, "TESTBIN");
		qw.paramBinary("bina", a.data(), a.size());
		qw.paramBinary("binb", b.data(), b.size());
		qw.paramBinary("binc", c.data(), c.size());
		qw.paramBinary("bind", d.data(), d.size());
		qw.paramBinary("bine", e.data(), e.size());
		FPQuestPtr quest = qw.take();
		FPQReader qr(quest);
		string ba = qr.get("bina", string());
		string bb = qr.get("binb", string());
		string bc = qr.get("binc", string());
		string bd = qr.get("bind", string());
		string be = qr.get("bine", string());
	}
	end = time(NULL);
	cout<<"BIN test COST:"<<end - start<<endl;

	return 0;
}

