#include <sys/time.h>
#include <sys/resource.h>
#include "FPLog.h"
#include "TCPEpollServer.h"
#include "ClientEngine.h"
#include "ServerMasterProcessor.h"
#include "TimeUtil.h"
#include "StringUtil.h"
#include "Config.h"
#include "Statistics.h"
#include "net.h"

using namespace fpnn;

/*===============================================================================
  Server Master Processor
=============================================================================== */
void ServerMasterProcessor::setServer(TCPEpollServer* server)
{
	_server = server;
}

void ServerMasterProcessor::prepare()
{
	_methodMap = new HashMap<std::string, MethodFunc>(4);
	_methodMap->insert("*status", &ServerMasterProcessor::status);
	_methodMap->insert("*infos", &ServerMasterProcessor::infos);
	_methodMap->insert("*tune", &ServerMasterProcessor::tune);
}

FPAnswerPtr ServerMasterProcessor::status(const FPReaderPtr, const FPQuestPtr quest, const ConnectionInfo&)
{
	return FPAWriter(1, quest)("status", _questProcessor->status());
}
void ServerMasterProcessor::serverInfos(std::stringstream& ss, bool show_interface_stat)
{
	std::shared_ptr<std::string> serverBaseInfos;
	{
		std::unique_lock<std::mutex> lck(_mutex);
		serverBaseInfos = _serverBaseInfos;	
	}

	if (serverBaseInfos == nullptr)
	{
		std::stringstream s;
		s<<"{";

		//-- sBase
		s<<"\"base\":{";
		s<<"\"name\":"<<StringUtil::escapseString(_server->sName())<<",";
		s<<"\"startTime\":"<<_server->started()<<",";
		s<<"\"compileTime\":"<<_server->compiled()<<",";
		s<<"\"startTimeStr\":"<<StringUtil::escapseString(TimeUtil::getDateTime(_server->started()))<<",";
		s<<"\"compileTimeStr\":"<<StringUtil::escapseString(TimeUtil::getDateTime(_server->compiled()))<<",";
		s<<"\"frameworkVersion\":"<<StringUtil::escapseString(_server->version())<<",";/*record history of core*/
		s<<"\"listenBacklog\":"<<_server->backlog()<<",",
		s<<"\"listenIP\":"<<StringUtil::escapseString(_server->ip())<<",";
		s<<"\"listenPort\":"<<_server->port()<<",";
		s<<"\"iobufferChunkSize\":"<<_server->ioBufferChunkSize()<<",";
		s<<"\"maxEvent\":"<<_server->maxEvent()<<",";
		s<<"\"maxConnection\":"<<_server->maxConnectionLimitation()<<",";
		s<<"\"questTimeout\":"<<_server->getQuestTimeout()<<",";
		s<<"\"idleTimeout\":"<<_server->getIdleTimeout()<<",";
		s<<std::boolalpha<<"\"presetSignal\":"<<Config::_server_preset_signals<<",";
		s<<std::boolalpha<<"\"httpSupported\":"<<Config::_server_http_supported<<","; //may be tuned
		s<<std::boolalpha<<"\"stat\":"<<Config::_server_stat<<"},"; //may be tuned

		//-- sProto
		s<<"\"proto\":{";
		s<<"\"msgpackVersion\":"<<StringUtil::escapseString(msgpack_version())<<",";
		s<<"\"rapidjsonVersion\":"<<StringUtil::escapseString(RAPIDJSON_VERSION_STRING)<<",";
		s<<"\"currentVersion\":"<<(int32_t)FPMessage::currentVersion()<<",";
		s<<"\"supportedVersion\":"<<(int32_t)FPMessage::supportedVersion()<<"},";

		//-- sSystem
		s<<"\"system\":{";
		struct rlimit r;
		getrlimit(RLIMIT_NOFILE,&r);
		s<<"\"noFile\":"<<r.rlim_cur<<",";
		getrlimit(RLIMIT_CORE,&r);
		s<<"\"coreSize\":"<<r.rlim_cur<<",";
		getrlimit(RLIMIT_STACK,&r);
		s<<"\"stackSize\":"<<r.rlim_cur<<"},";

		//-- sLog
		s<<"\"log\":{";
		s<<"\"level\":"<<StringUtil::escapseString(FPLog::getLevel())<<",";
		s<<std::boolalpha<<"\"quest\":"<<Config::_log_server_quest<<",";
		s<<std::boolalpha<<"\"answer\":"<<Config::_log_server_answer<<",";
		s<<"\"slow\":"<<Config::_log_server_slow<<",";
		s<<std::boolalpha<<"\"statusInfos\":"<<_server->getStatusInfosLogStatus()<<",";
		s<<"\"statusInfosInterval\":"<<_server->getStatusInfosLogIntervalSeconds()<<"},";


		std::string cache = s.str();

		std::unique_lock<std::mutex> lck(_mutex);
		_serverBaseInfos.reset(new std::string());
		_serverBaseInfos->swap(cache);
		serverBaseInfos = _serverBaseInfos;
	}

	ss<<(*serverBaseInfos);

	if (show_interface_stat)
		ss<<"\"stat\":"<<Statistics::str()<<",";

	ss<<"\"status\":{";
	ss<<"\"currentConnections\":"<<_server->currentConnections()<<"},";

	ss<<"\"thread\":{";
	ss<<"\"workThreadStatus\":"<<_server->workerPoolStatus()<<",";
	ss<<"\"ioThreadStatus\":"<<GlobalIOPool::nakedInstance()->ioPoolStatus()<<",";
	ss<<"\"duplexThreadStatus\":"<<_server->answerCallbackPoolStatus()<<"}";

	ss<<"}";
}
void ServerMasterProcessor::clientInfos(std::stringstream& ss)
{
	std::shared_ptr<std::string> clientBaseInfos;
	{
		std::unique_lock<std::mutex> lck(_mutex);
		clientBaseInfos = _clientBaseInfos;	
	}
	
	if (clientBaseInfos == nullptr)
	{
		std::stringstream s;
		s<<"{";

		//-- cBase
		s<<"\"base\":{";
		s<<"\"questTimeout\":"<<ClientEngine::getQuestTimeout();
		s<<"},";

		//-- cLog
		s<<"\"log\":{";
		s<<std::boolalpha<<"\"quest\":"<<Config::_log_client_quest<<",";
		s<<std::boolalpha<<"\"answer\":"<<Config::_log_client_answer<<",";
		s<<"\"slow\":"<<Config::_log_client_slow<<"},";

		std::string cache = s.str();
		std::unique_lock<std::mutex> lck(_mutex);
		_clientBaseInfos.reset(new std::string());
		_clientBaseInfos->swap(cache);
		clientBaseInfos = _clientBaseInfos;
	}

	ss<<(*clientBaseInfos);

	ss<<"\"thread\":{";
	ss<<"\"workThreadStatus\":"<<ClientEngine::answerCallbackPoolStatus()<<",";
	ss<<"\"ioThreadStatus\":"<<GlobalIOPool::nakedInstance()->ioPoolStatus()<<",";
	ss<<"\"duplexThreadStatus\":"<<ClientEngine::questProcessPoolStatus()<<"}";

	ss<<"}"; 
}
std::string ServerMasterProcessor::appInfos()
{
	try
	{
		return _questProcessor->infos();
	}
	catch (const FpnnError& ex){
		LOG_ERROR("Error in infos:(%d)%s", ex.code(), ex.what());
	}
	catch (...)
	{
		LOG_ERROR("Unknown Error when quest user infos");
	}

	return std::string("{}");
}
FPAnswerPtr ServerMasterProcessor::infos(const FPReaderPtr, const FPQuestPtr quest, const ConnectionInfo&)
{
	std::stringstream ss;
	{
		std::unique_lock<std::mutex> lck(_mutex);
		if (_infosHeader.empty())
		{
			std::stringstream s;
			s<<"{";
			s<<"\"FPNN.status\":{";
			s<<"\"server\":";
			_infosHeader = s.str();
		}
		ss<<_infosHeader;
	}

	serverInfos(ss, true);
	ss<<",";// end of server

	ss<<"\"client\":";
	clientInfos(ss);

	ss <<"},"; // end of fpnn.status

	ss<<"\"APP.status\":";
	ss<<appInfos();

	ss<<"}"; //end of ALL
	
	return FPAWriter(quest)(ss.str());
}
void ServerMasterProcessor::logStatus()
{
	{
		std::stringstream ss;
		serverInfos(ss, false);
		UXLOG("SVR.INFOS.BAISC", "%s", ss.str().c_str());
	}

	UXLOG("SVR.INFOS.STAT", "%s", Statistics::str().c_str());

	{
		std::stringstream ss;
		clientInfos(ss);
		UXLOG("SVR.INFOS.CLIENT", "%s", ss.str().c_str());
	}
	
	UXLOG("SVR.INFOS.APP", "%s", appInfos().c_str());
}
FPAnswerPtr ServerMasterProcessor::tune(const FPReaderPtr args, const FPQuestPtr quest, const ConnectionInfo& ci)
{
	if (_server->encrpytionEnabled() && !ci.isEncrypted())
	{
		LOG_ERROR("Server encryption enabled, but the tune quest is not encrypted. %s", ci.str().c_str());
		return FPAWriter::errorAnswer(quest, FPNN_EC_CORE_FORBIDDEN, "Forbidden Operation", "Server");
	}

	try
	{
		std::string key = args->wantString("key");
		std::string value = args->wantString("value");
		LOG_INFO("TUNE something, %s=>%s", key.c_str(), value.c_str());
		struct rlimit rlim;
		intmax_t n = atoi((value).c_str());
		if(key == "server.core.size"){
			rlim.rlim_cur = n;  
			rlim.rlim_max = n;  
			setrlimit(RLIMIT_CORE, &rlim);
		}    
		else if(key == "server.no.file"){
			rlim.rlim_cur = n;  
			rlim.rlim_max = n;  
			setrlimit(RLIMIT_NOFILE, &rlim);
		}   
		else if(key == "server.log.level"){
			std::string level = value;
			FPLog::setLevel(level);
		}  
		else if(key == "server.quest.log"){
			Config::_log_server_quest = n;
		}
		else if(key == "server.answer.log"){
			Config::_log_server_answer = n;
		}
		else if(key == "server.slow.log"){
			Config::_log_server_slow = n;
		}
		else if(key == "client.quest.log"){
			Config::_log_client_quest = n;
		}
		else if(key == "client.answer.log"){
			Config::_log_client_answer = n;
		}
		else if(key == "client.slow.log"){
			Config::_log_client_slow = n;
		}
		else if(key == "server.http.supported"){
			Config::_server_http_supported = n;
		}
		else if(key == "server.stat"){
			Config::_server_stat = n;
		}
		else if(key == "server.status.logStatusInfos"){
			if (n > 0)
				_server->setStatusInfosLogStatus(true);
			else
				_server->setStatusInfosLogStatus(strcasecmp("true", value.c_str()) == 0);
		}
		else if(key == "server.status.logStatusInterval"){
			_server->setStatusInfosLogIntervalSeconds(n);
		}
		else if(key == "server.security.ip.whiteList.enable"){
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

	{
		std::unique_lock<std::mutex> lck(_mutex);
		_serverBaseInfos.reset();
		_clientBaseInfos.reset();
	}
	return FPAWriter(1, quest)("return", true);
}

void ServerMasterProcessor::dealQuest(RequestPackage * requestPackage)
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
				answer = FPAWriter::errorAnswer(quest, FPNN_EC_CORE_FORBIDDEN, "Can not call internal method from external", requestPackage->_connectionInfo->str().c_str());
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
					answer = FPAWriter::errorAnswer(quest, ex.code(), ex.what(), requestPackage->_connectionInfo->str().c_str());
			}
		}
		catch (...)
		{
			LOG_ERROR("Unknow error when calling processQuest() function. %s", requestPackage->_connectionInfo->str().c_str());
			if (quest->isTwoWay())
			{
				if (_questProcessor->getQuestAnsweredStatus() == false)
					answer = FPAWriter::errorAnswer(quest, FPNN_EC_CORE_UNKNOWN_ERROR, "Unknow error when calling processQuest() function.", requestPackage->_connectionInfo->str().c_str());
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
			answer = FPAWriter::errorAnswer(quest, FPNN_EC_CORE_UNKNOWN_ERROR, "Twoway quest lose an answer.", requestPackage->_connectionInfo->str().c_str());
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
			answer = FPAWriter::errorAnswer(quest, ex.code(), ex.what(), requestPackage->_connectionInfo->str().c_str());
			raw = answer->raw();
		}
		catch (...)
		{
			/**  close the connection is to complex, so, return a error answer. It alway success? */

			answer = FPAWriter::errorAnswer(quest, FPNN_EC_CORE_UNKNOWN_ERROR, "exception while do answer raw", requestPackage->_connectionInfo->str().c_str());
			raw = answer->raw();
		}

		Config::ServerAnswerAndSlowLog(quest, answer, requestPackage->_connectionInfo->ip, requestPackage->_connectionInfo->port);

		_server->sendData(requestPackage->_connectionInfo->socket, requestPackage->_connectionInfo->token, raw);
	}

	delete requestPackage;
}
void ServerMasterProcessor::run(RequestPackage * requestPackage)
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
