#include <exception>
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
//#include <netinet/tcp.h>  //-- for TCP_NODELAY
//#include <errno.h>
#include "ClientEngine.h"
#include "TCPClient.h"
#include "ConnectionReclaimer.h"
#include "FPWriter.h"
#include "AutoRelease.h"
#include "FileSystemUtil.h"
#include "PEM_DER_SAX.h"

using namespace fpnn;

TCPClient::TCPClient(const std::string& host, int port, bool autoReconnect):
	Client(host, port, autoReconnect), _AESKeyLen(16), _packageEncryptionMode(true),
	_sslEnabled(false), _ioChunkSize(256), _keepAliveParams(NULL)
{
	if (Config::Client::KeepAlive::defaultEnable)
		keepAlive();
}

bool TCPClient::enableEncryptorByDerData(const std::string &derData, bool packageMode, bool reinforce)
{
	EccKeyReader reader;

	X690SAX derSAX;
	if (derSAX.parse(derData, &reader) == false)
		return false;

	enableEncryptor(reader.curveName(), reader.rawPublicKey(), packageMode, reinforce);
	return true;
}
bool TCPClient::enableEncryptorByPemData(const std::string &PemData, bool packageMode, bool reinforce)
{
	EccKeyReader reader;

	PemSAX pemSAX;
	if (pemSAX.parse(PemData, &reader) == false)
		return false;

	enableEncryptor(reader.curveName(), reader.rawPublicKey(), packageMode, reinforce);
	return true;
}
bool TCPClient::enableEncryptorByDerFile(const char *derFilePath, bool packageMode, bool reinforce)
{
	std::string content;
	if (FileSystemUtil::readFileContent(derFilePath, content) == false)
		return false;
	
	return enableEncryptorByDerData(content, packageMode, reinforce);
}
bool TCPClient::enableEncryptorByPemFile(const char *pemFilePath, bool packageMode, bool reinforce)
{
	std::string content;
	if (FileSystemUtil::readFileContent(pemFilePath, content) == false)
		return false;

	return enableEncryptorByPemData(content, packageMode, reinforce);
}

bool TCPClient::enableSSL(bool enable)
{
	if (OpenSSLModule::clientModuleInit())
	{
		_sslEnabled = enable;
		return true;
	}
	else
		return false;
}

void TCPClient::keepAlive()
{
	std::unique_lock<std::mutex> lck(_mutex);

	if (!_keepAliveParams)
	{
		_keepAliveParams = new TCPClientKeepAliveParams;

		_keepAliveParams->pingTimeout = 0;
		_keepAliveParams->pingInterval = Config::Client::KeepAlive::pingInterval;
		_keepAliveParams->maxPingRetryCount = Config::Client::KeepAlive::maxPingRetryCount;
	}
}
void TCPClient::setKeepAlivePingTimeout(int seconds)
{
	keepAlive();
	_keepAliveParams->pingTimeout = seconds * 1000;
}
void TCPClient::setKeepAliveInterval(int seconds)
{
	keepAlive();
	_keepAliveParams->pingInterval = seconds * 1000;
}
void TCPClient::setKeepAliveMaxPingRetryCount(int count)
{
	keepAlive();
	_keepAliveParams->maxPingRetryCount = count;
}

class QuestTask: public ITaskThreadPool::ITask
{
	FPQuestPtr _quest;
	ConnectionInfoPtr _connectionInfo;
	TCPClientPtr _client;
	bool _fatal;

public:
	QuestTask(TCPClientPtr client, FPQuestPtr quest, ConnectionInfoPtr connectionInfo):
		_quest(quest), _connectionInfo(connectionInfo), _client(client), _fatal(false) {}

	virtual ~QuestTask()
	{
		if (_fatal)
			_client->close();
	}

	virtual void run()
	{
		try
		{
			_client->processQuest(_quest, _connectionInfo);
		}
		catch (const FpnnError& ex){
			LOG_ERROR("processQuest() error:(%d)%s. Connection will be closed by client. %s", ex.code(), ex.what(), _connectionInfo->str().c_str());
			_fatal = true;
		}
		catch (const std::exception& ex)
		{
			LOG_ERROR("processQuest() error: %s. Connection will be closed by client. %s", ex.what(), _connectionInfo->str().c_str());
			_fatal = true;
		}
		catch (...)
		{
			LOG_ERROR("Fatal error occurred in processQuest() function. Connection will be closed by client. %s", _connectionInfo->str().c_str());
			_fatal = true;
		}
	}
};

