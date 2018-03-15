#include "FPReader.h"
#include "FPWriter.h"
#include "FPMessage.h"
#include "JSONConvert.h"
#include "msec.h"
#include <sys/time.h>
#include <unistd.h>

using namespace std;
using namespace fpnn;

struct TEST{
	int id;
	set<int> ids;
	vector<int> ids2;
	MSGPACK_DEFINE(id, ids,ids2);//must do this, ATTENTION!!!!!!
};

int main(int argc, char* argv[]){
	//first time
	vector<TEST> tt;
	TEST t;
	t.id = 1;
	t.ids.insert(1);
	t.ids.insert(2);
	t.ids2.push_back(3);
	t.ids2.push_back(4);
	t.ids2.push_back(5);
	tt.push_back(t);


	TEST t2;
	t2.id = 2;
	t2.ids.insert(2);
	t2.ids.insert(3);
	t2.ids.insert(33);
	t2.ids2.push_back(4);
	t2.ids2.push_back(5);
	t2.ids2.push_back(6);
	t2.ids2.push_back(7);
	tt.push_back(t2);
	FPQWriter qwwww(1, "TESTComplex");
	qwwww.param("complex", tt);

	FPQuestPtr quest0 = qwwww.take();
	cout<<"json:"<<quest0->json()<<endl;
	FPQReader qr0(quest0);
	vector<TEST> tttt;
	tttt = qr0.get("complex", vector<TEST>());
	for(size_t i = 0 ;i < tttt.size(); i++){
		cout<<"id:"<<tttt[i].id<<endl;
		cout<<"IDS:";
		for(set<int>::iterator it = tttt[i].ids.begin(); it != tttt[i].ids.end(); it++){
			cout<<*it<<",";
		}
		cout<<endl;
		cout<<"IDS2:";
		for(size_t j = 0; j < tttt[i].ids2.size(); j++){
			cout<<tttt[i].ids2[j]<<",";
		}
		cout<<endl;
	}


	//second, this is same as first time.
	FPQWriter q4(1, "TESTComplexB");
	q4.param("complexB");
	q4.paramArray(2);

	q4.paramArray(3);
	q4.param(1);
	q4.paramArray(2);
	q4.param(2);
	q4.param(3);
	q4.paramArray(2);
	q4.param(22);
	q4.param(33);

	q4.paramArray(3);
	q4.param(2);
	q4.paramArray(2);
	q4.param(3);
	q4.param(4);
	q4.paramArray(4);
	q4.param(33);
	q4.param(44);
	q4.param(55);
	q4.param(66);

	FPQuestPtr quest4 = q4.take();
	cout<<"json:"<<quest4->json()<<endl;
	FPQReader qr1(quest4);
	vector<TEST> t4;
	t4 = qr1.get("complexB", vector<TEST>());
	for(size_t i = 0 ;i < t4.size(); i++){
		cout<<"idB:"<<t4[i].id<<endl;
		cout<<"IDSB:";
		for(set<int>::iterator it = t4[i].ids.begin(); it != t4[i].ids.end(); it++){
			cout<<*it<<",";
		}
		cout<<endl;
		cout<<"IDSB2:";
		for(size_t j = 0; j < t4[i].ids2.size(); j++){
			cout<<t4[i].ids2[j]<<",";
		}
		cout<<endl;
	}

	return 0;
}

