#include "FPReader.h"
#include "FPWriter.h"
#include "FPMessage.h"
#include "JSONConvert.h"
#include "msec.h"
#include <sys/time.h>
#include <unistd.h>

using namespace std;
using namespace fpnn;

static bool printSC = true;
static bool convert = true;

static FPMessage::FP_Pack_Type def_ptype=FPMessage::FP_PACK_MSGPACK;

bool QuestEQ(const FPQuestPtr a, const FPQuestPtr b){
	string* as = a->raw();
	string* bs = b->raw();
	bool eq = (*as == *bs);
	delete as;
	delete bs;
	if(!eq) cout<<"****************** NOT same\n";
	return eq;
}

bool AnswerEQ(const FPAnswerPtr a, const FPAnswerPtr b){
	string* as = a->raw();
	string* bs = b->raw();
	bool eq = (*as == *bs);
	delete as;
	delete bs;
	if(!eq) cout<<"################### NOT same\n";
	return eq;
}


void printX(const string& data){
	if(!printSC) return;
	cout<<"LEN:"<<data.size()<<endl;
	for(size_t i = 0; i < data.size(); ++i){
		printf("%02X ", (unsigned char)data[i]);
		if((i+1) % 8 == 0)
			printf("\n");
	}
	printf("\n");
}
void printX(const FPAnswerPtr a){
	string *as = a->raw();
	printX(*as);
	delete as;
}
void printX(const FPQuestPtr a){
	string *as = a->raw();
	printX(*as);
	delete as;
}

FPQuestPtr QWriter(bool oneway){
	FPQWriter qw(6,oneway?"oneway":"twoway", oneway, def_ptype);
	qw.param("quest", "one");
	qw.param("int", 2);
	qw.param("double", 3.3);
	qw.param("boolean", true);
	qw.paramArray("ARRAY",2);
	qw.param("first_vec");
	qw.param(4);
	qw.paramMap("MAP",6);
	qw.param("map1","first_map");
	qw.param("map2",true);
	qw.param("map3",5);
	qw.param("map4",5.7);
	qw.param("map5","中文");
	qw.param("map6","{\"key\":\"value\"}");
	if(printSC){
		cout<<"json:"<<qw.json()<<endl;
	}
	if(convert && def_ptype == FPMessage::FP_PACK_MSGPACK){
		std::string jbuf = qw.json();
		std::string mbuf = JSONConvert::Json2Msgpack(jbuf);
		std::string	json = JSONConvert::Msgpack2Json(mbuf); 
		cout<<"AFTER CONVERT:"<<endl;
		cout<<json<<endl;
		assert(jbuf == json);
	}
	return qw.take();
}

void QReader(FPQuestPtr quest){
	FPQReader qr(quest);
	if(printSC){
		cout<<"Method:"<<qr.method()<<endl;
		cout<<"SeqNum:"<<qr.seqNum()<<endl;
		cout<<"isOneWay:"<<qr.isOneWay()<<endl;
		cout<<"isTCP:"<<qr.isTCP()<<endl;
		cout<<"Param:"<<endl;
		cout<<"get=>quest:"<<qr.get("quest", string(""))<<endl;
		cout<<"get=>int:"<<qr.get("int", (int)0)<<endl;
		cout<<"get=>double:"<<qr.get("double", (double)0)<<endl;
		cout<<"get=>boolean:"<<qr.get("boolean", (bool)0)<<endl;
		tuple<string, int> tup;
		tup = qr.get("ARRAY", tup);
		cout<<"get=>array_tuple:"<<std::get<0>(tup)<<"  "<<std::get<1>(tup)<<endl;
	}

	OBJECT obj = qr.getObject("MAP");
	FPReader fpr(obj);
	if(printSC){
		cout<<"object:"<<fpr.json()<<endl;
		cout<<"get=>MAP:"<<endl;
		cout<<"map1=>"<<fpr.get("map1",string(""))<<endl;
		cout<<"map2=>"<<fpr.get("map2",(bool)false)<<endl;
		cout<<"map3=>"<<fpr.get("map3",(int)0)<<endl;
		cout<<"map4=>"<<fpr.get("map4",(double)0.0)<<endl;
		cout<<"map5=>"<<fpr.get("map5",string(""))<<endl;
		try{
			cout<<"WANT:"<<fpr.want("map4", string(""))<<endl;
		}
		catch(const std::exception& ex){
			cout<<"EXECPTION: "<<ex.what()<<endl;
		}
		catch(...){
			cout<<"UNKNOW EXECPTION "<<endl;
		}

		try{
			cout<<"WANT:"<<fpr.want("map9", string(""))<<endl;
		}
		catch(const std::exception& ex){
			cout<<"EXECPTION: "<<ex.what()<<endl;
		}
		catch(...){
			cout<<"UNKNOW EXECPTION "<<endl;
		}
	}
}

