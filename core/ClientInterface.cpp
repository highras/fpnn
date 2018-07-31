#include "HostLookup.h"
#include "NetworkUtility.h"
#include "ClientInterface.h"

using namespace fpnn;

Client::Client(const std::string& host, int port, bool autoReconnect): _connected(false),
	_connStatus(ConnStatus::NoConnected), _answerCallbackPool(nullptr),
	_questProcessPool(nullptr), _timeoutQuest(0), _autoReconnect(autoReconnect)
{
	_engine = ClientEngine::instance();
	_isIPv4 = (host.find(':') == std::string::npos);
	if (_isIPv4)
	{
		_connectionInfo.reset(new ConnectionInfo(0, port, HostLookup::get(host), _isIPv4, false));
		_endpoint = std::string(host + ":").append(std::to_string(port));
	}
	else
	{
		_connectionInfo.reset(new ConnectionInfo(0, port, host, _isIPv4, false));
		_endpoint = std::string("[").append(host).append("]:").append(std::to_string(port));
	}
}

Client::~Client()
{
	if (_connected)
		close();

	if (_answerCallbackPool)
	{
		_answerCallbackPool->release();
		delete _answerCallbackPool;
	}
}

void Client::connected(BasicConnection* connection)
{
	if (_questProcessor)
	{
		try
		{
			_questProcessor->connected(*(connection->_connectionInfo));
		}
		catch (const FpnnError& ex){
			LOG_ERROR("connected() error:(%d)%s. %s", ex.code(), ex.what(), connection->_connectionInfo->str().c_str());
		}
		catch (...)
		{
			LOG_ERROR("Unknown error when calling connected() function. %s", connection->_connectionInfo->str().c_str());
		}
	}
}

void Client::onErrorOrCloseEvent(BasicConnection* connection, bool error)
{
	std::shared_ptr<ClientCloseTask> task(new ClientCloseTask(_questProcessor, connection, error));
	if (_questProcessor)
	{
		bool wakeup;
		if (_answerCallbackPool)
			wakeup = _answerCallbackPool->wakeUp(task);
		else
			wakeup = ClientEngine::wakeUpAnswerCallbackThreadPool(task);

		if (!wakeup)
			LOG_ERROR("wake up thread pool to process connection close event failed. Close callback will be called by Connection Reclaimer. %s", connection->_connectionInfo->str().c_str());
	}

	{
		std::unique_lock<std::mutex> lck(_mutex);
		if (_connectionInfo.get() == connection->_connectionInfo.get())
		{
			ConnectionInfoPtr newConnectionInfo(new ConnectionInfo(0, _connectionInfo->port, _connectionInfo->ip, _isIPv4, false));
			_connectionInfo = newConnectionInfo;
			_connected = false;
			_connStatus = ConnStatus::NoConnected;
		}
	}

	_engine->reclaim(task);	//-- MUST after change _connectionInfo, ensure the socket hasn't been closed before _connectionInfo reset.
}


void Client::dealAnswer(FPAnswerPtr answer, ConnectionInfoPtr connectionInfo)
{
	Config::ClientAnswerLog(answer, connectionInfo->ip, connectionInfo->port);

	BasicAnswerCallback* callback = ClientEngine::nakedInstance()->takeCallback(connectionInfo->socket, answer->seqNumLE());
	if (!callback)
	{
		LOG_ERROR("Recv an invalied answer, seq is %u. %s", answer->seqNumLE(), connectionInfo->str().c_str());
		return;
	}
	if (callback->syncedCallback())		//-- check first, then fill result.
	{
		SyncedAnswerCallback* sac = (SyncedAnswerCallback*)callback;
		sac->fillResult(answer, FPNN_EC_OK);
		return;
	}
	
	callback->fillResult(answer, FPNN_EC_OK);
	BasicAnswerCallbackPtr task(callback);

	bool wakeup;
	if (_answerCallbackPool)
		wakeup = _answerCallbackPool->wakeUp(task);
	else
		wakeup = ClientEngine::wakeUpAnswerCallbackThreadPool(task);

	if (!wakeup)
		LOG_ERROR("[Fatal] wake up thread pool to process answer failed. Close callback havn't called. %s", connectionInfo->str().c_str());
}

