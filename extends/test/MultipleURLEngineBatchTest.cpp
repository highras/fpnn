#include <iostream>
#include "MultipleURLEngine.h"

using namespace fpnn;
using namespace std;

std::atomic<int> taskCount(0);

class BatchCallback: public MultipleURLEngine::ResultCallback
{
	std::string _target;
public:
	BatchCallback(const std::string &target): MultipleURLEngine::ResultCallback(), _target(target) { taskCount++; }
	~BatchCallback() { taskCount--; }

	virtual void onCompleted(MultipleURLEngine::Result &result)
	{
		cout<<"visit "<<_target<<" completed. responseCode: "<<result.responseCode<<endl;
	}
	virtual void onExpired(MultipleURLEngine::CURLPtr curl_unique_ptr)
	{
		cout<<"visit "<<_target<<" expired."<<endl;
	}
	virtual void onTerminated(MultipleURLEngine::CURLPtr curl_unique_ptr)
	{
		cout<<"visit "<<_target<<" terminated."<<endl;
	}
	virtual void onException(MultipleURLEngine::CURLPtr curl_unique_ptr, enum MultipleURLEngine::VisitStateCode errorCode, const char *errorInfo)
	{
		cout<<"visit "<<_target<<" errored. state: "<<errorCode<<", info: "<<errorInfo<<endl;
	}
};

int main()
{

	MultipleURLEngine::init();

	MultipleURLEngine engine;
	engine.addToBatch("http://news.sina.com.cn", std::make_shared<BatchCallback>("news.sina"), 120);
	engine.addToBatch("http://baidu.com", std::make_shared<BatchCallback>("baidu"), 120);
	engine.addToBatch("http://taobao.com", std::make_shared<BatchCallback>("Taobao"), 120);
	engine.addToBatch("http://www.ioccc.org", std::make_shared<BatchCallback>("IOCCC"), 120);
	engine.addToBatch("http://www.amazon.cn/", std::make_shared<BatchCallback>("Amazon China"), 120);
	engine.addToBatch("http://z.cn", std::make_shared<BatchCallback>("z.cn"), 120);
	engine.commitBatch();
	
	while (taskCount)
		sleep(1);

	MultipleURLEngine::cleanup();
	return 0;
}
