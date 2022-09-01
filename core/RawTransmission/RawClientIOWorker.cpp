#include "../ClientEngine.h"
#include "RawClientInterface.h"
#include "RawClientIOWorker.h"

using namespace fpnn;

bool RawClientBasicConnection::waitForAllEvents()
{
	return ClientEngine::nakedInstance()->waitForAllEvents(this);
}

void RawClientBasicConnection::dealReceivedRawData(std::list<ReceivedRawData*>& rawDataList)
{
	RawClientPtr rawClient = client();
	if (rawClient)
	{
		//-- _activeTime vaule maybe in confusion with concurrent Sending on one connection.
		//-- But the probability is very low even server with high load. So, it hasn't be adjusted at current.
		_activeTime = slack_real_sec();

		rawClient->dealReceivedRawData(_connectionInfo, rawDataList);
	}
	else
	{
		int count = (int)rawDataList.size();
		int bytes = 0;

		for (auto rawData: rawDataList)
		{
			bytes += (int)(rawData->len);
			delete rawData;
		}

		rawDataList.clear();

		LOG_WARN("Raw Client Connection dealReceivedRawData() error. Client is destroyed. %s. Raw data count %d, total %d bytes.",
			_connectionInfo->str().c_str(), count, bytes);
	}
}

bool RawClientBasicConnection::recv()
{
	if (!_receiver->getToken())
		return true;

	const size_t maxListSize = 10;
	std::list<ReceivedRawData*> dataList;

	while (true)
	{
		if (_receiver->recv(_connectionInfo->socket))
		{
			ReceivedRawData* rawData = _receiver->fetch();
			if (rawData)
			{
				dataList.push_back(rawData);
				if (dataList.size() >= maxListSize)
					dealReceivedRawData(dataList);
			}
			else
			{
				_receiver->returnToken();

				if (dataList.size())
					dealReceivedRawData(dataList);

				if (_receiver->requireClose() == false)
					return true;
				else
					return false;
			}
		}
		else
		{
			_receiver->returnToken();
			LOG_ERROR("Error occurred when Raw Client receiving. Connection will be closed soon. %s", _connectionInfo->str().c_str());
			
			if (dataList.size())
				dealReceivedRawData(dataList);

			return false;
		}
	}
}

int RawClientBasicConnection::send(bool& needWaitSendEvent, std::string* data)
{
	bool actualSent = false;
	//-- _activeTime vaule maybe in confusion after concurrent Sending on one connection.
	//-- But the probability is very low even server with high load. So, it hasn't be adjusted at current.
	_activeTime = slack_real_sec();
	return _sender->send(_connectionInfo->socket, needWaitSendEvent, actualSent, data);
}


void RawClientIOWorker::run(RawClientBasicConnection * connection)
{
	bool connInvalid = false;
	bool needWaitSendEvent = false;

	if (connection->_needRecv)
	{
		connInvalid = connection->recv() == false;
		connection->_needRecv = false;
	}

	if (!connInvalid && connection->_needSend)
	{
		if (connection->send(needWaitSendEvent) != 0)
		{
			connInvalid = true;
			LOG_ERROR("Raw TCP Client connection sending error. Connection will be closed soon. %s", connection->_connectionInfo->str().c_str());
		}

		connection->_needSend = false;
	}

	if (connInvalid == false)
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

		LOG_INFO("Raw Client connection wait event failed. Connection will be closed. %s", connection->_connectionInfo->str().c_str());
	}

	closeConnection(connection);
}

void RawClientIOWorker::closeConnection(RawClientBasicConnection * connection)
{
	if (ClientEngine::nakedInstance()->takeConnection(connection->socket()) == NULL)
	{
		connection->_refCount--;
		return;
	}

	ClientEngine::nakedInstance()->exitEpoll(connection);
	connection->_refCount--;

	RawClientPtr client = connection->client();
	if (client)
	{
		client->willClose(connection);
	}
	else
	{
		LOG_ERROR("This codes (RawClientIOWorker::run) is impossible touched. This is just a safety inspection. If this ERROR triggered, please tell swxlion to add old CloseErrorTask class back, and fix it.");
		ConnectionReclaimTaskPtr task(new ConnectionReclaimTask(connection));
		ConnectionReclaimer::nakedInstance()->reclaim(task);
	}
}