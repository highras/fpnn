#include "Config.h"
#include "AdvanceAnswer.h"
#include "QuestSenderImp.h"
#include "IQuestProcessor.h"

using namespace fpnn;

std::atomic<uint64_t> ConnectionInfo::uniqueIdBase(0);

struct AnswerStatus
{
	bool _answered;
	FPQuestPtr _quest;
	ConnectionInfoPtr _connInfo;

	AnswerStatus(ConnectionInfoPtr connInfo, FPQuestPtr quest): _answered(false), _quest(quest), _connInfo(connInfo) {}
};

static thread_local std::unique_ptr<AnswerStatus> gtl_answerStatus;

void IQuestProcessor::initAnswerStatus(ConnectionInfoPtr connInfo, FPQuestPtr quest)
{
	gtl_answerStatus.reset(new AnswerStatus(connInfo, quest));
}

bool IQuestProcessor::getQuestAnsweredStatus()
{
	return gtl_answerStatus->_answered;
}

bool IQuestProcessor::finishAnswerStatus()
{
	bool status = gtl_answerStatus->_answered;
	gtl_answerStatus = nullptr;
	return status;
}

bool IQuestProcessor::sendAnswer(FPAnswerPtr answer)
{
	if (!answer || !gtl_answerStatus)
		return false;

	if (gtl_answerStatus->_answered)
		return false;

	if (!gtl_answerStatus->_quest->isTwoWay())
		return false;

	std::string* raw = answer->raw();

	ConnectionInfoPtr connInfo = gtl_answerStatus->_connInfo;

	if (connInfo->isTCP())
		_concurrentSender->sendData(connInfo->socket, connInfo->token, raw);
	else if (connInfo->isServerConnection())
	{
		//int64_t expiredMS = UDPEpollServer::instance()->getQuestTimeout() * 1000;
		_concurrentUDPSender->sendData(connInfo->socket, connInfo->token, raw, 0/*slack_real_msec() + expiredMS*/, false);
	}
	else
	{
		//int64_t expiredMS = ClientEngine::instance()->getQuestTimeout() * 1000;
		ClientEngine::instance()->sendUDPData(connInfo->socket, connInfo->token, raw, 0/*slack_real_msec() + expiredMS*/, false);
	}

	Config::ServerAnswerAndSlowLog(gtl_answerStatus->_quest, answer, connInfo->ip, connInfo->port);
	gtl_answerStatus->_answered = true;
	return true;
}

IAsyncAnswerPtr IQuestProcessor::genAsyncAnswer()
{
	if (!gtl_answerStatus)
		return nullptr;

	if (gtl_answerStatus->_answered)
		return nullptr;

	//-- 后续判断。确保业务没有判断也能正常执行。
	// if (gtl_answerStatus->_quest->isOneWay())
	//	return nullptr;

	IAsyncAnswerPtr async;

	if (standardInterface(*(gtl_answerStatus->_connInfo)))
		async.reset(new AsyncAnswerImp(_concurrentSender, gtl_answerStatus->_connInfo, gtl_answerStatus->_quest));
	else
		async.reset(new AsyncAnswerImp(_concurrentUDPSender, gtl_answerStatus->_connInfo, gtl_answerStatus->_quest));

	gtl_answerStatus->_answered = true;
	return async;
}

QuestSenderPtr IQuestProcessor::genQuestSender(const ConnectionInfo& connectionInfo)
{
	if (connectionInfo.isTCP())
		return std::make_shared<TCPQuestSender>(_concurrentSender, connectionInfo, connectionInfo._mutex);
	else
		return std::make_shared<UDPQuestSender>(_concurrentUDPSender, connectionInfo, connectionInfo._mutex);
}

FPAnswerPtr IQuestProcessor::sendQuest(FPQuestPtr quest, int timeout)
{
	return sendQuestEx(quest, quest->isOneWay(), timeout * 1000);
}

bool IQuestProcessor::sendQuest(FPQuestPtr quest, AnswerCallback* callback, int timeout)
{
	return sendQuestEx(quest, callback, quest->isOneWay(), timeout * 1000);
}

bool IQuestProcessor::sendQuest(FPQuestPtr quest, std::function<void (FPAnswerPtr answer, int errorCode)> task, int timeout)
{
	return sendQuestEx(quest, std::move(task), quest->isOneWay(), timeout * 1000);
}

FPAnswerPtr IQuestProcessor::sendQuestEx(FPQuestPtr quest, bool discardable, int timeoutMsec)
{
	if (!gtl_answerStatus)
	{
		if (quest->isTwoWay())
			return FPAWriter::errorAnswer(quest, FPNN_EC_CORE_FORBIDDEN, "Please call this method in the duplex thread.");
		else
			return nullptr;
	}

	ConnectionInfoPtr connInfo = gtl_answerStatus->_connInfo;

	if (connInfo->isTCP())
		return _concurrentSender->sendQuest(connInfo->socket, connInfo->token, connInfo->_mutex, quest, timeoutMsec);
	else
	{
		if (connInfo->isServerConnection())
		{
			return _concurrentUDPSender->sendQuest(connInfo->socket, connInfo->token, quest, timeoutMsec, discardable);
		}
		else
			return ClientEngine::instance()->sendQuest(connInfo->socket,
				connInfo->token, connInfo->_mutex, quest, timeoutMsec, discardable);
	}
}
bool IQuestProcessor::sendQuestEx(FPQuestPtr quest, AnswerCallback* callback, bool discardable, int timeoutMsec)
{
	if (!gtl_answerStatus)
		return false;

	ConnectionInfoPtr connInfo = gtl_answerStatus->_connInfo;

	if (connInfo->isTCP())
		return _concurrentSender->sendQuest(connInfo->socket, connInfo->token, quest, callback, timeoutMsec);
	else
	{
		if (connInfo->isServerConnection())
		{
			return _concurrentUDPSender->sendQuest(connInfo->socket, connInfo->token, quest, callback, timeoutMsec, discardable);
		}
		else
			return ClientEngine::instance()->sendQuest(connInfo->socket,
				connInfo->token, quest, callback, timeoutMsec, discardable);
	}
}
bool IQuestProcessor::sendQuestEx(FPQuestPtr quest, std::function<void (FPAnswerPtr answer, int errorCode)> task, bool discardable, int timeoutMsec)
{
	if (!gtl_answerStatus)
		return false;

	ConnectionInfoPtr connInfo = gtl_answerStatus->_connInfo;

	if (connInfo->isTCP())
		return _concurrentSender->sendQuest(connInfo->socket, connInfo->token, quest, std::move(task), timeoutMsec);
	else
	{
		if (connInfo->isServerConnection())
		{
			return _concurrentUDPSender->sendQuest(connInfo->socket, connInfo->token, quest, std::move(task), timeoutMsec, discardable);
		}
		else
			return ClientEngine::instance()->sendQuest(connInfo->socket,
				connInfo->token, quest, std::move(task), timeoutMsec, discardable);
	}
}