void TCPClient::dealQuest(FPQuestPtr quest, ConnectionInfoPtr connectionInfo)		//-- must done in thread pool or other thread.
{
	if (!_questProcessor)
	{
		LOG_ERROR("Recv a quest but without quest processor. %s", connectionInfo->str().c_str());
		return;
	}

	bool wakeup, exiting;
	std::shared_ptr<QuestTask> task(new QuestTask(shared_from_this(), quest, connectionInfo));
	if (_questProcessPool)
	{
		wakeup = _questProcessPool->wakeUp(task);
		if (!wakeup) exiting = _questProcessPool->exiting();
	}
	else
	{
		wakeup = ClientEngine::wakeUpQuestProcessThreadPool(task);
		if (!wakeup) exiting = ClientEngine::questProcessPoolExiting();
	}

	if (!wakeup)
	{
		if (exiting)
		{
			LOG_ERROR("wake up thread pool to process quest failed. Quest pool is exiting. %s", connectionInfo->str().c_str());
		}
		else
		{
			LOG_ERROR("wake up thread pool to process quest failed. Quest pool limitation is caught. Quest task havn't be executed. %s",
				connectionInfo->str().c_str());

			if (quest->isTwoWay())
			{
				try
				{
					FPAnswerPtr answer = FpnnErrorAnswer(quest, FPNN_EC_CORE_WORK_QUEUE_FULL, std::string("worker queue full, ") + connectionInfo->str().c_str());
					std::string *raw = answer->raw();
					_engine->sendData(connectionInfo->socket, connectionInfo->token, raw);
				}
				catch (const FpnnError& ex)
				{
					LOG_ERROR("Generate error answer for duplex client worker queue full failed. No answer returned, peer need to wait timeout. %s, exception:(%d)%s",
						connectionInfo->str().c_str(), ex.code(), ex.what());
				}
				catch (const std::exception& ex)
				{
					LOG_ERROR("Generate error answer for duplex client worker queue full failed. No answer returned, peer need to wait timeout. %s, exception: %s",
						connectionInfo->str().c_str(), ex.what());
				}
				catch (...)
				{
					LOG_ERROR("Generate error answer for duplex client worker queue full failed. No answer returned, peer need to wait timeout. %s",
						connectionInfo->str().c_str());
				}
			}
		}
		
	}
}

void TCPClient::configKeepAliveParams(const TCPClientKeepAliveParams* params)
{
	std::unique_lock<std::mutex> lck(_mutex);

	if (!_keepAliveParams)
		_keepAliveParams = new TCPClientKeepAliveParams;

	_keepAliveParams->config(params);	
}

bool TCPClient::configEncryptedConnection(TCPClientConnection* connection, std::string& publicKey)
{
	if (_eccCurve.empty())
		return true;

	ECCKeysMaker keysMaker;
	keysMaker.setPeerPublicKey(_serverPublicKey);
	if (keysMaker.setCurve(_eccCurve) == false)
		return false;

	publicKey = keysMaker.publicKey(true);
	if (publicKey.empty())
		return false;

	uint8_t key[32];
	uint8_t iv[16];
	if (keysMaker.calcKey(key, iv, _AESKeyLen) == false)
	{
		LOG_ERROR("Client's keys maker calcKey failed. Peer %s", connection->_connectionInfo->str().c_str());
		return false;
	}

	if (!connection->entryEncryptMode(key, _AESKeyLen, iv, !_packageEncryptionMode))
	{
		LOG_ERROR("Client connection entry encrypt mode failed. Peer %s", connection->_connectionInfo->str().c_str());
		return false;
	}
	connection->encryptAfterFirstPackageSent();
	return true;
}

