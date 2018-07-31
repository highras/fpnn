#ifndef FPNN_Advance_Answer_H
#define FPNN_Advance_Answer_H

#include <exception>
#include "FPLog.h"
#include "IQuestProcessor.h"
#include "ConcurrentSenderInterface.h"
#include "FPWriter.h"
#include "Config.h"

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
		std::string _traceRaiser;
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
			if (_sent == false)
			{
				const char* defaultErrorInfo = "Answer is lost in normal logic. The error answer is sent for instead.";
				FPAnswerPtr errAnswer = FpnnErrorAnswer(_quest, FPNN_EC_CORE_UNKNOWN_ERROR, (_traceInfo.empty() ? defaultErrorInfo : _traceInfo) + ", " +  _traceRaiser);
				try
				{
					sendAnswer(errAnswer);
				}
				catch (...) {
					LOG_ERROR("AsyncAnswer send default answer failed. trace info: %s, trace raiser: %s", _traceInfo.c_str(), _traceRaiser.c_str());
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
				if (_concurrentSender)
					_concurrentSender->sendData(_connectionInfo->socket, _connectionInfo->token, raw);
				else
					_concurrentUDPSender->sendData(_connectionInfo, raw);
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

		virtual void cacheTraceInfo(const std::string& ex, const std::string& raiser = "")
		{
			_traceInfo = ex;
			_traceRaiser = raiser;
		}
		virtual void cacheTraceInfo(const char * ex, const char* raiser = "")
		{
			_traceInfo = ex;
			_traceRaiser = raiser;
		}
	};
}

#endif
