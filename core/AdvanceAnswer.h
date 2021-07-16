#ifndef FPNN_Advance_Answer_H
#define FPNN_Advance_Answer_H

#include <exception>
#include "FPLog.h"
#include "IQuestProcessor.h"
#include "ConcurrentSenderInterface.h"
#include "FPWriter.h"
#include "Config.h"
#include "ClientEngine.h"
#include "UDPEpollServer.h"

namespace fpnn
{
	/*===============================================================================
	  Async Answer Implement
	=============================================================================== */
	class AsyncAnswerImp: public IAsyncAnswer
	{
		IConcurrentSender* _concurrentSender;
		IConcurrentUDPSender* _concurrentUDPSender;
		ConnectionInfoPtr _connectionInfo;
		FPQuestPtr _quest;
		std::string _traceInfo;
		bool _sent;

	public:
		AsyncAnswerImp(IConcurrentSender* concurrentSender, ConnectionInfoPtr connectionInfo, FPQuestPtr quest):
			_concurrentSender(concurrentSender), _concurrentUDPSender(NULL), _connectionInfo(connectionInfo),
			_quest(quest), _sent(false) {}

		AsyncAnswerImp(IConcurrentUDPSender* concurrentSender, ConnectionInfoPtr connectionInfo, FPQuestPtr quest):
			_concurrentSender(NULL), _concurrentUDPSender(concurrentSender), _connectionInfo(connectionInfo),
			_quest(quest), _sent(false) {}

		virtual ~AsyncAnswerImp()
		{
			if (_sent == false && _quest->isTwoWay())
			{
				const char* defaultErrorInfo = "Answer is lost in normal logic. The error answer is sent for instead.";
				FPAnswerPtr errAnswer = FpnnErrorAnswer(_quest, FPNN_EC_CORE_UNKNOWN_ERROR, (_traceInfo.empty() ? defaultErrorInfo : _traceInfo));
				try
				{
					sendAnswer(errAnswer);
				}
				catch (...) {
					LOG_ERROR("AsyncAnswer send default answer failed. trace info: %s", _traceInfo.c_str());
				}

			}
		}

		virtual const FPQuestPtr getQuest() { return _quest; }
		virtual bool sendAnswer(FPAnswerPtr answer)
		{
			if (_sent || !answer)
				return false;

			Config::ServerAnswerAndSlowLog(_quest, answer, _connectionInfo->ip, _connectionInfo->port);

			try
			{
				std::string* raw = answer->raw();
				if (_connectionInfo->isTCP())
					_concurrentSender->sendData(_connectionInfo->socket, _connectionInfo->token, raw);
				else if (_connectionInfo->isServerConnection())
				{
					int64_t expiredMS = UDPEpollServer::instance()->getQuestTimeout() * 1000 + slack_real_msec();
					_concurrentUDPSender->sendData(_connectionInfo->socket, _connectionInfo->token, raw, expiredMS, _quest->isOneWay());
				}
				else
				{
					int64_t expiredMS = ClientEngine::instance()->getQuestTimeout() * 1000 + slack_real_msec();
					ClientEngine::instance()->sendUDPData(_connectionInfo->socket, _connectionInfo->token, raw, expiredMS, _quest->isOneWay());
				}

				_sent = true;
				return true;
			}
			catch (const FpnnError& ex){
				//sendData will not throw any exception, so if run to here, raw throw an exception
				throw ex;
			}
			catch (...)
			{
				throw FPNN_ERROR_CODE_MSG(FpnnCoreError, FPNN_EC_CORE_UNKNOWN_ERROR, "Unknow error");
			}
			return false;

		}
		virtual bool isSent() { return _sent; }

		virtual void cacheTraceInfo(const std::string& ex)
		{
			_traceInfo = ex;
		}
		virtual void cacheTraceInfo(const char * ex)
		{
			_traceInfo = ex;
		}
	};
}

#endif
