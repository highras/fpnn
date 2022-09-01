#include "FpnnTCPClient.h"

using namespace fpnn;

void FpnnTCPSuperQuestProcessor::connected(const ConnectionInfo& info)
{
	if (_realProcessor)
		_realProcessor->connected(info);
}

void FpnnTCPSuperQuestProcessor::connectionWillClose(const ConnectionInfo& info, bool closeByError)
{
	if (_realProcessor)
	{
		try {
			_realProcessor->connectionWillClose(info, closeByError);
		}
		catch (const FpnnError& ex){
			LOG_ERROR("connectionWillClose() error:(%d)%s. %s", ex.code(), ex.what(), info.str().c_str());
		}
		catch (const std::exception& ex)
		{
			LOG_ERROR("connectionWillClose() error: %s. %s", ex.what(), info.str().c_str());
		}
		catch (...)
		{
			LOG_ERROR("Unknown error when calling connectionWillClose() function. %s", info.str().c_str());
		}
	}

	ClientCenter::unregisterConnection(info.socket);
}

bool FpnnTCPClient::sendQuestWithBasicAnswerCallback(FPQuestPtr quest, BasicAnswerCallback* callback, int msec_timeout)
{
	if (!quest)
		return false;

	if (quest->isTwoWay() && !callback)
		return false;

	std::string* raw = NULL;
	try
	{
		raw = quest->raw();
	}
	catch (const FpnnError& ex){
		LOG_ERROR("Quest Raw Exception:(%d)%s", ex.code(), ex.what());
		return false;
	}
	catch (const std::exception& ex)
	{
		LOG_ERROR("Quest Raw Exception: %s", ex.what());
		return false;
	}
	catch (...)
	{
		LOG_ERROR("Quest Raw Exception.");
		return false;
	}

	uint32_t seqNum = quest->seqNumLE();

	if (callback)
	{
		if (msec_timeout == 0)
			msec_timeout = FPNN_DEFAULT_QUEST_TIMEOUT * 1000;
		
		callback->updateExpiredTime(slack_real_msec() + msec_timeout);

		int socket = _client->socket();
		ClientCenter::registerCallback(socket, seqNum, callback);
	}

	if (_client->sendData(raw))
		return true;

	delete raw;

	if (callback)
	{
		int socket = _client->socket();
		BasicAnswerCallback* rev = ClientCenter::takeCallback(socket, seqNum);
		if (rev == NULL)
		{
			//-- Callback is token when ioBuffer->sendData() for closing or other rare cases.
			return true;
		}
	}

	return false;
}

FPAnswerPtr FpnnTCPClient::sendQuest(FPQuestPtr quest, int timeout)
{
	if (_client->connected() == false)
	{
		if (!_autoReconnect)
		{
			if (quest->isTwoWay())
				return FpnnErrorAnswer(quest, FPNN_EC_CORE_CONNECTION_CLOSED, "Client is not allowed auto-connected.");
			else
				return NULL;
		}

		if (!reconnect())
		{
			if (quest->isTwoWay())
				return FpnnErrorAnswer(quest, FPNN_EC_CORE_CONNECTION_CLOSED, "Reconnection failed.");
			else
				return NULL;
		}
	}

	ConnectionInfoPtr connInfo = _client->connectionInfo();
	Config::ClientQuestLog(quest, connInfo->ip, connInfo->port);

	if (!quest->isTwoWay())
	{
		sendQuestWithBasicAnswerCallback(quest, NULL, 0);
		return NULL;
	}

	std::mutex mutex;
	std::shared_ptr<SyncedAnswerCallback> s(new SyncedAnswerCallback(&mutex, quest));
	if (!sendQuestWithBasicAnswerCallback(quest, s.get(), timeout * 1000))
	{
		return FpnnErrorAnswer(quest, FPNN_EC_CORE_SEND_ERROR, "unknown sending error.");
	}

	return s->takeAnswer();
}

bool FpnnTCPClient::sendQuest(FPQuestPtr quest, AnswerCallback* callback, int timeout)
{
	if (_client->connected() == false)
	{
		if (!_autoReconnect)
			return false;

		if (!reconnect())
			return false;
	}

	ConnectionInfoPtr connInfo = _client->connectionInfo();
	Config::ClientQuestLog(quest, connInfo->ip, connInfo->port);

	return sendQuestWithBasicAnswerCallback(quest, callback, timeout * 1000);
}

bool FpnnTCPClient::sendQuest(FPQuestPtr quest, std::function<void (FPAnswerPtr answer, int errorCode)> task, int timeout)
{
	if (_client->connected() == false)
	{
		if (!_autoReconnect)
			return false;

		if (!reconnect())
			return false;
	}

	ConnectionInfoPtr connInfo = _client->connectionInfo();
	Config::ClientQuestLog(quest, connInfo->ip, connInfo->port);

	BasicAnswerCallback* t = new FunctionAnswerCallback(std::move(task));
	if (sendQuestWithBasicAnswerCallback(quest, t, timeout * 1000))
		return true;
	else
	{
		delete t;
		return false;
	}
}
