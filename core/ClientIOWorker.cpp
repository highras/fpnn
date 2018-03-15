#include "Decoder.h"
#include "TCPClient.h"
#include "ClientIOWorker.h"
#include "ClientEngine.h"

using namespace fpnn;

bool TCPClientConnection::waitForAllEvents()
{
	return ClientEngine::nakedInstance()->waitForAllEvents(this);
}

bool TCPClientIOWorker::read(TCPClientConnection * connection)
{
	if (!connection->_recvBuffer.getToken())
		return true;

	while (true)
	{
		bool needNextEvent;
		if (connection->recvPackage(needNextEvent) == false)
		{
			connection->_recvBuffer.returnToken();
			LOG_ERROR("Error occurred when client receiving. Connection will be closed soon. %s", connection->_connectionInfo->str().c_str());
			return false;
		}
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
			LOG_ERROR("Client receiving & decoding data error. Connection will be closed soon. %s", connection->_connectionInfo->str().c_str());
			return false;
		}
		if (isHTTP)
		{
			LOG_ERROR("Client just only support FPNN-TCP package, but received a non-FPNN-TCP package. Connection will be closed soon. %s", connection->_connectionInfo->str().c_str());
			return false;
		}
		if (quest)
		{
			if (deliverQuest(connection, quest) == false)
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
	connection->_recvBuffer.returnToken();
	return true;
}

bool TCPClientIOWorker::deliverAnswer(TCPClientConnection * connection, FPAnswerPtr answer)
{
	TCPClientPtr client = connection->client();
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

bool TCPClientIOWorker::deliverQuest(TCPClientConnection * connection, FPQuestPtr quest)
{
	TCPClientPtr client = connection->client();
	if (client)
	{
		client->dealQuest(quest, connection->_connectionInfo);
		return true;
	}
	else
	{
		LOG_ERROR("Duplex client is destroyed. Connection will be closed. %s", connection->_connectionInfo->str().c_str());
		return false;
	}
}

void TCPClientIOWorker::run(TCPClientConnection * connection)
{
	bool fdInvalid = false;
	bool needWaitSendEvent = false;
	if (connection->_needRecv)
	{
		fdInvalid = !read(connection);
		connection->_needRecv = false;
	}
	if (!fdInvalid && connection->_needSend)
	{
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
			LOG_ERROR("Client connection sending error. Connection will be closed soon. %s", connection->_connectionInfo->str().c_str());
		}

		connection->_needSend = false;
	}
	
	if (fdInvalid == false)
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

		LOG_INFO("Client connection wait event failed. Connection will be closed. %s", connection->_connectionInfo->str().c_str());
	}

	if (ClientEngine::nakedInstance()->takeConnection(connection->socket()) == NULL)
	{
		connection->_refCount--;
		return;
	}

	ClientEngine::nakedInstance()->exitEpoll(connection);
	connection->_refCount--;

	TCPClientPtr client = connection->client();
	if (client)
	{
		client->clearConnectionQuestCallbacks(connection, FPNN_EC_CORE_INVALID_CONNECTION);
		client->errorAndWillBeClosed(connection);
	}
	else
	{
		ClientEngine::nakedInstance()->clearConnectionQuestCallbacks(connection, FPNN_EC_CORE_INVALID_CONNECTION);
		
		CloseErrorTaskPtr task(new CloseErrorTask(connection, true));
		ClientEngine::wakeUpAnswerCallbackThreadPool(task);
	}
}
