#include "FPLog.h"
#include "../../../ClientEngine.h"
#include "FpnnClientCenter.h"
#include "FpnnUDPCodec.h"

using namespace fpnn;

void UDPFPNNProtocolProcessor::clearCallbacks()
{
	for (auto& pp: _callbackMap)
	{
		ClientCenter::runCallback(nullptr, FPNN_EC_CORE_CONNECTION_CLOSED, pp.second);
	}
	_callbackMap.clear();
}

void UDPFPNNProtocolProcessor::process(ConnectionInfoPtr connectionInfo, uint8_t* buffer, uint32_t len)
{
	std::shared_ptr<UDPIOBuffer> ioBuffer;
	{
		std::unique_lock<std::mutex> lck(_mutex);
		ioBuffer = _ioBuffer;
	}

	if (ioBuffer && ioBuffer->getRecvToken())
	{
		struct UDPIOReceivedResult result;
		bool available = ioBuffer->parseReceivedData(buffer, len, result);
		ioBuffer->returnRecvToken();

		if (available)
		{
			for (auto answer: result.answerList)
				dealAnswer(connectionInfo, answer);

			for (auto quest: result.questList)
				dealQuest(connectionInfo, quest);
		}

		if (result.requireClose)
		{
			//-- TODO
			//-- This is demo, do nothings.
			LOG_ERROR("Received Close signal, but this just is a demo, so do noting.");
		}
	}
}

class FPNNUDPQuestTask: public ITaskThreadPool::ITask
{
	UDPFPNNProtocolProcessorPtr _udpProcessor;
	ConnectionInfoPtr _connectionInfo;
	FPQuestPtr _quest;

public:
	virtual void run()
	{
		FPAnswerPtr answer = NULL;

		try
		{
			FPReaderPtr args(new FPReader(_quest->payload()));
			answer = _udpProcessor->getQuestProcessor()->processQuest(args, _quest, *_connectionInfo);
		}
		catch (const FpnnError& ex)
		{
			LOG_ERROR("processQuest ERROR:(%d) %s, connection:%s", ex.code(), ex.what(), _connectionInfo->str().c_str());
			if (_quest->isTwoWay())
				answer = FpnnErrorAnswer(_quest, ex.code(), std::string(ex.what()) + ", " + _connectionInfo->str());
		}
		catch (const std::exception& ex)
		{
			LOG_ERROR("processQuest ERROR: %s, connection:%s", ex.what(), _connectionInfo->str().c_str());
			if (_quest->isTwoWay())
				answer = FpnnErrorAnswer(_quest, FPNN_EC_CORE_UNKNOWN_ERROR, std::string(ex.what()) + ", " + _connectionInfo->str());
		}
		catch (...){
			LOG_ERROR("Unknown error when calling processQuest() function. %s", _connectionInfo->str().c_str());
			if (_quest->isTwoWay())
				answer = FpnnErrorAnswer(_quest, FPNN_EC_CORE_UNKNOWN_ERROR, std::string("Unknown error when calling processQuest() function, ") + _connectionInfo->str());
		}

		if (_quest->isTwoWay())
		{
			answer = FpnnErrorAnswer(_quest, FPNN_EC_CORE_UNKNOWN_ERROR, std::string("Twoway quest lose an answer. ") + _connectionInfo->str());
		}
		else if (answer)
		{
			LOG_ERROR("Oneway quest return an answer. %s", _connectionInfo->str().c_str());
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
				FPAnswerPtr errAnswer = FpnnErrorAnswer(_quest, ex.code(), std::string(ex.what()) + ", " + _connectionInfo->str());
				raw = errAnswer->raw();
			}
			catch (const std::exception& ex)
			{
				FPAnswerPtr errAnswer = FpnnErrorAnswer(_quest, FPNN_EC_CORE_UNKNOWN_ERROR, std::string(ex.what()) + ", " + _connectionInfo->str());
				raw = errAnswer->raw();
			}
			catch (...)
			{
				/**  close the connection is to complex, so, return a error answer. It alway success? */

				FPAnswerPtr errAnswer = FpnnErrorAnswer(_quest, FPNN_EC_CORE_UNKNOWN_ERROR, std::string("exception while do answer raw, ") + _connectionInfo->str());
				raw = errAnswer->raw();
			}

			_udpProcessor->sendData(raw);
		}
	}

	FPNNUDPQuestTask(ConnectionInfoPtr connectionInfo, UDPFPNNProtocolProcessorPtr udpProcessor, FPQuestPtr quest):
		_udpProcessor(udpProcessor), _connectionInfo(connectionInfo), _quest(quest) {}
	virtual ~FPNNUDPQuestTask() {}
};

void UDPFPNNProtocolProcessor::dealQuest(ConnectionInfoPtr connectionInfo, FPQuestPtr quest)
{
	std::shared_ptr<FPNNUDPQuestTask> task(new FPNNUDPQuestTask(connectionInfo, shared_from_this(), quest));
	ClientEngine::wakeUpAnswerCallbackThreadPool(task);
}

