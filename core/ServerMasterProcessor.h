#ifndef FPNN_Server_Master_Processor_H
#define FPNN_Server_Master_Processor_H

#include <map>
#include <mutex>
#include <memory>
#include "HashMap.h"
#include "ParamTemplateThreadPool.h"
#include "IQuestProcessor.h"
#include "ConnectionReclaimer.h"
#include "FPReader.h"

namespace fpnn
{
	class TCPEpollServer;
	class ServerMasterProcessor: public ParamTemplateThreadPool<RequestPackage *>::IProcessor
	{
		typedef FPAnswerPtr (ServerMasterProcessor::* MethodFunc)(const FPReaderPtr, const FPQuestPtr, const ConnectionInfo&);

	private:
		HashMap<std::string, MethodFunc>* _methodMap;
		TCPEpollServer* _server;
		IQuestProcessorPtr _questProcessor; 
		ConnectionReclaimerPtr _reclaimer;

		//-- common infos cache
		std::mutex _mutex;
		std::string _infosHeader;
		std::shared_ptr<std::string> _serverBaseInfos;
		std::shared_ptr<std::string> _clientBaseInfos;

		void prepare();
		FPAnswerPtr status(const FPReaderPtr, const FPQuestPtr, const ConnectionInfo&);
		FPAnswerPtr infos(const FPReaderPtr, const FPQuestPtr, const ConnectionInfo&);
		FPAnswerPtr tune(const FPReaderPtr, const FPQuestPtr, const ConnectionInfo&);

		void dealQuest(RequestPackage * requestPackage);

		void serverInfos(std::stringstream& ss, bool show_interface_stat);
		void clientInfos(std::stringstream& ss);
		std::string appInfos();

	public:
		ServerMasterProcessor(): _methodMap(0), _server(0)
		{
			_reclaimer = ConnectionReclaimer::instance();
			prepare();
		}
		virtual ~ServerMasterProcessor()
		{
			if (_methodMap)
				delete _methodMap;
		}
		virtual void run(RequestPackage * requestPackage);
		void setServer(TCPEpollServer* server);
		inline void setQuestProcessor(IQuestProcessorPtr processor) { _questProcessor = processor; }
		inline bool isMasterMethod(const std::string& method) { return _methodMap->find(method); }
		inline bool checkQuestProcessor() { return (_questProcessor != nullptr); }
		IQuestProcessorPtr getQuestProcessor() { return _questProcessor; }

		void logStatus();
	};
}

#endif
