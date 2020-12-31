#include <memory>
#include <errno.h>
#include <openssl/crypto.h>
#include <openssl/err.h>
#include "FPLog.h"
#include "Setting.h"
#include "IQuestProcessor.h"
#include "OpenSSLModule.h"

//-- G++ 8: for ignored unused-value warning triggered by CRYPTO_THREADID_set_callback()
#pragma GCC diagnostic ignored "-Wunused-value"

//-- G++ 8 & 9: for ignored unused-function warning triggered by OpenSSL required
//--            CRYPTO_set_locking_callback(openSSLLockerCallback) & CRYPTO_THREADID_set_callback(fetchOpenSSLThreadID)
#pragma GCC diagnostic ignored "-Wunused-function"

using namespace fpnn;

//------------------[ Local Definition & variables ]---------------------------//
typedef std::shared_ptr<OpenSSLModule> OpenSSLModulePtr;

static std::mutex gc_OpenSSL_mutex;
static bool gc_OpenSS_Client_inited(false);
static bool gc_OpenSS_Server_inited(false);
static OpenSSLModulePtr gc_OpenSSLModule(nullptr);

//------------------[ OpenSSL Module ]---------------------------//
OpenSSLModule::OpenSSLModule(): _context(0), _mutexArray(0)
{
	SSL_load_error_strings();
	SSL_library_init();
}

OpenSSLModule::~OpenSSLModule()
{
	if (_mutexArray)
	{
		CRYPTO_set_locking_callback(NULL);
		delete [] _mutexArray;

		SSL_CTX_free(_context);
		ERR_free_strings();
	}
}

bool OpenSSLModule::init()
{
	if (!_context)
	{
		_context = SSL_CTX_new(SSLv23_method());
		if (_context == NULL)
		{
			logLastErrors();
			return false;
		}

		initMutexArray();
	}

	return true;
}

bool OpenSSLModule::init(const std::string& certificate, const std::string& privateKey)
{
	if (init() == false)
		return false;

	if (SSL_CTX_use_certificate_chain_file(_context, certificate.c_str()) != 1)
	//if (SSL_CTX_use_certificate_file(_context, certificate.c_str(), SSL_FILETYPE_PEM) != 1)
	{
		logLastErrors();
		return false;
	}

	if (SSL_CTX_use_PrivateKey_file(_context, privateKey.c_str(), SSL_FILETYPE_PEM) != 1)
	{
		logLastErrors();
		return false;
	}

	if (SSL_CTX_check_private_key(_context) != 1) 
	{
		logLastErrors();
		return false;
	}

	return true;
}

#if OPENSSL_VERSION_NUMBER >= OPENSSL_VERSION_1_0_0

static void fetchOpenSSLThreadID(CRYPTO_THREADID *id)
{
	CRYPTO_THREADID_set_pointer(id, &errno);
}

#else

static unsigned long fetchOpenSSLThreadID(void)
{
	return (unsigned long)(&errno);
}
#endif

static void openSSLLockerCallback(int mode, int idx, const char *file, int line)
{
	(void)file;
	(void)line;

	if (mode & CRYPTO_LOCK)
		gc_OpenSSLModule->lock(idx);
	else
		gc_OpenSSLModule->unlock(idx);
}

void OpenSSLModule::initMutexArray()
{
	_mutexArray = new std::mutex[CRYPTO_num_locks()];

#if OPENSSL_VERSION_NUMBER >= OPENSSL_VERSION_1_0_0
	CRYPTO_THREADID_set_callback(fetchOpenSSLThreadID);
#else
	CRYPTO_set_id_callback(fetchOpenSSLThreadID);
#endif

	CRYPTO_set_locking_callback(openSSLLockerCallback);
}

std::list<std::string> OpenSSLModule::getLastErrors()
{
	const int bufferLen = 1024;
	std::list<std::string> messages;
	char* buffer = (char*)malloc(bufferLen);
	while (true)
	{
		const char *file, *data;
		int line, flags;
		unsigned long errorCode = ERR_get_error_line_data(&file, &line, &data, &flags);
		if (errorCode == 0)
		{
			free(buffer);
			return messages;
		}

		std::string info("[OpenSSL Error]");
		if (file)
			info.append("[").append(file).append(":").append(std::to_string(line)).append("]");
		else
			info.append("[unknown file]");

		info.append(" Code: ").append(std::to_string(errorCode));
		if (data && (flags & ERR_TXT_STRING))
			info.append(" ex: ").append(data);

		ERR_error_string_n(errorCode, buffer, bufferLen);
		if (*buffer)
			info.append(" Detail: ").append(buffer);

		messages.push_back(info);
	}
}