void UDPFPNNProtocolProcessor::dealAnswer(ConnectionInfoPtr connectionInfo, FPAnswerPtr answer)
{
	Config::ClientAnswerLog(answer, connectionInfo->ip, connectionInfo->port);

	uint32_t seqNum = answer->seqNumLE();
	BasicAnswerCallback* callback = NULL;
	{
		std::unique_lock<std::mutex> lck(_mutex);
		auto it = _callbackMap.find(seqNum);
		if (it != _callbackMap.end())
		{
			callback = it->second;
			_callbackMap.erase(seqNum);
		}
	}

	if (!callback)
	{
		LOG_WARN("Recv an invalid answer, seq: %u. Peer %s:%d, Info: %s", seqNum,
			connectionInfo->ip.c_str(), connectionInfo->port, answer->info().c_str());
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

	ClientEngine::wakeUpAnswerCallbackThreadPool(task);
}

void UDPFPNNProtocolProcessor::connect(int socket, bool isPrivateIP)
{
	int MTU;
	if (isPrivateIP)
		MTU = Config::UDP::_LAN_MTU;
	else
		MTU = Config::UDP::_internet_MTU;

	std::shared_ptr<UDPIOBuffer> ioBuffer;
	{
		std::unique_lock<std::mutex> lck(_mutex);
		
		clearCallbacks();
		ioBuffer = _ioBuffer;

		_socket = socket;
		_ioBuffer.reset(new UDPIOBuffer(&_mutex, socket, MTU));
		_ioBuffer->updateEndpointInfo(std::string(_ip).append(":").append(std::to_string(_port)));
	}

	if (ioBuffer)
	{
		bool needWaitSendEvent = false;
		ioBuffer->sendCloseSignal(needWaitSendEvent);
	}
}

void UDPFPNNProtocolProcessor::close()
{
	std::shared_ptr<UDPIOBuffer> ioBuffer;
	{
		std::unique_lock<std::mutex> lck(_mutex);
		clearCallbacks();
		ioBuffer = _ioBuffer;
		_ioBuffer.reset();
		_socket = 0;
	}

	if (ioBuffer)
	{
		bool needWaitSendEvent = false;
		ioBuffer->sendCloseSignal(needWaitSendEvent);
	}
}

void UDPFPNNProtocolProcessor::cleanExpiredCallbacks()
{
	std::map<uint32_t, BasicAnswerCallback*> currMap;
	int64_t current = slack_real_msec();

	{
		std::unique_lock<std::mutex> lck(_mutex);

		for (auto& pp: _callbackMap)
		{
			if (pp.second->expiredTime() <= current)
				currMap[pp.first] = pp.second;
		}

		for (auto& pp: currMap)
			_callbackMap.erase(pp.first);
	}

	for (auto& pp: currMap)
		ClientCenter::runCallback(nullptr, FPNN_EC_CORE_TIMEOUT, pp.second);
}

void UDPFPNNProtocolProcessor::sendData(std::string* data, int64_t msec_timeout)
{
	std::shared_ptr<UDPIOBuffer> ioBuffer;
	{
		std::unique_lock<std::mutex> lck(_mutex);
		ioBuffer = _ioBuffer;
	}

	if (ioBuffer)
	{
		if (msec_timeout == 0)
			msec_timeout = FPNN_DEFAULT_QUEST_TIMEOUT * 1000;

		bool needWaitSendEvent = false, blockByFlowControl = false;
		ioBuffer->sendData(needWaitSendEvent, blockByFlowControl, data, slack_real_msec() + msec_timeout, false);
	}
}

void UDPFPNNProtocolProcessor::sendCacheData()
{
	std::shared_ptr<UDPIOBuffer> ioBuffer;
	{
		std::unique_lock<std::mutex> lck(_mutex);
		ioBuffer = _ioBuffer;
	}

	if (ioBuffer)
	{
		bool needWaitSendEvent = false, blockByFlowControl = false;
		ioBuffer->sendCachedData(needWaitSendEvent, blockByFlowControl);
	}
}

bool UDPFPNNProtocolProcessor::sendQuestWithBasicAnswerCallback(FPQuestPtr quest, BasicAnswerCallback* callback, int msec_timeout)
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

	if (msec_timeout == 0)
		msec_timeout = FPNN_DEFAULT_QUEST_TIMEOUT * 1000;

	if (callback)
	{
		callback->updateExpiredTime(slack_real_msec() + msec_timeout);

		std::unique_lock<std::mutex> lck(_mutex);
		_callbackMap[seqNum] = callback;
	}

	std::shared_ptr<UDPIOBuffer> ioBuffer;
	{
		std::unique_lock<std::mutex> lck(_mutex);
		ioBuffer = _ioBuffer;
	}

	if (ioBuffer)
	{
		Config::ClientQuestLog(quest, _ip, _port);

		bool needWaitSendEvent = false, blockByFlowControl = false;
		ioBuffer->sendData(needWaitSendEvent, blockByFlowControl, raw, slack_real_msec() + msec_timeout, false);
		return true;
	}
	else
	{
		std::unique_lock<std::mutex> lck(_mutex);
		if (_callbackMap.find(seqNum) != _callbackMap.end())
		{
			_callbackMap.erase(seqNum);
			return false;
		}
		else
		{
			//-- Callback is token when ioBuffer->sendData() for closing or other rare cases.
			return true;
		}
	}
}

FPAnswerPtr UDPFPNNProtocolProcessor::sendQuest(FPQuestPtr quest, int timeout)
{
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

bool UDPFPNNProtocolProcessor::sendQuest(FPQuestPtr quest, AnswerCallback* callback, int timeout)
{
	return sendQuestWithBasicAnswerCallback(quest, callback, timeout * 1000);
}
bool UDPFPNNProtocolProcessor::sendQuest(FPQuestPtr quest, std::function<void (FPAnswerPtr answer, int errorCode)> task, int timeout)
{
	BasicAnswerCallback* t = new FunctionAnswerCallback(std::move(task));
	if (sendQuestWithBasicAnswerCallback(quest, t, timeout * 1000))
		return true;
	else
	{
		delete t;
		return false;
	}
}