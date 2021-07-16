#ifndef FPNN_UDP_Server_Master_Processor_H
#define FPNN_UDP_Server_Master_Processor_H

#include <mutex>
#include <memory>
#include "HashMap.h"
#include "ParamTemplateThreadPool.h"
#include "IQuestProcessor.h"
#include "FPReader.h"

namespace fpnn
{
	enum class UDPSessionEventType
	{
		Connected,
		Quest,
		Closed,
		Error
	};

	struct UDPRequestPackage
	{
		UDPSessionEventType _type;
		ConnectionInfoPtr _connectionInfo;
		UDPServerConnection* _connection;		//-- Only for non-Quest case.
		FPQuestPtr _quest;

		UDPRequestPackage(ConnectionInfoPtr connInfo, FPQuestPtr quest):
			_type(UDPSessionEventType::Quest), _connectionInfo(connInfo), _connection(NULL), _quest(quest) {}
		UDPRequestPackage(UDPServerConnection* conn, UDPSessionEventType type):
			_type(type), _connectionInfo(conn->_connectionInfo), _connection(conn) {}
	};

	class UDPEpollServer;
	class UDPServerMasterProcessor: public ParamTemplateThreadPool<UDPRequestPackage *>::IProcessor
	{
		typedef FPAnswerPtr (UDPServerMasterProcessor::* MethodFunc)(const FPReaderPtr, const FPQuestPtr, const ConnectionInfo&);

	private:
		HashMap<std::string, MethodFunc>* _methodMap;
		std::atomic<int> _validARQConnections;
		IQuestProcessorPtr _questProcessor;
		UDPEpollServer* _server;

		void prepare();
		FPAnswerPtr status(const FPReaderPtr, const FPQuestPtr, const ConnectionInfo&);
		FPAnswerPtr infos(const FPReaderPtr, const FPQuestPtr, const ConnectionInfo&);
		FPAnswerPtr tune(const FPReaderPtr, const FPQuestPtr, const ConnectionInfo&);

		void dealQuest(UDPRequestPackage * requestPackage);

	public:
		UDPServerMasterProcessor(): _methodMap(0), _validARQConnections(0), _server(0)
		{
			prepare();
		}
		virtual ~UDPServerMasterProcessor()
		{
			if (_methodMap)
				delete _methodMap;
		}
		virtual void run(UDPRequestPackage * requestPackage);
		inline void setServer(UDPEpollServer* server) { _server = server; }
		inline void setQuestProcessor(IQuestProcessorPtr processor) { _questProcessor = processor; }
		inline bool isMasterMethod(const std::string& method) { return _methodMap->find(method); }
		inline bool checkQuestProcessor() { return (_questProcessor != nullptr); }
		inline IQuestProcessorPtr getQuestProcessor() { return _questProcessor; }
		inline int validARQConnectionCount() { return _validARQConnections; }
	};
}

#endif