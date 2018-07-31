#include "Config.h"
#include "AdvanceAnswer.h"
#include "QuestSenderImp.h"
#include "IQuestProcessor.h"

using namespace fpnn;

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

	if (standardInterface(*connInfo))
		_concurrentSender->sendData(connInfo->socket, connInfo->token, raw);
	else
		_concurrentUDPSender->sendData(connInfo, raw);

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
	if (standardInterface(connectionInfo))
		return std::make_shared<TCPQuestSender>(_concurrentSender, connectionInfo, connectionInfo._mutex);
	else
	{
		if (gtl_answerStatus)
			return std::make_shared<UDPQuestSender>(_concurrentUDPSender, gtl_answerStatus->_connInfo);

		ConnectionInfoPtr ci(new ConnectionInfo(connectionInfo));
		return std::make_shared<UDPQuestSender>(_concurrentUDPSender, ci);
	}
}

FPAnswerPtr IQuestProcessor::sendQuest(const ConnectionInfo& connectionInfo, FPQuestPtr quest, int timeout)
{
	if (standardInterface(connectionInfo))
		return _concurrentSender->sendQuest(connectionInfo.socket, connectionInfo.token, connectionInfo._mutex, quest, timeout * 1000);
	else
	{
		if (gtl_answerStatus)
			return _concurrentUDPSender->sendQuest(gtl_answerStatus->_connInfo, quest, timeout * 1000);

		ConnectionInfoPtr ci(new ConnectionInfo(connectionInfo));
		return _concurrentUDPSender->sendQuest(ci, quest, timeout * 1000);
	}
}

bool IQuestProcessor::sendQuest(const ConnectionInfo& connectionInfo, FPQuestPtr quest, AnswerCallback* callback, int timeout)
{
	if (standardInterface(connectionInfo))
		return _concurrentSender->sendQuest(connectionInfo.socket, connectionInfo.token, quest, callback, timeout * 1000);
	else
	{
		if (gtl_answerStatus)
			return _concurrentUDPSender->sendQuest(gtl_answerStatus->_connInfo, quest, callback, timeout * 1000);

		ConnectionInfoPtr ci(new ConnectionInfo(connectionInfo));
		return _concurrentUDPSender->sendQuest(ci, quest, callback, timeout * 1000);
	}
}

bool IQuestProcessor::sendQuest(const ConnectionInfo& connectionInfo, FPQuestPtr quest, std::function<void (FPAnswerPtr answer, int errorCode)> task, int timeout)
{
	if (standardInterface(connectionInfo))
		return _concurrentSender->sendQuest(connectionInfo.socket, connectionInfo.token, quest, std::move(task), timeout * 1000);
	else
	{
		if (gtl_answerStatus)
			return _concurrentUDPSender->sendQuest(gtl_answerStatus->_connInfo, quest, std::move(task), timeout * 1000);

		ConnectionInfoPtr ci(new ConnectionInfo(connectionInfo));
		return _concurrentUDPSender->sendQuest(ci, quest, std::move(task), timeout * 1000);
	}
}
