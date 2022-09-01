#include "../Config.h"
#include "FPLog.h"
#include "HostLookup.h"
#include "NetworkUtility.h"
#include "RawClientInterface.h"

using namespace fpnn;

RawClient::RawClient(const std::string& host, int port, bool autoReconnect): _connected(false),
	_connStatus(ConnStatus::NoConnected), _rawDataProcessPool(NULL), _autoReconnect(autoReconnect)
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

	_rawDataProcessor = std::make_shared<IRawDataProcessor>();
}

RawClient::~RawClient()
{
	if (_connected)
		close();

	if (_rawDataProcessPool)
	{
		_rawDataProcessPool->release();
		delete _rawDataProcessPool;
	}
}

void RawClient::connected(BasicConnection* connection)
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
		catch (const std::exception& ex)
		{
			LOG_ERROR("connected() error: %s. %s", ex.what(), connection->_connectionInfo->str().c_str());
		}
		catch (...)
		{
			LOG_ERROR("Unknown error when calling connected() function. %s", connection->_connectionInfo->str().c_str());
		}
	}
}

void RawClient::onErrorOrCloseEvent(BasicConnection* connection, bool error)
{
	std::shared_ptr<ClientCloseTask> task(new ClientCloseTask(_questProcessor, connection, error));
	if (_questProcessor)
	{
		bool wakeup;
		if (_rawDataProcessPool)
			wakeup = _rawDataProcessPool->wakeUp(task);
		else
			wakeup = ClientEngine::wakeUpAnswerCallbackThreadPool(task);

		if (!wakeup)
			LOG_ERROR("wake up thread pool to process raw TCP connection close event failed. Close callback will be called by Connection Reclaimer. %s", connection->_connectionInfo->str().c_str());
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

class ProcessReceivedRawDataTask: public ITaskThreadPool::ITask
{
	ConnectionInfoPtr _connectionInfo;
	IRawDataProcessorPtr _procerssor;
	ReceivedRawData* _rawData;

public:
	ProcessReceivedRawDataTask(ConnectionInfoPtr connectionInfo, IRawDataProcessorPtr procerssor, ReceivedRawData* data):
		_connectionInfo(connectionInfo), _procerssor(procerssor), _rawData(data) {}
	virtual ~ProcessReceivedRawDataTask()
	{
		if (_rawData)
			delete _rawData;
	}
	virtual void run()
	{
		try
		{
			_procerssor->process(_connectionInfo, _rawData->data, _rawData->len);
		}
		catch (const std::exception& ex)
		{
			LOG_ERROR("Process raw data error: %s. Peer %s, raw data %d bytes.", ex.what(), _connectionInfo->str().c_str(), (int)(_rawData->len));
		}
		catch (...)
		{
			LOG_ERROR("Unknown error when process raw data. Peer %s, raw data %d bytes.", _connectionInfo->str().c_str(), (int)(_rawData->len));
		}
	}
};
typedef std::shared_ptr<ProcessReceivedRawDataTask> ProcessReceivedRawDataTaskPtr;

class ProcessReceivedRawDataListTask: public ITaskThreadPool::ITask
{
	bool _chargeType;
	IRawDataProcessorPtr _procerssor;
	ConnectionInfoPtr _connectionInfo;
	std::list<ReceivedRawData*> _rawDataList;

public:
	ProcessReceivedRawDataListTask(ConnectionInfoPtr connectionInfo, bool chargeType, IRawDataProcessorPtr procerssor):
		_chargeType(chargeType), _procerssor(procerssor), _connectionInfo(connectionInfo) {}
	virtual ~ProcessReceivedRawDataListTask()
	{
		if (!_chargeType)
			for (auto data: _rawDataList)
				delete data;
	}
	virtual void run()
	{
		try
		{
			if (!_chargeType)
				((IRawDataBatchProcessor*)(_procerssor.get()))->process(_connectionInfo, _rawDataList);
			else
				((IRawDataChargeProcessor*)(_procerssor.get()))->process(_connectionInfo, _rawDataList);
		}
		catch (const std::exception& ex)
		{
			LOG_ERROR("Batch process raw data error: %s. Peer %s.", ex.what(), _connectionInfo->str().c_str());
		}
		catch (...)
		{
			LOG_ERROR("Unknown error when batch process raw data. Peer %s.", _connectionInfo->str().c_str());
		}
	}
	void fillData(std::list<ReceivedRawData*>& rawDataList)
	{
		_rawDataList.swap(rawDataList);
	}
};
typedef std::shared_ptr<ProcessReceivedRawDataListTask> ProcessReceivedRawDataListTaskPtr;

void RawClient::dealReceivedRawData_standard(ConnectionInfoPtr connectionInfo, std::list<ReceivedRawData*>& rawDataList)
{
	bool wakeup;
	for (auto data: rawDataList)
	{
		ProcessReceivedRawDataTaskPtr task(new ProcessReceivedRawDataTask(connectionInfo, _rawDataProcessor, data));

		if (_rawDataProcessPool)
			wakeup = _rawDataProcessPool->wakeUp(task);
		else
			wakeup = ClientEngine::wakeUpAnswerCallbackThreadPool(task);

		if (!wakeup)
			LOG_ERROR("[Fatal] wake up thread pool to process raw data list failed, data %d bytes. %s",
				(int)(data->len), connectionInfo->str().c_str());
	}

	//-- DO NOT delete inner raw data pointer.
	rawDataList.clear();
}

void RawClient::dealReceivedRawData_batch(ConnectionInfoPtr connectionInfo, std::list<ReceivedRawData*>& rawDataList)
{
	bool charge = (_rawDataProcessor->processType() == IRawDataProcessor::ChargeType);

	ProcessReceivedRawDataListTaskPtr task(new ProcessReceivedRawDataListTask(connectionInfo, charge, _rawDataProcessor));
	task->fillData(rawDataList);

	bool wakeup;
	if (_rawDataProcessPool)
		wakeup = _rawDataProcessPool->wakeUp(task);
	else
		wakeup = ClientEngine::wakeUpAnswerCallbackThreadPool(task);

	if (!wakeup)
		LOG_ERROR("[Fatal] wake up thread pool to process raw data list failed. %s", connectionInfo->str().c_str());
}

void RawClient::dealReceivedRawData(ConnectionInfoPtr connectionInfo, std::list<ReceivedRawData*>& rawDataList)
{
	if (Config::RawClient::log_received_raw_data)
	{
		uint64_t bytes = 0;

		for (auto data: rawDataList)
			bytes += data->len;

		UXLOG("RAW.RECV","%s:%d - %d blocks, %llu bytes", connectionInfo->ip.c_str(), connectionInfo->port, (int)(rawDataList.size()), bytes);
	}

	if (_rawDataProcessor->processType() == IRawDataProcessor::StandardType)
		dealReceivedRawData_standard(connectionInfo, rawDataList);
	else
		dealReceivedRawData_batch(connectionInfo, rawDataList);
}

void RawClient::close()
{
	if (!_connected)
		return;

	ConnectionInfoPtr oldConnInfo;
	{
		std::unique_lock<std::mutex> lck(_mutex);
		while (_connStatus == ConnStatus::Connecting)
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
		如果在 RawClient::_mutex 内调用 takeConnection() 会导致在 singleClientConcurrentTset 中，
		其他线程处于发送状态时，死锁。
	*/
	BasicConnection* conn = _engine->takeConnection(oldConnInfo.get());
	if (conn == NULL)
		return;

	_engine->exitEpoll(conn);
	willClose(conn);
}

bool RawClient::reconnect()
{
	close();
	return connect();
}