void QReaderPayload(const char* payload, size_t len){
	FPReader r(payload, len);
	if(printSC){
		cout<<"READ payload Param:"<<endl;
		cout<<"get=>quest:"<<r.get("quest", string(""))<<endl;
		cout<<"get=>int:"<<r.get("int", (int)0)<<endl;
		cout<<"get=>double:"<<r.get("double", (double)0)<<endl;
		cout<<"get=>boolean:"<<r.get("boolean", (bool)0)<<endl;
		tuple<string, int> tup;
		tup = r.get("ARRAY", tup);
		cout<<"get=>array_tuple:"<<std::get<0>(tup)<<"  "<<std::get<1>(tup)<<endl;
	}
	OBJECT obj = r.getObject("MAP");
	if(printSC){
		FPReader fpr(obj);
		cout<<"object:"<<fpr.json()<<endl;
		cout<<"get=>MAP:"<<endl;
		cout<<"map1=>"<<fpr.get("map1",string(""))<<endl;
		cout<<"map2=>"<<fpr.get("map2",(bool)false)<<endl;
		cout<<"map3=>"<<fpr.get("map3",(int)0)<<endl;
		cout<<"map4=>"<<fpr.get("map4",(double)0.0)<<endl;
		cout<<"map5=>"<<fpr.get("map5",string(""))<<endl;
		try{
			cout<<"WANT:"<<fpr.want("map4", string(""))<<endl;
		}
		catch(const std::exception& ex){
			cout<<"EXECPTION: "<<ex.what()<<endl;
		}
		catch(...){
			cout<<"UNKNOW EXECPTION "<<endl;
		}
		try{
			cout<<"WANT:"<<fpr.want("map9", string(""))<<endl;
		}
		catch(const std::exception& ex){
			cout<<"EXECPTION: "<<ex.what()<<endl;
		}
		catch(...){
			cout<<"UNKNOW EXECPTION "<<endl;
		}
	}
}


FPAnswerPtr AWriter(FPQuestPtr quest, int status = 0){
	if(status!=0 && status!=200){
		return FPAWriter::errorAnswer(quest, 300, "exceptions", "raiser");
	}
	FPAWriter aw(6, quest);
	aw.param("answer", "one");
	aw.param("int", 2);
	aw.param("double", 3.3);
	aw.param("boolean", true);
	aw.paramArray("ARRAY",2);
	aw.param(make_tuple ("tuple1", 3.1, 14, false));
	aw.param(make_tuple ("tuple2", 5.7, 9, true));
	aw.paramMap("MAP",5);
	aw.param("m1","first_map");
	aw.param("m2",true);
	aw.param("m3",5);
	aw.param("m4",5.7);
	aw.param("m5","中文2");
	if(printSC){
		cout<<"json:"<<aw.json()<<endl;
	}
	if(convert && def_ptype == FPMessage::FP_PACK_MSGPACK){
		std::string jbuf = aw.json();
		std::string mbuf = JSONConvert::Json2Msgpack(jbuf);
		std::string	json = JSONConvert::Msgpack2Json(mbuf); 
		cout<<"AFTER CONVERT:"<<endl;
		cout<<json<<endl;
		assert(jbuf == json);
	}
	return aw.take();
}