void Client::processQuest(FPQuestPtr quest, ConnectionInfoPtr connectionInfo)
{
	FPAnswerPtr answer = NULL;
	_questProcessor->initAnswerStatus(connectionInfo, quest);

	try
	{
		FPReaderPtr args(new FPReader(quest->payload()));
		answer = _questProcessor->processQuest(args, quest, *connectionInfo);
	}
	catch (const FpnnError& ex)
	{
		LOG_ERROR("processQuest ERROR:(%d) %s, connection:%s", ex.code(), ex.what(), connectionInfo->str().c_str());
		if (quest->isTwoWay())
		{
			if (_questProcessor->getQuestAnsweredStatus() == false)
				answer = FpnnErrorAnswer(quest, ex.code(), std::string(ex.what()) + ", " + connectionInfo->str());
		}
	}
	catch (...){
		LOG_ERROR("Unknown error when calling processQuest() function. %s", connectionInfo->str().c_str());
		if (quest->isTwoWay())
		{
			if (_questProcessor->getQuestAnsweredStatus() == false)
				answer = FpnnErrorAnswer(quest, FPNN_EC_CORE_UNKNOWN_ERROR, std::string("Unknown error when calling processQuest() function, ") + connectionInfo->str());
		}
	}

	bool questAnswered = _questProcessor->finishAnswerStatus();
	if (quest->isTwoWay())
	{
		if (questAnswered)
		{
			if (answer)
			{
				LOG_ERROR("Double answered after an advance answer sent, or async answer generated. %s", connectionInfo->str().c_str());
			}
			return;
		}
		else if (!answer)
			answer = FpnnErrorAnswer(quest, FPNN_EC_CORE_UNKNOWN_ERROR, std::string("Twoway quest lose an answer. ") + connectionInfo->str());
	}
	else if (answer)
	{
		LOG_ERROR("Oneway quest return an answer. %s", connectionInfo->str().c_str());
		answer = NULL;
	}

	if (answer)
	{
		std::string* raw = NULL;
		try
		{
			raw = answer->raw();
		}
		catch (const FpnnError& ex){
			FPAnswerPtr errAnswer = FpnnErrorAnswer(quest, ex.code(), std::string(ex.what()) + ", " + connectionInfo->str());
			raw = errAnswer->raw();
		}
		catch (...)
		{
			/**  close the connection is to complex, so, return a error answer. It alway success? */

			FPAnswerPtr errAnswer = FpnnErrorAnswer(quest, FPNN_EC_CORE_UNKNOWN_ERROR, std::string("exception while do answer raw, ") + connectionInfo->str());
			raw = errAnswer->raw();
		}

		ClientEngine::nakedInstance()->sendData(connectionInfo->socket, connectionInfo->token, raw);
	}
}

void Client::close()
{
	if (!_connected)
		return;

	ConnectionInfoPtr oldConnInfo;
	{
		std::unique_lock<std::mutex> lck(_mutex);
		while (_connStatus == ConnStatus::Connecting || _connStatus == ConnStatus::KeyExchanging)
			_condition.wait(lck);

		if (_connStatus == ConnStatus::NoConnected)
			return;

		oldConnInfo = _connectionInfo;

		ConnectionInfoPtr newConnectionInfo(new ConnectionInfo(0, _connectionInfo->port, _connectionInfo->ip, _isIPv4, false));
		_connectionInfo = newConnectionInfo;
		_connected = false;
		_connStatus = ConnStatus::NoConnected;
	}

	/*
		!!! 注意 !!!
		如果在 Client::_mutex 内调用 takeConnection() 会导致在 singleClientConcurrentTset 中，
		其他线程处于发送状态时，死锁。
	*/
	BasicConnection* conn = _engine->takeConnection(oldConnInfo.get());
	if (conn == NULL)
		return;

	_engine->exitEpoll(conn);
	clearConnectionQuestCallbacks(conn, FPNN_EC_CORE_CONNECTION_CLOSED);
	willClose(conn);
}

