#include <errno.h>
#include "FPLog.h"
#include "Config.h"
#include "KeyExchange.h"
#include "TCPEpollServer.h"
#include "ServerIOWorker.h"
#include "ConnectionReclaimer.h"

using namespace fpnn;

bool TCPServerConnection::joinEpoll()
{
	if (_joined)
		return true;

	struct epoll_event	ev;

	ev.data.fd = _connectionInfo->socket;
	ev.events = EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLET | EPOLLRDHUP | EPOLLONESHOT;

	if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, _connectionInfo->socket, &ev) != 0)
		return false;
	
	_joined = true;
	return true;
}


bool TCPServerConnection::waitForAllEvents()
{
	struct epoll_event	ev;

	ev.data.fd = _connectionInfo->socket;
	if (_disposable == false)
		ev.events = EPOLLIN | EPOLLOUT | EPOLLERR | EPOLLHUP | EPOLLET | EPOLLRDHUP | EPOLLONESHOT;
	else
		ev.events = EPOLLOUT | EPOLLERR | EPOLLHUP | EPOLLET | EPOLLRDHUP | EPOLLONESHOT;

	if (epoll_ctl(_epoll_fd, EPOLL_CTL_MOD, _connectionInfo->socket, &ev) != 0)
		return false;
	else
		return true;
}
bool TCPServerConnection::waitForRecvEvent()
{
	struct epoll_event	ev;

	ev.data.fd = _connectionInfo->socket;
	if (_disposable == false)
		ev.events = EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLET | EPOLLRDHUP | EPOLLONESHOT;
	else
		ev.events = EPOLLERR | EPOLLHUP | EPOLLET | EPOLLRDHUP | EPOLLONESHOT;

	if (epoll_ctl(_epoll_fd, EPOLL_CTL_MOD, _connectionInfo->socket, &ev) != 0)
		return false;
	else
		return true;
}

void TCPServerConnection::exitEpoll()
{
	if (!_joined)
		return;

	struct epoll_event	ev;

	ev.data.fd = _connectionInfo->socket;
	ev.events = 0;

	epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, _connectionInfo->socket, &ev);
	_joined = false;
}

bool TCPServerIOWorker::read(TCPServerConnection * connection, bool& additionalSend)
{
	if (!connection->_recvBuffer.getToken())
		return true;

	while (true)
	{
		bool needNextEvent;
		if (connection->recvPackage(needNextEvent) == false)
		{
			connection->_recvBuffer.returnToken();
			LOG_ERROR("Error occurred when server receiving. Connection will be closed soon. %s", connection->_connectionInfo->str().c_str());
			return false;
		}
		
		connection->_activeTime = slack_real_sec();

		if (needNextEvent)
		{
			connection->_recvBuffer.returnToken();
			return true;
		}

		bool isHTTP;
		FPQuestPtr quest;
		FPAnswerPtr answer;
		bool status = connection->_recvBuffer.fetch(quest, answer, isHTTP);
		if (status == false)
		{
			LOG_ERROR("Server receiving & decoding data error. Connection will be closed soon. %s", connection->_connectionInfo->str().c_str());
			return false;
		}
		if (isHTTP == false)
		{
			if (quest)
			{
				if (deliverQuest(connection, quest, additionalSend) == false)
				{
					connection->_recvBuffer.returnToken();
					return false;
				}
			}
			else
			{
				if (deliverAnswer(connection, answer) == false)
				{
					connection->_recvBuffer.returnToken();
					return false;
				}
			}
		}
		else
		{
			if (connection->isWebSocket())
			{
				if (quest)
				{
					if (deliverQuest(connection, quest, additionalSend) == false)
					{
						connection->_recvBuffer.returnToken();
						return false;
					}
				}
				else if (answer)
				{
					if (deliverAnswer(connection, answer) == false)
					{
						connection->_recvBuffer.returnToken();
						return false;
					}
				}
				else
				{
					uint8_t opCode = connection->_recvBuffer.getWebSocketOpCode();
					if (opCode == 0x9)
					{
						uint8_t code = 0xa;
						std::string *strCode = new std::string((char*)&code, 1);
						connection->_sendBuffer.appendData(strCode);
						additionalSend = true;
					}
					else if (opCode == 0xa) {}
					else if (opCode == 0x8)
					{
						connection->_requireClose = true;
						connection->_recvBuffer.returnToken();
						return true;
					}
					else
					{
						connection->_recvBuffer.returnToken();
						LOG_ERROR("Receive unsupported opCode %u from webSocket. Connection will be closed soon. %s", opCode, connection->_connectionInfo->str().c_str());
						return false;
					}
				}
			}
			else
			{
				std::string webSocketKey = quest->getWebSocket();
				if (webSocketKey.empty())
				{
					if (Config::_server_http_close_after_answered)
						connection->_disposable = true;

					if (deliverQuest(connection, quest, additionalSend) == false)
					{
						connection->_recvBuffer.returnToken();
						return false;
					}

					if (connection->_disposable)
						break;
				}
				else
				{
					FPAnswerPtr answer = FPAWriter::emptyAnswer(quest);
					std::string *raw = answer->raw();
					connection->entryWebSocketMode(raw);
					additionalSend = true;
				}
			}
		}
	}
	connection->_recvBuffer.returnToken();
	return true;
}