FPAnswerPtr AHTTPWriter(FPQuestPtr quest, int status = 0){
	if(status!=0 && status!=200){
		return FPAWriter::errorAnswer(quest, 400, "http test exceptions", "raiser");
	}
	FPAWriter aw(6, quest);
	aw.param("answer", "one");
	aw.param("int", 2);
	aw.param("double", 3.3);
	aw.param("boolean", true);
	aw.paramArray("ARRAY",2);
	aw.param(make_tuple ("tuple1", 3.1, 14, false));
	aw.param(make_tuple ("tuple2", 5.7, 9, true));
	aw.paramMap("MAP",5);
	aw.param("m1","first_map");
	aw.param("m2",true);
	aw.param("m3",5);
	aw.param("m4",5.7);
	aw.param("m5","中文2");
	if(printSC){
		cout<<"json:"<<aw.json()<<endl;
	}
	if(convert && def_ptype == FPMessage::FP_PACK_MSGPACK){
		std::string jbuf = aw.json();
		std::string mbuf = JSONConvert::Json2Msgpack(jbuf);
		std::string	json = JSONConvert::Msgpack2Json(mbuf); 
		cout<<"AFTER CONVERT:"<<endl;
		cout<<json<<endl;
		assert(jbuf == json);
	}
	return aw.take();
}

void AReader(FPAnswerPtr answer){
	FPAReader ar(answer);
	if(printSC){
		cout<<"SeqNum:"<<ar.seqNum()<<endl;
		cout<<"Status:"<<ar.status()<<endl;
		cout<<"Param:"<<endl;
		cout<<"get=>quest:"<<ar.getString("answer")<<endl;
		cout<<"get=>int:"<<ar.getInt("int")<<endl;
		cout<<"get=>double:"<<ar.getDouble("double")<<endl;
		cout<<"get=>boolean:"<<ar.getBool("boolean")<<endl;
		vector<tuple<string, double, int, bool> > vec;
		vec = ar.get("ARRAY", vec);
		for(size_t i = 0; i < vec.size(); ++i){
			tuple<string, double, int, bool> tup = vec[i];
			cout<<"get=>array_tuple:"<<std::get<0>(tup)<<"  "<<std::get<1>(tup)<<endl;
		}
	}
	OBJECT obj = ar.getObject("MAP");
	FPReader fpr(obj);
	if(printSC){
		cout<<"object:"<<fpr.json()<<endl;
		cout<<"get=>MAP:"<<endl;
		cout<<"map1=>"<<fpr.getString("m1")<<endl;
		cout<<"map2=>"<<fpr.getBool("m2")<<endl;
		cout<<"map3=>"<<fpr.getUInt("m3")<<endl;
		cout<<"map4=>"<<fpr.getDouble("m4")<<endl;
		cout<<"map5=>"<<fpr.getString("m5")<<endl;
		try{
			cout<<"WANT:"<<fpr.wantString("m4")<<endl;
		}
		catch(const std::exception& ex){
			cout<<"EXECPTION: "<<ex.what()<<endl;
		}
		catch(...){
			cout<<"UNKNOW EXECPTION "<<endl;
		}
		try{
			cout<<"WANT:"<<fpr.want("m9", string(""))<<endl;
		}
		catch(const std::exception& ex){
			cout<<"EXECPTION: "<<ex.what()<<endl;
		}
		catch(...){
			cout<<"UNKNOW EXECPTION "<<endl;
		}
	}
}

void AErrorReader(FPAnswerPtr answer){
	FPAReader ar(answer);
	if(printSC){
		cout<<"SeqNum:"<<ar.seqNum()<<endl;
		cout<<"Status:"<<ar.status()<<endl;
		cout<<"Param:"<<endl;
		cout<<"get=>code:"<<ar.getInt("code")<<endl;
		cout<<"get=>ex:"<<ar.getString("ex")<<endl;
		cout<<"get=>msg:"<<ar.getString("msg")<<endl;
		cout<<"get=>raiser:"<<ar.getString("raiser")<<endl;
	}
}

void AErrorReaderPayload(const char* payload, size_t len){
	FPReader ar(payload, len);
	if(printSC){
		cout<<"READ ERROR payload Param:"<<endl;
		cout<<"get=>code:"<<ar.getInt("code")<<endl;
		cout<<"get=>ex:"<<ar.getString("ex")<<endl;
		cout<<"get=>msg:"<<ar.getString("msg")<<endl;
		cout<<"get=>raiser:"<<ar.getString("raiser")<<endl;
	}
}

