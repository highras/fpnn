//-- g++ --std=c++11 -I. -I../../base/ -lstdc++ -o RapidJson.vs.FPJson ../msec.c ../FPJson.cpp ../FPJsonParser.cpp RapidJson.vs.FPJson.cpp ../../base/FpnnError.o ../../base/StringUtil.o -lpthread

#include <iostream>
#include "FPJson.h"
#include "msec.h"

#include "../../proto/rapidjson/document.h"
#include "../../proto/rapidjson/writer.h"
#include "../../proto/rapidjson/stringbuffer.h"

using namespace std;
using namespace fpnn;

const std::string jsonStr = R"(
{"sw":375,"sh":812,"manu":"iPhone","model":"iPhone X (GSM+CDMA)<iPhone10,3>","os":"iOS 12.0","osv":"iOS 12.0","nw":"4g","lang":"zh_TW","from":"wechat","appv":"201812031215","v":"1.0.11","first":false,"ev":"open","eid":1544095090856701,"pid":41000011,"sid":1544095090856700,"uid":"o3Bv15dZFPlTXF7ZN5bUtzta9iXc","rid":"1543891212234-480A-9237-834A9F9FED25","ts":1544095082,"sts":1544095091}
)";

const std::string jsonStr2 = R"(
{ "type": "wx_running_error", "message": "gameThirdScriptError\nURI error;at api onShow callback function\ndecodeURIComponent@[native code]\nm@https://usr/game.js:28710:11546\nhttps://usr/game.js:28709:7528\nt@https://lib/WAGameSubContext.js:1:114320\nhttps://lib/WAGame.js:1:194916\nemit@https://lib/WAGame.js:1:217263\nemit@https://lib/WAGame.js:1:288924\nfc@https://lib/WAGame.js:1:362139\nhttps://lib/WAGame.js:1:362741\nhttps://lib/WAGame.js:1:253311\nS@https://lib/WAGame.js:1:1740\nglobal code","ev": "error","eid": 154587602905037,"pid": 41000011,"sid": 15458756975261,"uid": "o3Bv15Yz-DhmmObLVhXX5Ir99pjk","rid": "1545273750883-495D-87F2-8F7F5D74AAED","ts": 1545875763,"sts": 1545876037}
)";

const int count = 10 * 10000;

void test(const std::string& str)
{
	int value = 0;
	
	int64_t begin = slack_mono_msec();	
	for (int i = 0; i < count; i++)
	{
		JsonPtr json = Json::parse(str.c_str());
		if (json->exist("message"))
			value += 1;
		else
			value -= 1;
	}
	int64_t cost = slack_mono_msec() - begin;

	cout<<" cost: "<<cost<<" value: "<< value<<endl;
}

void test2(const std::string& str)
{
	int value = 0;
	int64_t begin = slack_mono_msec();
        for (int i = 0; i < count; i++)
	{
		rapidjson::Document document;
		document.Parse(str.c_str());

		if (document.HasMember("message"))
		        value += 1;
                else
                        value -= 1;
	}
	int64_t cost = slack_mono_msec() - begin;

        cout<<" cost: "<<cost<<" value: "<< value<<endl;
}

int main()
{
	test(jsonStr);
	test(jsonStr2);
	test2(jsonStr);
	test2(jsonStr2);
	return 0;
}