ConnectionInfoPtr TCPClient::perpareConnection(int socket, std::string& publicKey)
{
	/*int flag = 1;
	if (-1 == setsockopt(socket, IPPROTO_TCP, TCP_NODELAY, (void*)&flag, sizeof(flag)))
	{
		::close(socket);
		LOG_ERROR("TCP-Nodelay: disable Nagle failed. Socket: %d", socket);
		return nullptr;
	}*/

	ConnectionInfoPtr newConnectionInfo;
	TCPClientConnection* connection = NULL;
	{
		std::unique_lock<std::mutex> lck(_mutex);
		newConnectionInfo.reset(new ConnectionInfo(socket, _connectionInfo->port, _connectionInfo->ip, _isIPv4, false));
		connection = new TCPClientConnection(shared_from_this(), _ioChunkSize, newConnectionInfo);

		if (_keepAliveParams)
		{
			if (_keepAliveParams->pingTimeout)
				connection->configKeepAlive(_keepAliveParams);
			else
			{
				_keepAliveParams->pingTimeout = _timeoutQuest ? _timeoutQuest : ClientEngine::getQuestTimeout() * 1000;
				connection->configKeepAlive(_keepAliveParams);
				_keepAliveParams->pingTimeout = 0;
			}
		}
	}

	if (_sslEnabled)
	{
		newConnectionInfo->_isSSL = true;
		if (!connection->prepareSSL(false))
		{
			LOG_ERROR("Error occurred when prepare SSL. %s", newConnectionInfo->str().c_str());
			OpenSSLModule::logLastErrors();

			delete connection;		//-- connection will close socket.
			return nullptr;
		}
	}

	if (configEncryptedConnection(connection, publicKey) == false)
	{
		delete connection;		//-- connection will close socket.
		return nullptr;
	}

	connected(connection);

	bool joined = ClientEngine::nakedInstance()->joinEpoll(connection);
	if (!joined)
	{
		LOG_ERROR("Join epoll failed after connected event. %s", connection->_connectionInfo->str().c_str());
		errorAndWillBeClosed(connection);

		return nullptr;
	}
	else
		return newConnectionInfo;
}

//-- cache for change sync connect to async connect
/*
bool TCPClient::perpareConnectIPv4Address()
{
	std::unique_lock<std::mutex> lck(_mutex);
	if (_connStatus != NoConnected)
		return true;

	struct sockaddr_in serverAddr;
	memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET; 
	serverAddr.sin_addr.s_addr = inet_addr(_connectionInfo->ip.c_str()); 
	serverAddr.sin_port = htons(_connectionInfo->port);

	if (serverAddr.sin_addr.s_addr == INADDR_NONE)
		return false;

	int socketfd = ::socket(AF_INET, SOCK_STREAM, 0);
	if (socketfd < 0)
		return false;

	TCPClientConnection* connection = NULL;
	ConnectionInfoPtr newConnectionInfo(new ConnectionInfo(socketfd, _connectionInfo->port, _connectionInfo->ip, _isIPv4, false));
	connection = new TCPClientConnection(shared_from_this(), _ioChunkSize, newConnectionInfo);
	
	bool joined = ClientEngine::nakedInstance()->joinEpoll(connection);
	if (!joined)
	{
		LOG_ERROR("Join epoll failed before connect to server. %s", newConnectionInfo->str().c_str());
		delete connection;
		return false;
	}

	int rev = ::connect(socketfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
	if (rev != 0 && errno == EINPROGRESS)
	{
		LOG_ERROR("Connect to server failed. %s", newConnectionInfo->str().c_str());
		connection = ClientEngine::nakedInstance()->takeConnection(socketfd);
		if (connection)		//-- if connection == NULL, connection is process by other thread.
		{
			ClientEngine::nakedInstance()->exitEpoll(connection);
			delete connection;
		}
		return false;
	}

	_connectionInfo = newConnectionInfo;
	if (rev == 0)
	{
		_connStatus = ConnStatus::Connected; //-- will be changed when add key exchange process.
		_connected = true;
		connected(connection);
	}
	else
		_connStatus = ConnStatus::Connecting;

	return true;
}*/

int TCPClient::connectIPv4Address(ConnectionInfoPtr currConnInfo)
{
	struct sockaddr_in serverAddr;
	memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = inet_addr(currConnInfo->ip.c_str()); 
	serverAddr.sin_port = htons(currConnInfo->port);

	if (serverAddr.sin_addr.s_addr == INADDR_NONE)
		return 0;

	int socketfd = ::socket(AF_INET, SOCK_STREAM, 0);
	if (socketfd < 0)
		return 0;

	if (::connect(socketfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) != 0)
	{
		::close(socketfd);
		return 0;
	}

	return socketfd;
}
int TCPClient::connectIPv6Address(ConnectionInfoPtr currConnInfo)
{
	struct sockaddr_in6 serverAddr;
	memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin6_family = AF_INET6;  
	serverAddr.sin6_port = htons(currConnInfo->port);

	if (inet_pton(AF_INET6, currConnInfo->ip.c_str(), &serverAddr.sin6_addr) != 1)
		return 0;

	int socketfd = ::socket(AF_INET6, SOCK_STREAM, 0);
	if (socketfd < 0)
		return 0;

	if (::connect(socketfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) != 0)
	{
		::close(socketfd);
		return 0;
	}

	return socketfd;
}

