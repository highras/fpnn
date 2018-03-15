#include "Config.h"
#include "AdvanceAnswer.h"
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
	_concurrentSender->sendData(connInfo->socket, connInfo->token, raw);
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

	IAsyncAnswerPtr async(new AsyncAnswerImp(_concurrentSender, gtl_answerStatus->_connInfo, gtl_answerStatus->_quest));
	gtl_answerStatus->_answered = true;
	return async;
}
