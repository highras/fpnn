#include <iostream>
#include "MultipleURLEngine.h"

using namespace fpnn;
using namespace std;

struct UserData
{
	std::string effective_url;
	long responseCode;
	long connectCode;
	long filetime;
	double totalTime;
	double nameLookupTime;
	double connectTime;
	double redirectTime;
	long redirectCount;
	std::string redirectUrl;
	double downloadSize;
	double downloadSpeed;
	long headSize;
	long requestSize;
	double downContentLength;
	double uploadContentLength;
	std::string contentType;
	long connectNum;
	std::string targetIp;
	std::string localIp;
	long localPort;

	void print()
	{
		cout<<endl<<"----------------"<<endl;
		cout<<"effective_url: "<<effective_url<<endl;

		cout<<"responseCode: "<<responseCode<<endl;
		cout<<"connectCode: "<<connectCode<<endl;
		cout<<"filetime: "<<filetime<<endl;
		cout<<"totalTime: "<<totalTime<<endl;
		cout<<"nameLookupTime: "<<nameLookupTime<<endl;

		cout<<"connectTime: "<<connectTime<<endl;
		cout<<"redirectTime: "<<redirectTime<<endl;
		cout<<"redirectCount: "<<redirectCount<<endl;
		cout<<"redirectUrl: "<<redirectUrl<<endl;
		cout<<"downloadSize: "<<downloadSize<<endl;

		cout<<"downloadSpeed: "<<downloadSpeed<<endl;
		cout<<"headSize: "<<headSize<<endl;
		cout<<"requestSize: "<<requestSize<<endl;
		cout<<"downContentLength: "<<downContentLength<<endl;
		cout<<"uploadContentLength: "<<uploadContentLength<<endl;

		cout<<"contentType: "<<contentType<<endl;
		cout<<"connectNum: "<<connectNum<<endl;
		cout<<"targetIp: "<<targetIp<<endl;
		cout<<"localIp: "<<localIp<<endl;
		cout<<"localPort: "<<localPort<<endl;

		cout<<"----------------"<<endl;
	}
};

void getInfo(CURL *handle, UserData *data)
{
	char *strData;
	curl_easy_getinfo(handle, CURLINFO_EFFECTIVE_URL, &strData);
	if (strData)
		data->effective_url = strData;

	curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, &(data->responseCode));
	curl_easy_getinfo(handle, CURLINFO_HTTP_CONNECTCODE, &(data->connectCode));
	curl_easy_getinfo(handle, CURLINFO_FILETIME, &(data->filetime));
	curl_easy_getinfo(handle, CURLINFO_TOTAL_TIME, &(data->totalTime));
	curl_easy_getinfo(handle, CURLINFO_NAMELOOKUP_TIME, &(data->nameLookupTime));
	curl_easy_getinfo(handle, CURLINFO_CONNECT_TIME, &(data->connectTime));
	curl_easy_getinfo(handle, CURLINFO_REDIRECT_TIME, &(data->redirectTime));
	curl_easy_getinfo(handle, CURLINFO_REDIRECT_COUNT, &(data->redirectCount));
	curl_easy_getinfo(handle, CURLINFO_REDIRECT_URL, &strData);
	if (strData)
		data->redirectUrl = strData;

	curl_easy_getinfo(handle, CURLINFO_SIZE_DOWNLOAD, &(data->downloadSize));
	curl_easy_getinfo(handle, CURLINFO_SPEED_DOWNLOAD, &(data->downloadSpeed));
	curl_easy_getinfo(handle, CURLINFO_HEADER_SIZE, &(data->headSize));
	curl_easy_getinfo(handle, CURLINFO_REQUEST_SIZE, &(data->requestSize));
	curl_easy_getinfo(handle, CURLINFO_CONTENT_LENGTH_DOWNLOAD,  &(data->downContentLength));
	curl_easy_getinfo(handle, CURLINFO_CONTENT_LENGTH_UPLOAD,  &(data->uploadContentLength));


	curl_easy_getinfo(handle, CURLINFO_CONTENT_TYPE, &strData);
	if (strData)
		data->contentType = strData;

	curl_easy_getinfo(handle, CURLINFO_NUM_CONNECTS, &(data->connectNum));
	curl_easy_getinfo(handle, CURLINFO_PRIMARY_IP, &strData);
	if (strData)
		data->targetIp = strData;

	curl_easy_getinfo(handle, CURLINFO_LOCAL_IP, &strData);
	if (strData)
		data->localIp = strData;

	curl_easy_getinfo(handle, CURLINFO_LOCAL_PORT, &(data->localPort));
}

int main(int argc, const char **argv)
{
	if (argc < 2)
	{
		cout<<"Usage: "<<argv[0]<<" url"<<endl;
		return 0;
	}

	MultipleURLEngine::init();

	UserData info;

	MultipleURLEngine engine;
	MultipleURLEngine::Result result;
	engine.visit(argv[1], result, 120);

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

	getInfo(result.curlHandle.get(), &info);
	info.print();

	MultipleURLEngine::cleanup();
	return 0;
}