void AReaderPayload(const char* payload, size_t len){
	FPReader ar(payload, len);
	if(printSC){
		cout<<"READ payload Param:"<<endl;
		cout<<"get=>quest:"<<ar.getString("answer")<<endl;
		cout<<"get=>int:"<<ar.getInt("int")<<endl;
		cout<<"get=>double:"<<ar.getDouble("double")<<endl;
		cout<<"get=>boolean:"<<ar.getBool("boolean")<<endl;
		vector<tuple<string, int> > vec;
		vec = ar.get("ARRAY", vec);
		for(size_t i = 0; i < vec.size(); ++i){
			tuple<string, int> tup = vec[i];
			cout<<"get=>array_tuple:"<<std::get<0>(tup)<<"  "<<std::get<1>(tup)<<endl;
		}
	}

	OBJECT obj = ar.getObject("MAP");
	FPReader fpr(obj);
	if(printSC){
		cout<<"object:"<<fpr.json()<<endl;
		cout<<"get=>MAP:"<<endl;
		cout<<"map1=>"<<fpr.getString("m1")<<endl;
		cout<<"map2=>"<<fpr.getBool("m2",(bool)false)<<endl;
		cout<<"map3=>"<<fpr.getUInt("m3",(int)0)<<endl;
		cout<<"map4=>"<<fpr.getDouble("m4",(double)0.0)<<endl;
		cout<<"map5=>"<<fpr.getString("m5",string(""))<<endl;
		try{
			cout<<"WANT:"<<fpr.wantString("m4", string(""))<<endl;
		}
		catch(const std::exception& ex){
			cout<<"EXECPTION: "<<ex.what()<<endl;
		}
		catch(...){
			cout<<"UNKNOW EXECPTION "<<endl;
		}
		try{
			cout<<"WANT:"<<fpr.want("m9", string(""))<<endl;
		}
		catch(const std::exception& ex){
			cout<<"EXECPTION: "<<ex.what()<<endl;
		}
		catch(...){
			cout<<"UNKNOW EXECPTION "<<endl;
		}
	}
}

void testSimpleMessage(){
	cout<<"TEST operator override"<<endl;
	FPQuestPtr q = FPQWriter(1,"twoway")("simple", "OK");
	if(printSC){
		cout<<"SIMPLE:"<<q->json()<<endl;
		cout<<"SIMPLE:"<<q->info()<<endl;
	}

	FPAnswerPtr a = FPAWriter(2, q)("Simple","one")("Simple2", 2);
	if(printSC){
		cout<<"SIMPLE:"<<a->json()<<endl;
		cout<<"SIMPLE:"<<a->info()<<endl;
	}

	FPQuestPtr qq = FPQWriter::emptyQuest("emptyQuestTest");
	if(printSC){
		cout<<"EmptyQuestTest:"<<qq->json()<<endl;
		cout<<"EmptyQuestTest:"<<qq->info()<<endl;
	}

	FPQWriter qw(1,"empty_Array" );
	qw.paramArray("EMPTYARRAY",0);
	if(printSC){
		FPQuestPtr q = qw.take();
		cout<<"Empty Array Test" << q->json()<<endl;
		cout<<"Empty Array Test" << q->info()<<endl;
	}

	FPQWriter qw2(1,"empty_MAP" );
	qw2.paramMap("EMPTYMAP",0);
	if(printSC){
		FPQuestPtr q = qw2.take();
		cout<<"Empty MAP Test" << q->json()<<endl;
		cout<<"Empty MAP Test" << q->info()<<endl;
	}

	{
		map<string, int> m;
		vector<int64_t> v;
		FPQuestPtr q = FPQWriter(2,"TESTEmptyArrayAndEmptyMap")("Map", m)("Vector",v);
		cout<<"TESTEmptyArrayAndEmptyMap"<<q->json()<<endl;
		cout<<"TESTEmptyArrayAndEmptyMap"<<q->info()<<endl;
		FPQReader qr(q);
		map<string, int> mm = qr.want("Map", map<string, int>());;
		mm = qr.want("Map", mm);
		vector<int64_t> vv = qr.want("Vector", vector<int64_t>());
	}

}

