#ifndef FPNN_Server_Interface_H
#define FPNN_Server_Interface_H

#include <string>

namespace fpnn
{
	class ServerInterface
	{
	public:
		virtual ~ServerInterface() {}

		virtual bool startup() = 0;
		virtual void run() = 0;
		virtual void stop() = 0;

		virtual void setIP(const std::string& ip) = 0;
		virtual void setPort(unsigned short port) = 0;
		virtual void setBacklog(int backlog) = 0;

		virtual unsigned short port() const = 0;
		virtual std::string ip() const = 0;	

		//virtual bool config(const char* sectionHeader) = 0;
	};
	
}

#endif
