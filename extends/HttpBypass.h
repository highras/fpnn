#ifndef FPNN_HTTP_BYPASS_H
#define FPNN_HTTP_BYPASS_H

/*
	配置文件约束

	1. 旁路信息

	# split by ";, "
	HTTPBypass.methods.list =
	# ? 为 methods 里表中对应的 method
	HTTPBypass.method.?.url =
	# timeout 默认 120 秒
	HTTPBypass.Task.timeoutSeconds =

	例 1:
	HTTPBypass.methods.list = deomHethod
	HTTPBypass.method.deomHethod.url = http://demo.com/s/deomHethodInterface

	例 2:
	HTTPBypass.methods.list = liveStart, liveFinish, watchLive
	HTTPBypass.method.liveStart.url = http://demo.com/s/liveStart
	HTTPBypass.method.liveFinish.url = http://demo.com/s/liveFinish
	HTTPBypass.method.watchLive.url = http://demo.com/s/watchLive

	2. MulitpleURLEngine 部分

	如果服务没有自己的 MultipleURLEngine， 或者希望 HTTPBypass 使用独立的 MultipleURLEngine，那需要增加以下条目：

	HTTPBypass.URLEngine.connsInPerThread = 
	HTTPBypass.URLEngine.ThreadPool.initCount = 
	HTTPBypass.URLEngine.ThreadPool.perfectCount = 
	HTTPBypass.URLEngine.ThreadPool.maxCount = 
	HTTPBypass.URLEngine.maxConcurrentTaskCount =
	HTTPBypass.URLEngine.ThreadPool.latencySeconds =
*/

#include <list>
#include <memory>
#include <vector>
#include <unordered_map>
#include "Setting.h"
#include "StringUtil.h"
#include "FPWriter.h"
#include "MultipleURLEngine.h"

using namespace fpnn;

class HttpBypass
{
	bool _timeout;
	bool _holdEngine;
	std::shared_ptr<MultipleURLEngine> _urlEngine;
	std::unordered_map<std::string, std::string> _urlMap;
	
	void init()
	{
		_timeout = Setting::getInt("HTTPBypass.Task.timeoutSeconds", 120);
	}
	void buildUrlsMap()
	{
		std::string methodsNames = Setting::getString("HTTPBypass.methods.list");
		std::vector<std::string> methodsList;
		StringUtil::split(methodsNames, ";, ", methodsList);

		for (auto& method: methodsList)
		{
			std::string name("HTTPBypass.method.");
			name.append(method).append(".url");

			std::string url = Setting::getString(name);
			if (url.empty() || strncasecmp(url.c_str(), "http://", 7) != 0)
			{
				LOG_FATAL("Cannot found config '%s' for method: %s", name.c_str(), method.c_str());
			}
			else
				_urlMap[method] = url;
		}
	}

	void initURLEngine()
	{
		int nConnsInPerThread = Setting::getInt("HTTPBypass.URLEngine.connsInPerThread", 200);
		int initThreadCount = Setting::getInt("HTTPBypass.URLEngine.ThreadPool.initCount", 1);
		int perfectThreadCount = Setting::getInt("HTTPBypass.URLEngine.ThreadPool.perfectCount", 10);
		int maxThreadCount = Setting::getInt("HTTPBypass.URLEngine.ThreadPool.maxCount", 100);
		int maxConcurrentCount = Setting::getInt("HTTPBypass.URLEngine.maxConcurrentTaskCount", 25000);
		int tempThreadLatencySeconds = Setting::getInt("HTTPBypass.URLEngine.ThreadPool.latencySeconds", 120);
		
		_urlEngine.reset(new MultipleURLEngine(nConnsInPerThread, initThreadCount, perfectThreadCount,
			maxThreadCount, maxConcurrentCount, tempThreadLatencySeconds));
	}

public:
	HttpBypass(std::shared_ptr<MultipleURLEngine> engine = nullptr): _urlEngine(engine)
	{
		init();

		_holdEngine = (engine == nullptr);
		if (_holdEngine)
		{
			MultipleURLEngine::init();
			initURLEngine();
		}
		buildUrlsMap();
	}

	~HttpBypass()
	{
		if (_holdEngine)
			MultipleURLEngine::cleanup();
	}

	void bypass(const FPQuestPtr quest)
	{
		const std::string& method = quest->method();
		auto it = _urlMap.find(method);
		if (it == _urlMap.end())
		{
			LOG_ERROR("Cannot bypass metod %s: not be configed. Quest: %s", method.c_str(), quest->json().c_str());
			return;
		}

		CURL *curl = curl_easy_init();

		std::string url = it->second;
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
		struct curl_slist *chunk = NULL;
		chunk = curl_slist_append(chunk, "Accept:");
		std::string s = "Content-Type: application/json;charset=utf-8";
		chunk = curl_slist_append(chunk, s.c_str());
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);


		bool status = _urlEngine->visit(curl, [quest, chunk] (MultipleURLEngine::Result &result) {
			curl_slist_free_all(chunk);
			if (result.visitState != MultipleURLEngine::VISIT_OK)
				LOG_ERROR("Bypass metod %s excepted. Quest: %s", quest->method().c_str(), quest->json().c_str());
		}, _timeout, true, quest->json());
		if (!status){
			curl_easy_cleanup(curl);
			curl_slist_free_all(chunk);
			LOG_ERROR("Bypass metod %s failed. Quest: %s", method.c_str(), quest->json().c_str());
		}
	}
};
typedef std::shared_ptr<HttpBypass> HttpBypassPtr;

#endif