void OpenSSLModule::logLastErrors()
{
	std::list<std::string> errors = getLastErrors();
	for (auto it = errors.begin(); it != errors.end(); it++)
		LOG_ERROR((*it).c_str());
}

bool OpenSSLModule::clientModuleInit()
{
	std::unique_lock<std::mutex> lck(gc_OpenSSL_mutex);
	if (gc_OpenSS_Client_inited)
		return true;

	if (!gc_OpenSSLModule)
		gc_OpenSSLModule.reset(new OpenSSLModule);

	gc_OpenSS_Client_inited = gc_OpenSSLModule->init();
	return gc_OpenSS_Client_inited;
}

bool OpenSSLModule::serverModuleInit()
{
	std::unique_lock<std::mutex> lck(gc_OpenSSL_mutex);
	
	if (gc_OpenSS_Server_inited)
		return true;

	if (!gc_OpenSSLModule)
		gc_OpenSSLModule.reset(new OpenSSLModule);

	std::string certificate = Setting::getString("FPNN.server.security.ssl.certificate");
	if (certificate.empty())
	{
		LOG_ERROR("Config item 'FPNN.server.security.ssl.certificate' is invalid.");
		return false;
	}
	std::string privateKey = Setting::getString("FPNN.server.security.ssl.privateKey");
	if (privateKey.empty())
	{
		LOG_ERROR("Config item 'FPNN.server.security.ssl.privateKey' is invalid.");
		return false;
	}

	gc_OpenSS_Server_inited = gc_OpenSSLModule->init(certificate, privateKey);
	return gc_OpenSS_Server_inited;
}

SSL* OpenSSLModule::newSession(int socket)
{
	SSL* ssl = SSL_new(gc_OpenSSLModule->_context);
	if (ssl)
	{
		if (SSL_set_fd(ssl, socket) != 1)
		{
			SSL_free(ssl);
			ssl = NULL;
		}
	}
	return ssl;
}

//------------------[ SSL Context ]---------------------------//

bool SSLContext::init(int socket, bool server)
{
	_ssl = OpenSSLModule::newSession(socket);
	if (_ssl)
	{
		if (server)
			SSL_set_accept_state(_ssl);
		else
			SSL_set_connect_state(_ssl);
		
		SSL_set_mode(_ssl, /* SSL_get_mode(_ssl) & */ SSL_MODE_ENABLE_PARTIAL_WRITE);
		return true;
	}
	else
		return false;
}

bool SSLContext::doHandshake(bool& needSend, ConnectionInfo* ci)
{
	int ret = SSL_do_handshake(_ssl);
	if (ret == 1)
	{
		_connected = true;
		needSend = false;
		return true;
	}

	int errorCode = SSL_get_error(_ssl, ret);
	if (errorCode == SSL_ERROR_WANT_WRITE)
	{
		needSend = true;
		return true;
	}
	else if (errorCode == SSL_ERROR_WANT_READ)
	{
		needSend = false;
		return true;
	}
	else if (errorCode == SSL_ERROR_SYSCALL)
	{
		if (errno == 0 || errno == ECONNRESET)
		{
			OpenSSLModule::logLastErrors();
			return false;
		}

		if (errno == EAGAIN || errno == EWOULDBLOCK)
		{
			needSend = true;
			return true;
		}

		LOG_ERROR("SSL do handshake error. SSL error code: SSL_ERROR_SYSCALL, errno: %d. %s", errno, ci->str().c_str());
		OpenSSLModule::logLastErrors();
		return false;
	}
	else if (errorCode == SSL_ERROR_SSL)
	{
		LOG_ERROR("SSL do handshake error. SSL error code: SSL_ERROR_SSL. %s", ci->str().c_str());
		OpenSSLModule::logLastErrors();
		return false;
	}
	else
	{
		LOG_ERROR("SSL do handshake error. SSL error code: %d, errno: %d. %s", errorCode, errno, ci->str().c_str());
		OpenSSLModule::logLastErrors();
		return false;
	}
}

void SSLContext::close()
{
	if (_ssl)
	{
		SSL_shutdown(_ssl);
		SSL_free(_ssl);
		_ssl = NULL;
	}
}
