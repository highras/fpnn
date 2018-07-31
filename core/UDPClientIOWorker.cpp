#include "Decoder.h"
#include "UDPClient.h"
#include "UDPClientIOWorker.h"
#include "ClientEngine.h"

using namespace fpnn;

bool UDPClientConnection::waitForAllEvents()
{
	return ClientEngine::nakedInstance()->waitForAllEvents(this);
}

void UDPClientIOWorker::read(UDPClientConnection * connection)
{
	if (!connection->getRecvToken())
		return;

	UDPClientRecvBuffer recvBuffer;
	std::list<FPQuestPtr> questList;
	std::list<FPAnswerPtr> answerList;
	
	while (recvBuffer.recv(connection->_connectionInfo, questList, answerList))
	{
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
	}

	bool needWaitSendEvent = false;
	if (connection->_needSend)
	{
		connection->send(needWaitSendEvent);
		connection->_needSend = false;
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

		LOG_INFO("UDP client connection wait event failed. System memory maybe run out. Connection maybe unusable. %s", connection->_connectionInfo->str().c_str());
		//connection->_refCount--;
	}

	//-- If UDP client connection wait event failed, system memory maybe run out. We close this connection/session.
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
		client->clearConnectionQuestCallbacks(connection, FPNN_EC_CORE_INVALID_CONNECTION);
		client->errorAndWillBeClosed(connection);
	}
	else
	{
		ClientEngine::nakedInstance()->clearConnectionQuestCallbacks(connection, FPNN_EC_CORE_INVALID_CONNECTION);
		
		//CloseErrorTaskPtr task(new CloseErrorTask(connection, true));
		//ClientEngine::wakeUpAnswerCallbackThreadPool(task);

		LOG_ERROR("This codes (UDPClientIOWorker::run) is impossible touched. This is just a safety inspection. If this ERROR triggered, please tell swxlion to add old CloseErrorTask class back, and fix it.");
		ConnectionReclaimTaskPtr task(new ConnectionReclaimTask(connection));
		ConnectionReclaimer::nakedInstance()->reclaim(task);
	}
}
