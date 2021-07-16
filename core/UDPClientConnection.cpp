
#include "ClientEngine.h"
#include "UDPClientConnection.h"
#include "UDPClient.h"

using namespace fpnn;

//====================================================//
//--                 UDPClientConnection                  --//
//====================================================//
bool UDPClientConnection::waitForAllEvents()
{
	return ClientEngine::nakedInstance()->waitForAllEvents(this);
}

int UDPClientConnection::send(bool& needWaitSendEvent, std::string* data)
{
	//_activeTime = slack_real_sec();
	//_ioBuffer.sendData(needWaitSendEvent, data);
	LOG_ERROR("Unused interface/method for UDPClientConnection. If this ERROR triggered, please tell swxlion to fix it.");
	//-- Unused
	return -1;
}

void UDPClientConnection::sendCachedData(bool& needWaitSendEvent, bool socketReady)
{
	bool blockByFlowControl = false;
	_ioBuffer.sendCachedData(needWaitSendEvent, blockByFlowControl, socketReady);
	_activeTime = slack_real_sec();
}
void UDPClientConnection::sendData(bool& needWaitSendEvent, std::string* data, int64_t expiredMS, bool discardable)
{
	bool blockByFlowControl = false;
	_ioBuffer.sendData(needWaitSendEvent, blockByFlowControl, data, expiredMS, discardable);
	_activeTime = slack_real_sec();
}

bool UDPClientConnection::recvData(std::list<FPQuestPtr>& questList, std::list<FPAnswerPtr>& answerList)
{
	bool status = _ioBuffer.recvData();
	questList.swap(_ioBuffer.getReceivedQuestList());
	answerList.swap(_ioBuffer.getReceivedAnswerList());

	if (questList.size() || answerList.size())
		_activeTime = slack_real_sec();

	return status;
}

//====================================================//
//--              UDPClientIOWorker                 --//
//====================================================//
void UDPClientIOWorker::read(UDPClientConnection * connection)
{
	if (!connection->getRecvToken())
		return;
	
	std::list<FPQuestPtr> questList;
	std::list<FPAnswerPtr> answerList;
	
	bool goon = true;
	while (goon && connection->isRequireClose() == false)
	{
		goon = connection->recvData(questList, answerList);

		for (auto& answer: answerList)
			if (!deliverAnswer(connection, answer))
				break;
		
		for (auto& quest: questList)
			if (!deliverQuest(connection, quest))
				break;

		questList.clear();
		answerList.clear();
	}

	connection->returnRecvToken();
}

bool UDPClientIOWorker::deliverAnswer(UDPClientConnection * connection, FPAnswerPtr answer)
{
	UDPClientPtr client = connection->client();
	if (client)
	{
		client->dealAnswer(answer, connection->_connectionInfo);
		return true;
	}
	else
	{
		return false;
	}
}

bool UDPClientIOWorker::deliverQuest(UDPClientConnection * connection, FPQuestPtr quest)
{
	UDPClientPtr client = connection->client();
	if (client)
	{
		client->dealQuest(quest, connection->_connectionInfo);
		return true;
	}
	else
	{
		LOG_ERROR("UDP duplex client is destroyed. Connection will be closed. %s", connection->_connectionInfo->str().c_str());
		return false;
	}
}

void UDPClientIOWorker::run(UDPClientConnection * connection)
{
	if (connection->_needRecv)
	{
		read(connection);
		connection->_needRecv = false;
		connection->_needSend = true;		//-- receive UDP 后，强制调用 send 检查是否有数据需要发送（大概率有ARQ应答包需要发送）。
	}

	bool needWaitSendEvent = false;
	if (connection->_needSend)
	{
		connection->sendCachedData(needWaitSendEvent, true);
		connection->_needSend = false;
	}

	if (connection->isRequireClose())
	{
		closeConnection(connection);
		return;
	}

	{
		if (needWaitSendEvent)
		{
			if (ClientEngine::nakedInstance()->waitForAllEvents(connection))
			{
				connection->_refCount--;
				return;
			}
		}
		else
		{
			if (ClientEngine::nakedInstance()->waitForRecvEvent(connection))
			{
				connection->_refCount--;
				return;
			}
		}

		LOG_INFO("UDP connection wait event failed. System memory maybe run out. Connection maybe unusable. %s", connection->_connectionInfo->str().c_str());
		//connection->_refCount--;
	}

	//-- If UDP connection wait event failed, system memory maybe run out. We close this connection/session.
	closeConnection(connection);
}

void UDPClientIOWorker::closeConnection(UDPClientConnection * connection)
{
	if (ClientEngine::nakedInstance()->takeConnection(connection->socket()) == NULL)
	{
		connection->_refCount--;
		return;
	}

	ClientEngine::nakedInstance()->exitEpoll(connection);
	connection->_refCount--;

	UDPClientPtr client = connection->client();
	if (client)
	{
		client->clearConnectionQuestCallbacks(connection, FPNN_EC_CORE_CONNECTION_CLOSED);
		client->willClose(connection);
	}
	else
	{
		ClientEngine::nakedInstance()->clearConnectionQuestCallbacks(connection, FPNN_EC_CORE_INVALID_CONNECTION);

		IQuestProcessorPtr processor = connection->questProcessor();
		if (processor)
		{
			std::shared_ptr<ClientCloseTask> task(new ClientCloseTask(processor, connection, false));
			ClientEngine::wakeUpAnswerCallbackThreadPool(task);
			ConnectionReclaimer::nakedInstance()->reclaim(task);
		}
		else
		{
			ConnectionReclaimTaskPtr task(new ConnectionReclaimTask(connection));
			ConnectionReclaimer::nakedInstance()->reclaim(task);
		}
	}
}
