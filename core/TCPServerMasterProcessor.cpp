#include <sys/time.h>
#include <sys/resource.h>
#include "FPLog.h"
#include "TCPEpollServer.h"
#include "ClientEngine.h"
#include "ServerController.h"
#include "TCPServerMasterProcessor.h"
#include "TimeUtil.h"
#include "StringUtil.h"
#include "Config.h"
#include "Statistics.h"
#include "net.h"

using namespace fpnn;

/*===============================================================================
  Server Master Processor
=============================================================================== */
void TCPServerMasterProcessor::prepare()
{
	_methodMap = new HashMap<std::string, MethodFunc>(4);
	_methodMap->insert("*status", &TCPServerMasterProcessor::status);
	_methodMap->insert("*infos", &TCPServerMasterProcessor::infos);
	_methodMap->insert("*tune", &TCPServerMasterProcessor::tune);
}

FPAnswerPtr TCPServerMasterProcessor::status(const FPReaderPtr, const FPQuestPtr quest, const ConnectionInfo&)
{
	return FPAWriter(1, quest)("status", _questProcessor->status());
}

FPAnswerPtr TCPServerMasterProcessor::infos(const FPReaderPtr, const FPQuestPtr quest, const ConnectionInfo&)
{
	return FPAWriter(quest)(ServerController::infos());
}

FPAnswerPtr TCPServerMasterProcessor::tune(const FPReaderPtr args, const FPQuestPtr quest, const ConnectionInfo& ci)
{
	if (_server->encrpytionEnabled() && !ci.isEncrypted())
	{
		LOG_ERROR("Server encryption enabled, but the tune quest is not encrypted. %s", ci.str().c_str());
		return FpnnErrorAnswer(quest, FPNN_EC_CORE_FORBIDDEN, "Forbidden Operation");
	}

	std::string key = args->wantString("key");
	std::string value = args->wantString("value");
	LOG_INFO("TUNE something (TCP), %s=>%s", key.c_str(), value.c_str());

	try
	{
		if (ServerController::tune(key, value))
			return FPAWriter(1, quest)("return", true);

		if(key == "server.security.ip.whiteList.enable"){
			_server->enableIPWhiteList(strcasecmp("true", value.c_str()) == 0);
		}
		else if(key == "server.security.ip.whiteList.addIP"){
			_server->addIPToWhiteList(value);
		}
		else if(key == "server.security.ip.whiteList.removeIP"){
			_server->removeIPFromWhiteList(value);
		}
		else{
			_questProcessor->tune(key, value);
		}
	}
	catch (const FpnnError& ex){
		LOG_ERROR("Error in tune:(%d)%s", ex.code(), ex.what());
	}   
	catch (...)
	{   
		LOG_ERROR("Unknown Error when quest user tune");
	}

	return FPAWriter(1, quest)("return", true);
}

