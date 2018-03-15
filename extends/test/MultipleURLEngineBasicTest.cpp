#include <iostream>
#include "MultipleURLEngine.h"

using namespace fpnn;
using namespace std;

int main()
{

	MultipleURLEngine::init();

	MultipleURLEngine engine;
	MultipleURLEngine::Result result;
	engine.visit("http://www.ioccc.org", result);
		
	if (result.visitState == MultipleURLEngine::VISIT_OK)
		cout<<"Visit Completed. Response code: "<<result.responseCode<<endl;
	else if (result.visitState == MultipleURLEngine::VISIT_EXPIRED)
		cout<<"Visit expried!"<<endl;
	else if (result.visitState == MultipleURLEngine::VISIT_TERMINATED)
		cout<<"Visit terminated!"<<endl;
	else if (result.visitState == MultipleURLEngine::VISIT_NOT_EXECUTED)
		cout<<"Visit not exectued!"<<endl;
	else
		cout<<"Visit Errored. error: "<<result.errorInfo<<endl;

	cout<<"Curl code "<<result.curlCode<<endl;

	MultipleURLEngine::cleanup();
	return 0;
}