void Client::clearConnectionQuestCallbacks(BasicConnection* connection, int errorCode)
{
	for (auto callbackPair: connection->_callbackMap)
	{
		BasicAnswerCallback* callback = callbackPair.second;
		if (callback->syncedCallback())		//-- check first, then fill result.
			callback->fillResult(NULL, errorCode);
		else
		{
			callback->fillResult(NULL, errorCode);

			BasicAnswerCallbackPtr task(callback);
			
			bool wakeup;
			if (_answerCallbackPool)
				wakeup = _answerCallbackPool->wakeUp(task);
			else
				wakeup = ClientEngine::wakeUpAnswerCallbackThreadPool(task);

			if (!wakeup)
			{
				LOG_ERROR("wake up thread pool to process quest callback when connection closing failed. Quest callback will be called in current thread. %s", connection->_connectionInfo->str().c_str());
				task->run();
			}
		}
	}
	// connection->_callbackMap.clear(); //-- If necessary.
}

bool Client::reconnect()
{
	close();
	return connect();
}

FPAnswerPtr Client::sendQuest(FPQuestPtr quest, int timeout)
{
	if (!_connected)
	{
		if (!_autoReconnect)
			return FpnnErrorAnswer(quest, FPNN_EC_CORE_CONNECTION_CLOSED, "Client is not allowed auto-connected.");

		if (!reconnect())
			return FpnnErrorAnswer(quest, FPNN_EC_CORE_CONNECTION_CLOSED, "Reconnection failed.");
	}

	ConnectionInfoPtr connInfo;
	{
		std::unique_lock<std::mutex> lck(_mutex);
		connInfo = _connectionInfo;
	}
	Config::ClientQuestLog(quest, connInfo->ip.c_str(), connInfo->port);

	if (timeout == 0)
		return ClientEngine::nakedInstance()->sendQuest(connInfo->socket, connInfo->token, &_mutex, quest, _timeoutQuest);
	else
		return ClientEngine::nakedInstance()->sendQuest(connInfo->socket, connInfo->token, &_mutex, quest, timeout * 1000);
}

bool Client::sendQuest(FPQuestPtr quest, AnswerCallback* callback, int timeout)
{
	if (!_connected)
	{
		if (!_autoReconnect)
			return false;

		if (!reconnect())
			return false;
	}

	ConnectionInfoPtr connInfo;
	{
		std::unique_lock<std::mutex> lck(_mutex);
		connInfo = _connectionInfo;
	}
	Config::ClientQuestLog(quest, connInfo->ip.c_str(), connInfo->port);

	if (timeout == 0)
		return ClientEngine::nakedInstance()->sendQuest(connInfo->socket, connInfo->token, quest, callback, _timeoutQuest);
	else
		return ClientEngine::nakedInstance()->sendQuest(connInfo->socket, connInfo->token, quest, callback, timeout * 1000);
}
bool Client::sendQuest(FPQuestPtr quest, std::function<void (FPAnswerPtr answer, int errorCode)> task, int timeout)
{
	if (!_connected)
	{
		if (!_autoReconnect)
			return false;

		if (!reconnect())
			return false;
	}

	ConnectionInfoPtr connInfo;
	{
		std::unique_lock<std::mutex> lck(_mutex);
		connInfo = _connectionInfo;
	}
	Config::ClientQuestLog(quest, connInfo->ip.c_str(), connInfo->port);

	if (timeout == 0)
		return ClientEngine::nakedInstance()->sendQuest(connInfo->socket, connInfo->token, quest, std::move(task), _timeoutQuest);
	else
		return ClientEngine::nakedInstance()->sendQuest(connInfo->socket, connInfo->token, quest, std::move(task), timeout * 1000);
}