void testSimpleMessage2(){
	cout<<"TEST operator override222"<<endl;
	map<string, string> amap;
	amap["TEST1"]="TEST1";
	amap["TEST2"]="TEST2";
	FPQuestPtr q = FPQWriter("twoway")(amap);
	if(printSC){
		cout<<"SIMPLE:"<<q->json()<<endl;
		cout<<"SIMPLE:"<<q->info()<<endl;
	}

	FPQWriter qw("twoway2");
	qw.param(amap);
	if(printSC){
		FPQuestPtr q = qw.take();
		cout<<"SIMPLE2:"<<q->json()<<endl;
		cout<<"SIMPLE2:"<<q->info()<<endl;
	}

	FPAnswerPtr a = FPAWriter(q)(amap);
	if(printSC){
		cout<<"SIMPLE:"<<a->json()<<endl;
		cout<<"SIMPLE:"<<a->info()<<endl;
	}

	FPAWriter aw(q);
	aw.param(amap);
	if(printSC){
		FPAnswerPtr a = aw.take();
		cout<<"SIMPLE2:"<<a->json()<<endl;
		cout<<"SIMPLE2:"<<a->info()<<endl;
	}


}

void testCreateFromJson(){
	cout<<"TEST Create From JSON"<<endl;
	string json = "{\"Json1\":\"one\", \"Json2\":2, \"Json3\":true}";
	FPQWriter qw("twoway2", json);
	FPQuestPtr q = qw.take();
	if(printSC){
		cout<<"TEST Quest:"<<q->json()<<endl;
		cout<<"TEST Quest:"<<q->info()<<endl;
	}

	FPAWriter aw(json, q);
	if(printSC)
		cout<<"TEST Answer:"<<aw.take()->json()<<endl;

}

void testConvert(){
	cout<<"TEST Convert"<<endl;
	FPQuestPtr q = FPQWriter((size_t)0,"twoway");
	FPAnswerPtr a = FPAWriter(3, q)("Simple","one")("Simple2", "two")("Simple3", "three");
	if(printSC){
		cout<<"Convert:"<<a->json()<<endl;
		cout<<"Convert:"<<a->info()<<endl;
	}
	FPAReader ar(a);
	map<string, string> convert;
	convert = ar.convert(convert);
	if(printSC)
		for(map<string, string>::iterator it = convert.begin(); it != convert.end(); ++it){
			cout<<it->first<<"====>"<<it->second<<endl;
		}
}