bool TCPServerIOWorker::processECDH(TCPServerConnection * connection, FPQuestPtr quest)
{
	const char* reason = "Encrypt key calculate failed.";
	try
	{
		FPQReader qr(quest);
		std::string publicKey = qr.wantString("publicKey");
		bool streamMode = qr.getBool("streamMode", false);
		int bits = qr.getInt("bits", 128);

		if (bits != 128 && bits != 256)
		{
			LOG_ERROR("Invalid bits %d. Peer %s", bits, connection->_connectionInfo->str().c_str());
			return false;
		}

		int keyLen = bits/8;
		uint8_t key[32];
		uint8_t iv[16];
		if (_server->calcEncryptorKey(key, iv, keyLen, publicKey) == false)
		{
			LOG_ERROR("calcKey failed. Peer %s", connection->_connectionInfo->str().c_str());
			return false;
		}
		if (!connection->entryEncryptMode(key, keyLen, iv, streamMode))  //-- Error log in entryEncryptMode() function.
			return false;

		reason = "Generate normal answer for key exchange failed.";
		FPAnswerPtr answer = FPAWriter::emptyAnswer(quest);
		std::string *raw = answer->raw();
		connection->_sendBuffer.appendData(raw);
		return true;
	}
	catch (const FpnnError& ex)
	{
		LOG_ERROR("%s Connection will be closed by server. %s, exception:(%d)%s",
			reason, connection->_connectionInfo->str().c_str(), ex.code(), ex.what());
		return false;
	}
	catch (...)
	{
		LOG_ERROR("%s Connection will be closed by server. %s", reason, connection->_connectionInfo->str().c_str());
		return false;
	}
}

bool TCPServerIOWorker::returnServerStoppingAnswer(TCPServerConnection * connection, FPQuestPtr quest)
{
	try
	{
		FPAnswerPtr answer = FPAWriter::errorAnswer(quest, FPNN_EC_CORE_SERVER_STOPPING, "", _server->sName());
		std::string *raw = answer->raw();
		connection->_sendBuffer.appendData(raw);
		return true;
	}
	catch (const FpnnError& ex)
	{
		LOG_ERROR("Connection will be closed by server when return server stopping answer. %s, exception:(%d)%s",
			connection->_connectionInfo->str().c_str(), ex.code(), ex.what());
		return false;
	}
	catch (...)
	{
		LOG_ERROR("Connection will be closed by server when return server stopping answer. %s", connection->_connectionInfo->str().c_str());
		return false;
	}
}

bool TCPServerIOWorker::deliverAnswer(TCPServerConnection * connection, FPAnswerPtr answer)
{
	_server->dealAnswer(connection->_connectionInfo->socket, answer);
	return true;
}

bool TCPServerIOWorker::deliverQuest(TCPServerConnection * connection, FPQuestPtr quest, bool& additionalSend)
{
	//-- additionalSend: Don't assign false. avoid erase the flag in other quest deliver in parent's cycle.
	if (_serverIsStopping)
	{
		if (returnServerStoppingAnswer(connection, quest))
		{
			additionalSend = true;
			return true;
		}
		return false;
	}

	bool prior = false;
	const std::string& questMethod = quest->method();

	if (questMethod[0] == '*')
	{
		if (questMethod == "*key")
		{
			if (processECDH(connection, quest))
			{
				additionalSend = true;
				return true;
			}
			return false;
		}
		
		if (_server->isMasterMethod(questMethod))
			prior = true;
	}

	RequestPackage* requestPackage = new(std::nothrow) RequestPackage(IOEventType::Recv, connection->_connectionInfo, quest);
	if (!requestPackage)
	{
		LOG_ERROR("Alloc Event package for recv event failed. Connection will be closed by server. %s", connection->_connectionInfo->str().c_str());
		return false;
	}

	bool wakeUpResult;
	if (!prior)
	{
		if (Config::_server_user_methods_force_encrypted && !connection->isEncrypted())
		{
			LOG_ERROR("All user methods reuiqre encrypted. Unencrypted connection will visit %s. Connection will be closed by server. %s",
				questMethod.c_str(), connection->_connectionInfo->str().c_str());

			delete requestPackage;
			return false;
		}

		wakeUpResult = _workerPool->wakeUp(requestPackage);
	}
	else
		wakeUpResult = _workerPool->priorWakeUp(requestPackage);

	if (wakeUpResult == false)
	{
		bool rev;
		if (!_workerPool->exiting())
		{
			LOG_ERROR("Worker pool wake up for socket recv event failed, worker Pool length limitation is caught. %s",
				connection->_connectionInfo->str().c_str());
			rev = true;

			if (quest->isTwoWay())
			{
				try
				{
					FPAnswerPtr answer = FPAWriter::errorAnswer(quest, FPNN_EC_CORE_WORK_QUEUE_FULL, "Server queue full", connection->_connectionInfo->str().c_str());
					std::string *raw = answer->raw();
					connection->_sendBuffer.appendData(raw);
				}
				catch (const FpnnError& ex)
				{
					LOG_ERROR("Generate error answer for server worker queue full failed. Connection will be closed by server. %s, exception:(%d)%s",
						connection->_connectionInfo->str().c_str(), ex.code(), ex.what());
					rev = false;
				}
				catch (...)
				{
					LOG_ERROR("Generate error answer for server worker queue full failed. Connection will be closed by server. %s",
						connection->_connectionInfo->str().c_str());
					rev = false;
				}
			}
		}
		else
		{
			LOG_ERROR("Worker pool wake up for socket recv event failed, server is exiting. Connection will be closed by server. %s",
				connection->_connectionInfo->str().c_str());
			rev = false;
		}

		delete requestPackage;
		return rev;
	}
	return true;
}