void TCPServerMasterProcessor::dealQuest(RequestPackage * requestPackage)
{
	FPAnswerPtr answer = NULL;
	bool sysCommend = false;

	FPQuestPtr quest = requestPackage->_quest;
	const std::string& method = requestPackage->_quest->method();

	FPReaderPtr args(new FPReader(quest->payload()));

	Config::ServerQuestLog(quest, requestPackage->_connectionInfo->ip, requestPackage->_connectionInfo->port);

	_questProcessor->initAnswerStatus(requestPackage->_connectionInfo, quest);

	if (method.data()[0] == '*')
	{
		HashMap<std::string, MethodFunc>::node_type* node = _methodMap->find(method);
		if (node)
		{
			sysCommend = true;
			if (requestPackage->_connectionInfo->isPrivateIP())
				answer = (this->*(node->data))(args, quest, *(requestPackage->_connectionInfo));
			else
				answer = FpnnErrorAnswer(quest, FPNN_EC_CORE_FORBIDDEN, std::string("Can not call internal method from external, ") + requestPackage->_connectionInfo->str());
		}
	}

	if (!sysCommend)
	{
		try
		{
			answer = _questProcessor->processQuest(args, quest, *(requestPackage->_connectionInfo));
		}
		catch (const FpnnError& ex){
			LOG_ERROR("processQuest() Exception:(%d)%s. %s", ex.code(), ex.what(), requestPackage->_connectionInfo->str().c_str());
			if (quest->isTwoWay())
			{
				if (_questProcessor->getQuestAnsweredStatus() == false)
					answer = FpnnErrorAnswer(quest, ex.code(), std::string(ex.what()) + ", " + requestPackage->_connectionInfo->str());
			}
		}
		catch (...)
		{
			LOG_ERROR("Unknow error when calling processQuest() function. %s", requestPackage->_connectionInfo->str().c_str());
			if (quest->isTwoWay())
			{
				if (_questProcessor->getQuestAnsweredStatus() == false)
					answer = FpnnErrorAnswer(quest, FPNN_EC_CORE_UNKNOWN_ERROR, std::string("Unknow error when calling processQuest() function. ") + requestPackage->_connectionInfo->str());
			}
		}
	}

	bool questAnswered = _questProcessor->finishAnswerStatus();
	if (quest->isTwoWay())
	{
		if (questAnswered)
		{
			if (answer)
			{
				LOG_ERROR("Double answered after an advance answer sent, or async answer generated. %s", requestPackage->_connectionInfo->str().c_str());
			}

			delete requestPackage;
			return;
		}
		else if (!answer)
			answer = FpnnErrorAnswer(quest, FPNN_EC_CORE_UNKNOWN_ERROR, std::string("Twoway quest lose an answer. ") + requestPackage->_connectionInfo->str());
	}
	else if (answer)
	{
		LOG_ERROR("Oneway quest return an answer. %s", requestPackage->_connectionInfo->str().c_str());
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
			answer = FpnnErrorAnswer(quest, ex.code(), std::string(ex.what()) + ", " + requestPackage->_connectionInfo->str());
			raw = answer->raw();
		}
		catch (...)
		{
			/**  close the connection is to complex, so, return a error answer. It alway success? */

			answer = FpnnErrorAnswer(quest, FPNN_EC_CORE_UNKNOWN_ERROR, std::string("exception while do answer raw, ") + requestPackage->_connectionInfo->str());
			raw = answer->raw();
		}

		Config::ServerAnswerAndSlowLog(quest, answer, requestPackage->_connectionInfo->ip, requestPackage->_connectionInfo->port);

		_server->sendData(requestPackage->_connectionInfo->socket, requestPackage->_connectionInfo->token, raw);
	}

	delete requestPackage;
}
void TCPServerMasterProcessor::run(RequestPackage * requestPackage)
{
	if (requestPackage->_ioEvent == IOEventType::Recv)
	{
		try
		{
			dealQuest(requestPackage);
			return;
		}
		catch (const FpnnError& ex){
			LOG_ERROR("Fatal error occurred when deal quest:(%d)%s. Connection will be closed by server. %s", ex.code(), ex.what(), requestPackage->_connectionInfo->str().c_str());
		}
		catch (...)
		{
			LOG_ERROR("Fatal error occurred when deal quest. Connection will be closed by server. %s", requestPackage->_connectionInfo->str().c_str());
		}

		//exception processor
		TCPServerConnection* connection = _server->takeConnection(requestPackage->_connectionInfo.get());
		if (connection)
		{
			connection->exitEpoll();
			requestPackage->_quest = NULL;
			requestPackage->_connection = connection;
			requestPackage->_ioEvent = IOEventType::Error;

			run(requestPackage);
			return;
		}

		delete requestPackage;

		return;
	}

	else if (requestPackage->_ioEvent == IOEventType::Connected)
	{
		try{
			_questProcessor->connected(*(requestPackage->_connectionInfo.get()));
		}
		catch (const FpnnError& ex){
			LOG_ERROR("connected() error:(%d)%s. %s", ex.code(), ex.what(), requestPackage->_connectionInfo->str().c_str());
		}
		catch (...)
		{
			LOG_ERROR("Unknown error when calling connected() function. %s", requestPackage->_connectionInfo->str().c_str());
		}
		if (!requestPackage->_connection->joinEpoll())
		{
			LOG_ERROR("Add socket into epoll failed. Connection will be closed by server-end. %s", requestPackage->_connectionInfo->str().c_str());
			_server->takeConnection(requestPackage->_connectionInfo.get());

			requestPackage->_ioEvent = IOEventType::Error;
			run(requestPackage);
			return;
		}
		delete requestPackage;
		return;
	}

	else
	{
		bool closeByError = (requestPackage->_ioEvent != IOEventType::Closed);	//-- current ONLY 4 types used: Recv, Connected, Closed, Error.
		const char* paramDesc = closeByError ? "true" : "false";
		try{
			_questProcessor->connectionWillClose(*(requestPackage->_connectionInfo.get()), closeByError);
		}
		catch (const FpnnError& ex){
			LOG_ERROR("connectionWillClose(..., %s) error:(%d)%s. %s", paramDesc, ex.code(), ex.what(), requestPackage->_connectionInfo->str().c_str());
		}
		catch (...)
		{
			LOG_ERROR("Unknown error when calling connectionWillClose(..., %s) function. %s", paramDesc, requestPackage->_connectionInfo->str().c_str());
		}
		TCPServerConnectionPtr autoRelease(requestPackage->_connection);
		_server->decreaseConnectionCount();
		_reclaimer->reclaim(autoRelease);
		delete requestPackage;
		return;
	}
}
