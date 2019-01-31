#ifndef FPNN_OpenSSL_Module_h
#define FPNN_OpenSSL_Module_h

#include <mutex>
#include <list>
#include <string>
#include <openssl/ssl.h>

namespace fpnn
{
	class OpenSSLModule
	{
		SSL_CTX* _context;
		std::mutex* _mutexArray;

	private:
		OpenSSLModule();

		bool init();
		bool init(const std::string& certificate, const std::string& privateKey);
		void initMutexArray();

	public:
		~OpenSSLModule();

		void   lock(int idx) { _mutexArray[idx].lock(); }
		void unlock(int idx) { _mutexArray[idx].unlock(); }

	public:
		static bool clientModuleInit();
		static bool serverModuleInit();
		static SSL* newSession(int socket);

		static void logLastErrors();
		static std::list<std::string> getLastErrors();
	};

	class ConnectionInfo;

	enum class SSLNegotiate
	{
		Normal,
		//Write_Want_Write,
		Write_Want_Read,
		Read_Want_Write
		//Read_Want_Read
	};

	struct SSLContext
	{
		SSL *_ssl;
		volatile bool _connected;
		SSLNegotiate _negotiate;

		SSLContext(): _ssl(0), _connected(false), _negotiate(SSLNegotiate::Normal) {}
		bool init(int socket, bool server);
		bool doHandshake(bool& needSend, ConnectionInfo* ci);
		void close();
	};
}

#endif