void TCPServerIOWorker::closeConnection(TCPServerConnection * connection, bool error)
{
	if (_server->takeConnection(connection->_connectionInfo->socket) == NULL)
	{
		connection->_refCount--;
		return;
	}
	connection->exitEpoll();
	_server->clearConnectionQuestCallbacks(connection, FPNN_EC_CORE_INVALID_CONNECTION);
	connection->_refCount--;

	RequestPackage* requestPackage = NULL;
	if (error)
		requestPackage = new(std::nothrow) RequestPackage(IOEventType::Error, connection->_connectionInfo, connection);
	else
		requestPackage = new(std::nothrow) RequestPackage(IOEventType::Closed, connection->_connectionInfo, connection);

	if (!requestPackage)
	{
		LOG_ERROR("Alloc Event package for %s failed. Connection will be closed without calling closing event. %s",
			error ? "send/recv error" : "close connection", connection->_connectionInfo->str().c_str());

		TCPServerConnectionPtr autoRelease(connection);
		ConnectionReclaimer::nakedInstance()->reclaim(autoRelease);
		return;
	}

	if (_workerPool->forceWakeUp(requestPackage) == false)
	{
		LOG_ERROR("Worker pool wake up for %s failed. Server is exiting. Connection will be closed without calling closing event. %s",
			error ? "send/recv error" : "close connection",
			connection->_connectionInfo->str().c_str());

		delete requestPackage;

		TCPServerConnectionPtr autoRelease(connection);
		ConnectionReclaimer::nakedInstance()->reclaim(autoRelease);
		return;
	}
}

bool TCPServerIOWorker::sendData(TCPServerConnection * connection, bool& fdInvalid, bool& needWaitSendEvent)
{
	fdInvalid = false;
	needWaitSendEvent = false;
	int errno_ = connection->send(needWaitSendEvent);
	switch (errno_)
	{
	case 0:
		break;

	case EPIPE:
	case EBADF:
	case EINVAL:
	default:
		fdInvalid = true;
		LOG_ERROR("Sending error. Connection will be closed soon. %s", connection->_connectionInfo->str().c_str());
	}
	connection->_needSend = false;

	if (!fdInvalid && !needWaitSendEvent && connection->_disposable)
	{
		closeConnection(connection, false);
		return false;
	}
	return true;
}

void TCPServerIOWorker::run(TCPServerConnection * connection)
{
	bool fdInvalid = false;
	bool needWaitSendEvent = false;

	if (connection->_needSend)
	{
		if (sendData(connection, fdInvalid, needWaitSendEvent) == false)
			return;
	}
	if (!fdInvalid && connection->_needRecv)
	{
		bool additionalSend = false;
		fdInvalid = !read(connection, additionalSend);
		connection->_needRecv = false;

		if (additionalSend && !fdInvalid && !needWaitSendEvent)
			if (sendData(connection, fdInvalid, needWaitSendEvent) == false)
				return;

		if (connection->_requireClose)
		{
			closeConnection(connection, false);
			return;
		}
	}
	
	if (fdInvalid == false)
	{
		if (needWaitSendEvent)
		{
			if (connection->waitForAllEvents())
			{
				connection->_refCount--;
				return;
			}
		}
		else
		{
			if (connection->waitForRecvEvent())
			{
				connection->_refCount--;
				return;
			}
		}

		LOG_ERROR("Server wait socket event failed. Connection will be closed. %s", connection->_connectionInfo->str().c_str());
	}

	closeConnection(connection, true);
}