bool TCPClient::connect()
{
	if (_connected)
		return true;

	ConnectionInfoPtr currConnInfo;
	{
		std::unique_lock<std::mutex> lck(_mutex);
		while (_connStatus == ConnStatus::Connecting || _connStatus == ConnStatus::KeyExchanging)
			_condition.wait(lck);

		if (_connStatus == ConnStatus::Connected)
			return true;

		currConnInfo = _connectionInfo;

		_connected = false;
		_connStatus = ConnStatus::Connecting;
	}

	TCPClient* self = this;

	CannelableFinallyGuard errorGuard([self, currConnInfo](){
		std::unique_lock<std::mutex> lck(self->_mutex);
		if (currConnInfo.get() == self->_connectionInfo.get())
		{
			self->_connected = false;
			self->_connStatus = ConnStatus::NoConnected;
		}
		self->_condition.notify_all();
	});

	int socket = 0;
	if (_isIPv4)
		socket = connectIPv4Address(currConnInfo);
	else
		socket = connectIPv6Address(currConnInfo);

	if (socket == 0)
	{
		LOG_ERROR("TCP client connect remote server %s failed.", currConnInfo->str().c_str());
		return false;
	}

	std::string publicKey;
	ConnectionInfoPtr newConnInfo = perpareConnection(socket, publicKey);
	if (!newConnInfo)
		return false;

	//========= If encrypt connection ========//
	if (_eccCurve.size())
	{
		uint64_t token = newConnInfo->token;

		FPQWriter qw(3, "*key");
		qw.param("publicKey", publicKey);
		qw.param("streamMode", !_packageEncryptionMode);
		qw.param("bits", _AESKeyLen * 8); 
		FPQuestPtr quest = qw.take();

		Config::ClientQuestLog(quest, newConnInfo->ip, newConnInfo->port);

		FPAnswerPtr answer;
		//if (timeout == 0)
			answer = ClientEngine::nakedInstance()->sendQuest(socket, token, &_mutex, quest, _timeoutQuest);
		//else
		//	answer = ClientEngine::nakedInstance()->sendQuest(socket, token, &_mutex, quest, timeout * 1000);

		if (answer->status() != 0)
		{
			LOG_ERROR("Client's key exchanging failed. Peer %s", newConnInfo->str().c_str());
			
			TCPClientConnection* conn = (TCPClientConnection*)_engine->takeConnection(newConnInfo.get());
			if (conn == NULL)
				return false;

			_engine->exitEpoll(conn);
			clearConnectionQuestCallbacks(conn, FPNN_EC_CORE_UNKNOWN_ERROR);
			errorAndWillBeClosed(conn);
			return false;
		}
	}

	errorGuard.cancel();
	{
		std::unique_lock<std::mutex> lck(_mutex);
		if (_connectionInfo.get() == currConnInfo.get())
		{
			_connectionInfo = newConnInfo;
			_connected = true;
			_connStatus = ConnStatus::Connected;
			_condition.notify_all();

			return true;
		}
	}

	LOG_ERROR("This codes (TCPClient::connect dupled) is impossible touched. This is just a safety inspection. If this ERROR triggered, please tell swxlion to fix it.");

	//-- dupled
	TCPClientConnection* conn = (TCPClientConnection*)_engine->takeConnection(newConnInfo.get());
	if (conn)
	{
		_engine->exitEpoll(conn);
		clearConnectionQuestCallbacks(conn, FPNN_EC_CORE_CONNECTION_CLOSED);
		willClose(conn);
	}

	std::unique_lock<std::mutex> lck(_mutex);

	while (_connStatus == ConnStatus::Connecting || _connStatus == ConnStatus::KeyExchanging)
		_condition.wait(lck);

	_condition.notify_all();
	if (_connStatus == ConnStatus::Connected)
		return true;

	return false;
}

TCPClientPtr Client::createTCPClient(const std::string& host, int port, bool autoReconnect)
{
	return TCPClient::createClient(host, port, autoReconnect);
}
TCPClientPtr Client::createTCPClient(const std::string& endpoint, bool autoReconnect)
{
	return TCPClient::createClient(endpoint, autoReconnect);
}
