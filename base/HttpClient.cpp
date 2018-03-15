#include "HttpClient.h"
#include "hex.h"
#include "FPLog.h"
#include <string.h>
#include <curl/curl.h>
#include <sstream>

using namespace fpnn;

static size_t OnWriteData(void* buffer, size_t size, size_t nmemb, void* lpVoid)  
{  
    std::string* str = dynamic_cast<std::string*>((std::string *)lpVoid);  
    if( NULL == str || NULL == buffer )  
    {  
        return -1;  
    }  
  
    char* pData = (char*)buffer;  
    str->append(pData, size * nmemb);  
    return nmemb;  
}  

int HttpClient::Post(const std::string& url, const std::string& data, std::string& resp, int timeout){
    CURL* curl = curl_easy_init();  
    if(NULL == curl)  
    {  
        return -1;  
    }  
	long retcode = 0;
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, timeout);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout);  
	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
    curl_easy_setopt(curl, CURLOPT_POST, 1);  
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());  
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());  
    curl_easy_setopt(curl, CURLOPT_READFUNCTION, NULL);  
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, OnWriteData);  
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&resp);  
    CURLcode res = curl_easy_perform(curl);  
	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE , &retcode);
    curl_easy_cleanup(curl);  
	if(CURLE_OK != res){
		return -2;
	}
	if(retcode != 200){
		return retcode;
	}
    return 0;  
}

int HttpClient::Get(const std::string& url, std::string& resp, int timeout){
    CURL* curl = curl_easy_init();  
    if(NULL == curl)  
    {  
        return -1;  
    }  
	long retcode = 0;
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, timeout);  
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, timeout);  
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);  
	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());  
    curl_easy_setopt(curl, CURLOPT_READFUNCTION, NULL);  
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, OnWriteData);  
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&resp);  
    CURLcode res = curl_easy_perform(curl);  
	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE , &retcode);
    curl_easy_cleanup(curl);  
	if(CURLE_OK != res){
		return -2;
	}
	if(retcode != 200){
		return retcode;
	}
    return 0;  
}


std::string HttpClient::uriEncode(const std::string& uri){
	std::string ret;
	CURL * curl = nullptr;
	char * esc = nullptr;
	curl = curl_easy_init();
	esc = curl_easy_escape(curl, uri.c_str(), 0);
	if(esc) ret = esc;
	curl_free((void*)esc);
	curl_easy_cleanup(curl);
	return ret;
}

std::string HttpClient::uriDecode(const std::string& uri){
	std::string ret;
	CURL * curl = nullptr;
	char * uesc = nullptr;
	curl = curl_easy_init();
	uesc = curl_easy_unescape(curl, uri.c_str(), 0, nullptr);
	if(uesc) ret = uesc;
	curl_free((void*)uesc);
	curl_easy_cleanup(curl);
	return ret;
}

std::string HttpClient::uriDecode2(const std::string& uri){
    size_t len = uri.length();
    std::string ret = ""; 
    const char* curi = uri.c_str();
    for (size_t i = 0; i < len; ++i){
        int c = (unsigned char)curi[i];
        if (c == '%' && (i + 2 < len) && isxdigit(curi[i+1]) && isxdigit(curi[i+2])){
            char dst[1];
            unhexlify(dst, &curi[i+1], 2); 
            i += 2;
            ret.push_back(dst[0]);
        }   
        else ret.push_back(c);
    }   
    return ret;
}


#ifdef TEST_HTTP_CLIENT
//g++ -g -DTEST_HTTP_CLIENT HttpClient.cpp -std=c++11 -lcurl hex.c
#include <sstream>
#include <iostream>
using namespace std;
using namespace HttpClient;

int main(int argc, char **argv)
{
	string resp;
	string url = "http://localhost:9876/service/httpDemo?a=b";
	string data = "{\"key\":5,\"key2\":\"value\"}";
	HttpClient::Post(url, data, resp);
	cout<<resp<<endl;

	resp = "";
	url = "http://localhost:31023/service/translate";
	data = "{\"source\":\"en\",\"target\":\"de\",\"q\":\"Do you understand?\"}";
	HttpClient::Post(url, data, resp);
	cout<<resp<<endl;

	resp = "";
	url = "http://169.254.169.254/latest/meta-data/placement/availability-zone/";
	HttpClient::Get(url, resp);
	cout<<resp<<endl;

	resp = "";
	url = "Do you understand?";
	resp = HttpClient::uriEncode(url);
	cout<<resp<<endl;

	cout<<HttpClient::uriDecode(resp)<<endl;
	cout<<HttpClient::uriDecode2(resp)<<endl;
	return 0;
}

#endif
