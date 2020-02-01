#include "FPLog.h"
#include "Config.h"
#include "TCPEpollServer.h"
#include "UDPEpollServer.h"
#include "ServerController.h"
#include "UDPServerMasterProcessor.h"

using namespace fpnn;

void UDPServerMasterProcessor::prepare()
{
	_methodMap = new HashMap<std::string, MethodFunc>(4);
	_methodMap->insert("*status", &UDPServerMasterProcessor::status);
	_methodMap->insert("*infos", &UDPServerMasterProcessor::infos);
	_methodMap->insert("*tune", &UDPServerMasterProcessor::tune);
}

FPAnswerPtr UDPServerMasterProcessor::status(const FPReaderPtr, const FPQuestPtr quest, const ConnectionInfo&)
{
	return FPAWriter(1, quest)("status", _questProcessor->status());
}
FPAnswerPtr UDPServerMasterProcessor::infos(const FPReaderPtr, const FPQuestPtr quest, const ConnectionInfo&)
{
	return FPAWriter(quest)(ServerController::infos());
}

FPAnswerPtr UDPServerMasterProcessor::tune(const FPReaderPtr args, const FPQuestPtr quest, const ConnectionInfo& ci)
{
	std::string key = args->wantString("key");
	std::string value = args->wantString("value");
	LOG_INFO("TUNE something (UDP), %s=>%s", key.c_str(), value.c_str());

	TCPServerPtr tcpServer = TCPEpollServer::instance();
	if (!tcpServer || tcpServer->encrpytionEnabled() == false)
	{
		if (ServerController::tune(key, value))
			return FPAWriter(1, quest)("return", true);
	}

	try
	{
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
	catch (const std::exception& ex)
	{
		LOG_ERROR("Error in tune: %s", ex.what());
	}
	catch (...)
	{   
		LOG_ERROR("Unknown Error when quest user tune");
	}
	
	return FPAWriter(1, quest)("return", true);
}

void UDPServerMasterProcessor::dealQuest(UDPRequestPackage * requestPackage)
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
		catch (const std::exception& ex)
		{
			LOG_ERROR("processQuest() Exception: %s. %s", ex.what(), requestPackage->_connectionInfo->str().c_str());
			if (quest->isTwoWay())
			{
				if (_questProcessor->getQuestAnsweredStatus() == false)
					answer = FpnnErrorAnswer(quest, FPNN_EC_CORE_UNKNOWN_ERROR, std::string(ex.what()) + ", " + requestPackage->_connectionInfo->str());
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
		catch (const std::exception& ex)
		{
			answer = FpnnErrorAnswer(quest, FPNN_EC_CORE_UNKNOWN_ERROR, std::string(ex.what()) + ", " + requestPackage->_connectionInfo->str());
			raw = answer->raw();
		}
		catch (...)
		{
			answer = FpnnErrorAnswer(quest, FPNN_EC_CORE_UNKNOWN_ERROR, std::string("exception while do answer raw, ") + requestPackage->_connectionInfo->str());
			raw = answer->raw();
		}

		Config::ServerAnswerAndSlowLog(quest, answer, requestPackage->_connectionInfo->ip, requestPackage->_connectionInfo->port);

		_server->sendData(requestPackage->_connectionInfo, raw);
	}

	delete requestPackage;
}

void UDPServerMasterProcessor::run(UDPRequestPackage * requestPackage)
{
	try
	{
		dealQuest(requestPackage);
		return;
	}
	catch (const FpnnError& ex){
		LOG_ERROR("Fatal error occurred when deal quest:(%d)%s. Connection will be closed by server. %s", ex.code(), ex.what(), requestPackage->_connectionInfo->str().c_str());
	}
	catch (const std::exception& ex)
	{
		LOG_ERROR("Fatal error occurred when deal quest: %s. Connection will be closed by server. %s", ex.what(), requestPackage->_connectionInfo->str().c_str());
	}
	catch (...)
	{
		LOG_ERROR("Fatal error occurred when deal quest. Connection will be closed by server. %s", requestPackage->_connectionInfo->str().c_str());
	}

	delete requestPackage;
}
