#include "FPReader.h"
#include "FPWriter.h"
#include "FPMessage.h"
#include "JSONConvert.h"
#include "msec.h"
#include <sys/time.h>
#include <unistd.h>
#include <fstream>
#include <uuid.h>

using namespace std;
using namespace fpnn;

int main(int argc, char* argv[]){
	map<int, int> intMap;
	intMap[1]=1;
	intMap[2]=2;
	map<string, string> stringMap;
	stringMap["one"]="one";
	stringMap["two"]="two";
	FPQWriter qwwww(2, "TESTMAP");
	qwwww.param("int", intMap);
	qwwww.param("string", stringMap);

	FPQuestPtr quest0 = qwwww.take();
	cout<<"json:"<<quest0->json()<<endl;
	FPQReader qr0(quest0);
	intMap = qr0.get("int", map<int,int>());
	stringMap = qr0.get("string", map<string,string>());
	for(map<int, int>::iterator it = intMap.begin(); it != intMap.end(); ++it){
		cout<<it->first<<"=>"<<it->second<<endl;
	}
	for(map<string, string>::iterator it = stringMap.begin(); it != stringMap.end(); ++it){
		cout<<it->first<<"=>"<<it->second<<endl;
	}

	FPQWriter qw(2,"dbResultDemo");
	qw.paramArray("fields", 4);
	qw.param("user");
	qw.param("name");
	qw.param("gender");
	qw.param("birth");
	qw.paramArray("rows", 2);//2 rows
	qw.paramArray(4);//4 data each row;
	qw.param("fp user");
	qw.param("fp-name");
	qw.param("fp-gender");
	qw.param("fp-birth");
	qw.paramArray(4);//4 data each row;
	qw.param("fp user2");
	qw.param("fp-name2");
	qw.param("fp-gender2");
	qw.param("fp-birth2");

	cout<<"json:"<<qw.json()<<endl;

	vector<string> field;
	field.push_back("user");
	field.push_back("name");
	field.push_back("gender");
	field.push_back("birth");
	
	vector<string> row1;
	row1.push_back("fp user");
	row1.push_back("fp-name");
	row1.push_back("fp-gender");
	row1.push_back("fp-birth");
	vector<string> row2;
	row2.push_back("fp user2");
	row2.push_back("fp-name2");
	row2.push_back("fp-gender2");
	row2.push_back("fp-birth2");

	FPQWriter qw2(2,"dbResultDemo2");
	qw2.param("fields", field);
	qw2.paramArray("rows",2);
	qw2.param(row1);
	qw2.param(row2);
	cout<<"json2:"<<qw2.json()<<endl;

	FPQuestPtr quest = qw.take();

	FPQReader qr(quest);

	cout<<"FIELDS:"<<endl;
	vector<string> field2 = qr.get("fields", vector<string>()); 
	for(size_t i = 0; i < field2.size(); ++i){
		cout<<field2[i]<<",";
	}
	cout<<endl;

	cout<<"rows:"<<endl;
	vector<vector<string> > rows2 = qr.get("rows", vector<vector<string> >());
	for(size_t i = 0; i < rows2.size(); ++i){
		cout<<"row"<<i<<endl;
		for(size_t j = 0; j < rows2[i].size(); ++j){
			cout<<rows2[i][j]<<",";
		}
		cout<<endl;
	}

	{

		FPQWriter qw(2000,"testZip");
		for(size_t i = 0; i < 1000; ++i){
			qw.param(string("intField") + to_string(i), rand());

			uuid_t uuid;
			char buf[UUID_STRING_LEN+1] = {0};
			uuid_generate(uuid);
			uuid_string(uuid, buf, sizeof(buf));
			qw.param(string("stringField") + to_string(i), string(buf, UUID_STRING_LEN));
		}
		FPQuestPtr quest = qw.take();
		ofstream f1("aa", ofstream::binary);
		f1.write(qw.raw().data(), qw.raw().size());
		cout<<"Data Len:"<<qw.raw().size()<<endl;
		f1.close();
	}

	return 0;
}

