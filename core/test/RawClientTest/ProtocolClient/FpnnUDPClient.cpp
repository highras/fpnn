#include "FpnnUDPClient.h"

using namespace fpnn;

void FpnnUDPSuperQuestProcessor::connected(const ConnectionInfo& info)
{
	_rawDataProcessor->connect(info.socket, info.isPrivateIP());
	
	if (_realProcessor)
	{
		_realProcessor->connected(info);
	}
}

void FpnnUDPSuperQuestProcessor::connectionWillClose(const ConnectionInfo& info, bool closeByError)
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

	_rawDataProcessor->close();
}

FPAnswerPtr FpnnUDPClient::sendQuest(FPQuestPtr quest, int timeout)
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

	return _rawDataProcessor->sendQuest(quest, timeout);
}

bool FpnnUDPClient::sendQuest(FPQuestPtr quest, AnswerCallback* callback, int timeout)
{
	if (_client->connected() == false)
	{
		if (!_autoReconnect)
			return false;

		if (!reconnect())
			return false;
	}

	return _rawDataProcessor->sendQuest(quest, callback, timeout);
}

bool FpnnUDPClient::sendQuest(FPQuestPtr quest, std::function<void (FPAnswerPtr answer, int errorCode)> task, int timeout)
{
	if (_client->connected() == false)
	{
		if (!_autoReconnect)
			return false;

		if (!reconnect())
			return false;
	}

	return _rawDataProcessor->sendQuest(quest, std::move(task), timeout);
}