int main(int argc, char** argv){
	if(argc != 2){
		cout<<"Usage:"<<argv[0]<<" [json/msgpack]"<<endl;
		return -1;
	}
	if(strcasecmp(argv[1], "json") == 0) def_ptype=FPMessage::FP_PACK_JSON;
	else if(strcasecmp(argv[1], "msgpack") == 0) def_ptype=FPMessage::FP_PACK_MSGPACK;
	else{
		cout<<"Usage:"<<argv[0]<<" [json/msgpack]"<<endl;
		return -1;
	}

	cout<<"ATTENTION: TEST "<< (def_ptype==FPMessage::FP_PACK_JSON?"json":"msgpack")<<endl;

	{
		//TEST one way
		FPQuestPtr oneway = QWriter(true);
		printX(oneway);
		if(printSC)
			cout<<"oneway Quest:"<< oneway->json()<<endl;
		try{
			FPAnswerPtr onewayA = AWriter(oneway);
			printX(onewayA);
		}
		catch(const std::exception& ex){
			cout<<"***********EXCEPTION::"<<ex.what()<<endl;
		}


		QReader(oneway);
		QReaderPayload(oneway->payload().data(), oneway->payload().size());

		//create quest from data
		string pl = oneway->payload();
		if(def_ptype==FPMessage::FP_PACK_JSON)
			pl = JSONConvert::Msgpack2Json(oneway->payload());
		FPQuestPtr onewayC(new FPQuest(oneway->_hdr,oneway->seqNum(),oneway->method(), pl));
		QuestEQ(oneway, onewayC);
		cout<<oneway->json()<<endl;
		string* raw = oneway->raw();
		FPQuestPtr onewayC2(new FPQuest(raw->data(),raw->size()));
		delete raw;
		QuestEQ(oneway, onewayC2);
	}

	{
		//TEST two way
		FPQuestPtr twoway = QWriter(false);
		printX(twoway);
		if(printSC)
			cout<<"twoway Quest:"<< twoway->json()<<endl;
		QReader(twoway);
		QReaderPayload(twoway->payload().data(), twoway->payload().size());
		//create quest from data
		string pl = twoway->payload();
		if(def_ptype==FPMessage::FP_PACK_JSON)
			pl = JSONConvert::Msgpack2Json(twoway->payload());
		FPQuestPtr twowayC(new FPQuest(twoway->_hdr,twoway->seqNum(),twoway->method(), pl));
		QuestEQ(twoway, twowayC);
		cout<<twoway->json()<<endl;
		string* raw = twoway->raw();
		FPQuestPtr twowayC2(new FPQuest(raw->data(),raw->size()));
		delete raw;
		QuestEQ(twoway, twowayC2);

		//create HTTP
		if(def_ptype==FPMessage::FP_PACK_JSON){
			StringMap infos;
			cout<<"HTTP:"<<pl<<endl;
			FPQuestPtr httpQuest(new FPQuest(twoway->method(), pl, infos, true));
			printX(httpQuest);
			if(printSC)
				cout<<"HTTP Quest:"<< httpQuest->json()<<endl;
			QReader(httpQuest);
			FPAnswerPtr ahttp = AWriter(httpQuest);//200 OK
			string* raw = ahttp->raw();
			std::cout<<*raw<<endl;
			delete raw;
			FPAnswerPtr ahttp2 = AWriter(httpQuest, 301);
			raw = ahttp2->raw();
			std::cout<<*raw<<endl;
			delete raw;
			AErrorReader(ahttp2);
		}

		//TEST anwser
		FPAnswerPtr twowayA = AWriter(twoway);
		printX(twowayA);
		if(printSC)
			cout<<"twoway Answer:"<< twowayA->json()<<endl;
		AReader(twowayA);
		AReaderPayload(twowayA->payload().data(), twowayA->payload().size());
		pl = twowayA->payload();
		if(def_ptype==FPMessage::FP_PACK_JSON)
			pl = JSONConvert::Msgpack2Json(twowayA->payload());
		FPAnswerPtr twowayAC(new FPAnswer(twowayA->_hdr,twowayA->seqNum(),pl));
		AnswerEQ(twowayA, twowayAC);
		FPAnswerPtr twowayAC2(new FPAnswer(*(twowayA->raw())));
		AnswerEQ(twowayA, twowayAC2);

		FPQuestPtr twoway2 = QWriter(false);
		printX(twoway2);
		FPAnswerPtr twowayA2 = AWriter(twoway2, 1);
		printX(twowayA2);
		if(printSC)
			cout<<twowayA2->json()<<endl;
		AErrorReader(twowayA2);
		AErrorReaderPayload(twowayA2->payload().data(), twowayA2->payload().size());
		if(printSC)
			cout<<"Empty Answer"<<endl;
		FPAnswerPtr emptyAnswer = FPAWriter::emptyAnswer(twoway2);
		printX(emptyAnswer);
	}
	//convert empty json
	cout<<"Convert Empty Json"<<endl;
	string ejson = "{}";
	string mmjson = JSONConvert::Json2Msgpack(ejson);
	string ejson2 = JSONConvert::Msgpack2Json(mmjson);
	cout<<"Source:"<<ejson<<endl;
	cout<<"Converted:"<<ejson2<<endl;

	testSimpleMessage();

	testSimpleMessage2();

	testCreateFromJson();

	testConvert();

	printSC = false;
	convert = false;
	//speed
	if(def_ptype==FPMessage::FP_PACK_JSON)
		cout<<"JSON PROTO:"<<endl;
	else
		cout<<"MSGPACK PROTO:"<<endl;
	size_t test_count = 2000000;
	struct timeval start;
	gettimeofday(&start, NULL);

	for(size_t i = 0; i < test_count; ++i){
		FPQuestPtr twoway = QWriter(false);
		//QReader(twoway);
		//FPAnswerPtr twowayA = AWriter(twoway);
		//AReader(twowayA);
		//string raw = twoway->raw();
	}

	struct timeval end;
	gettimeofday(&end, NULL);

	int64_t dur = ((end.tv_sec - start.tv_sec)*1000 + (end.tv_usec - start.tv_usec)/1000);
	cout << test_count<<" finished, FPQWrite: speed="<< test_count*1000/dur << "/s"<<endl;

	FPQuestPtr twoway = QWriter(false);
	gettimeofday(&start, NULL);

	for(size_t i = 0; i < test_count; ++i){
		QReader(twoway);
		//FPAnswerPtr twowayA = AWriter(twoway);
		//AReader(twowayA);
		//string raw = twoway->raw();
	}

	gettimeofday(&end, NULL);

	dur = ((end.tv_sec - start.tv_sec)*1000 + (end.tv_usec - start.tv_usec)/1000);
	cout << test_count<<" finished, FPQReader: speed="<< test_count*1000/dur << "/s"<<endl;

	twoway = QWriter(false);
	string* rawData = twoway->raw();
	gettimeofday(&start, NULL);

	for(size_t i = 0; i < test_count; ++i){
		FPQuestPtr twowayC2(new FPQuest(rawData->data(),rawData->size()));
		//QReader(twoway);
		//FPAnswerPtr twowayA = AWriter(twoway);
		//AReader(twowayA);
		//string raw = twoway->raw();
	}
	delete rawData;

	gettimeofday(&end, NULL);

	dur = ((end.tv_sec - start.tv_sec)*1000 + (end.tv_usec - start.tv_usec)/1000);
	cout << test_count<<" finished, FPQWrite RAW Data: speed="<< test_count*1000/dur << "/s"<<endl;


	gettimeofday(&start, NULL);

	for(size_t i = 0; i < test_count; ++i){
		FPAnswerPtr twowayA = AWriter(twoway);
		//AReader(twowayA);
		//string raw = twoway->raw();
	}

	gettimeofday(&end, NULL);

	dur = ((end.tv_sec - start.tv_sec)*1000 + (end.tv_usec - start.tv_usec)/1000);
	cout << test_count<<" finished, FPAWrite: speed="<< test_count*1000/dur << "/s"<<endl;

	FPAnswerPtr twowayA = AWriter(twoway);
	gettimeofday(&start, NULL);

	for(size_t i = 0; i < test_count; ++i){
		AReader(twowayA);
		//string raw = twoway->raw();
	}

	gettimeofday(&end, NULL);

	dur = ((end.tv_sec - start.tv_sec)*1000 + (end.tv_usec - start.tv_usec)/1000);
	cout << test_count<<" finished, FPAReader: speed="<< test_count*1000/dur << "/s"<<endl;

	twowayA = AWriter(twoway);
	string* rawDataA = twowayA->raw();
	gettimeofday(&start, NULL);

	for(size_t i = 0; i < test_count; ++i){
		FPAnswerPtr twowayAC2(new FPAnswer(*rawDataA));
		//string raw = twoway->raw();
	}
	delete rawDataA;

	gettimeofday(&end, NULL);

	dur = ((end.tv_sec - start.tv_sec)*1000 + (end.tv_usec - start.tv_usec)/1000);
	cout << test_count<<" finished, FPAWrite RAW Data: speed="<< test_count*1000/dur << "/s"<<endl;


	//convert json to msgpack
	string json = "{\"answer\":\"one\", \"int\":2, \"double\":3.3, \"boolean\":true, \"ARRAY\":[[\"tuple1\", 3.1, 14, false], [\"tuple2\", 5.7, 9, true]], \"MAP\":{\"m1\":\"first_map\", \"m2\":true, \"m3\":5, \"m4\":5.7, \"m5\":\"中文3\"}}";
	gettimeofday(&start, NULL);

	for(size_t i = 0; i < test_count; ++i){
		JSONConvert::Json2Msgpack(json);
	}

	gettimeofday(&end, NULL);

	dur = ((end.tv_sec - start.tv_sec)*1000 + (end.tv_usec - start.tv_usec)/1000);
	cout << test_count<<" finished, JSON2MsgPack: speed="<< test_count*1000/dur << "/s"<<endl;


	return 0;
}

