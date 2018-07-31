#ifndef FPNN_TCP_Server_Master_Processor_H
#define FPNN_TCP_Server_Master_Processor_H

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
	class TCPServerMasterProcessor: public ParamTemplateThreadPool<RequestPackage *>::IProcessor
	{
		typedef FPAnswerPtr (TCPServerMasterProcessor::* MethodFunc)(const FPReaderPtr, const FPQuestPtr, const ConnectionInfo&);

	private:
		HashMap<std::string, MethodFunc>* _methodMap;
		IQuestProcessorPtr _questProcessor;
		ConnectionReclaimerPtr _reclaimer;
		TCPEpollServer* _server;

		void prepare();
		FPAnswerPtr status(const FPReaderPtr, const FPQuestPtr, const ConnectionInfo&);
		FPAnswerPtr infos(const FPReaderPtr, const FPQuestPtr, const ConnectionInfo&);
		FPAnswerPtr tune(const FPReaderPtr, const FPQuestPtr, const ConnectionInfo&);

		void dealQuest(RequestPackage * requestPackage);

	public:
		TCPServerMasterProcessor(): _methodMap(0), _server(0)
		{
			_reclaimer = ConnectionReclaimer::instance();
			prepare();
		}
		virtual ~TCPServerMasterProcessor()
		{
			if (_methodMap)
				delete _methodMap;
		}
		virtual void run(RequestPackage * requestPackage);
		inline void setServer(TCPEpollServer* server) { _server = server; }
		inline void setQuestProcessor(IQuestProcessorPtr processor) { _questProcessor = processor; }
		inline bool isMasterMethod(const std::string& method) { return _methodMap->find(method); }
		inline bool checkQuestProcessor() { return (_questProcessor != nullptr); }
		inline IQuestProcessorPtr getQuestProcessor() { return _questProcessor; }
	};
}

#endif
