#ifndef FPNN_Server_Interface_H
#define FPNN_Server_Interface_H

#include <string>
#include <memory>

namespace fpnn
{
	class IQuestProcessor;
	typedef std::shared_ptr<IQuestProcessor> IQuestProcessorPtr;

	class ServerInterface
	{
	public:
		virtual ~ServerInterface() {}

		virtual bool startup() = 0;
		virtual void run() = 0;
		virtual void stop() = 0;

		virtual void setIP(const std::string& ip) = 0;
		virtual void setIPv6(const std::string& ipv6) = 0;
		virtual void setPort(unsigned short port) = 0;
		virtual void setPort6(unsigned short port) = 0;
		virtual void setBacklog(int backlog) = 0;

		virtual unsigned short port() const = 0;
		virtual unsigned short port6() const = 0;
		virtual std::string ip() const = 0;
		virtual std::string ipv6() const = 0;

		virtual void setQuestProcessor(IQuestProcessorPtr questProcessor) = 0;
		virtual IQuestProcessorPtr getQuestProcessor() = 0;

		virtual std::string workerPoolStatus() = 0;
		virtual std::string answerCallbackPoolStatus() = 0;
	};
	
	typedef std::shared_ptr<ServerInterface> ServerPtr;
}

#endif
