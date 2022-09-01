#include "FPLog.h"
#include "../../../Decoder.h"
#include "../../../ClientEngine.h"
#include "FpnnTCPCodec.h"

using namespace fpnn;

int TCPFPNNProtocolProcessor::packageTotalLength()
{
	const char* header = (const char *)_buffer->header(FPMessage::_HeaderLength);

	if (FPMessage::isTCP(header))
		return (int)(sizeof(FPMessage::Header) + FPMessage::BodyLen(header));
	else
		return -1;
}

void TCPFPNNProtocolProcessor::processBuffer(uint8_t* buffer, int len)
{
	if (_curr < FPMessage::_HeaderLength)
	{
		int require = FPMessage::_HeaderLength - _curr;
		if (require > len)
		{
			_buffer->append(buffer, len);
			_curr += len;
			return;
		}
		else
		{
			_buffer->append(buffer, require);
			_curr += require;

			_total = packageTotalLength();
			if (require < len)
			{
				processBuffer(buffer + require, len - require);
				return;
			}
		}
	}
	else if (_curr < _total)
	{
		int require = _total - _curr;
		if (require > len) 
		{
			_buffer->append(buffer, len);
			_curr += len;
			return;
		}
		else
		{
			_buffer->append(buffer, require);
			_curr += require;

			processFPNNPackage();

			_curr = 0;
			_total = FPMessage::_HeaderLength;
			if (require < len)
			{
				processBuffer(buffer + require, len - require);
				return;
			}
		}
	}
	else
	{
		LOG_ERROR("Error logic triggered. Please tell swxlion to fix it.");
	}
}

void TCPFPNNProtocolProcessor::dealQuest(FPQuestPtr quest)
{
	FPAnswerPtr answer = NULL;
	_questProcessor->initAnswerStatus(_connectionInfo, quest);

	try
	{
		FPReaderPtr args(new FPReader(quest->payload()));
		answer = _questProcessor->processQuest(args, quest, *_connectionInfo);
	}
	catch (const FpnnError& ex)
	{
		LOG_ERROR("processQuest ERROR:(%d) %s, connection:%s", ex.code(), ex.what(), _connectionInfo->str().c_str());
		if (quest->isTwoWay())
		{
			if (_questProcessor->getQuestAnsweredStatus() == false)
				answer = FpnnErrorAnswer(quest, ex.code(), std::string(ex.what()) + ", " + _connectionInfo->str());
		}
	}
	catch (const std::exception& ex)
	{
		LOG_ERROR("processQuest ERROR: %s, connection:%s", ex.what(), _connectionInfo->str().c_str());
		if (quest->isTwoWay())
		{
			if (_questProcessor->getQuestAnsweredStatus() == false)
				answer = FpnnErrorAnswer(quest, FPNN_EC_CORE_UNKNOWN_ERROR, std::string(ex.what()) + ", " + _connectionInfo->str());
		}
	}
	catch (...){
		LOG_ERROR("Unknown error when calling processQuest() function. %s", _connectionInfo->str().c_str());
		if (quest->isTwoWay())
		{
			if (_questProcessor->getQuestAnsweredStatus() == false)
				answer = FpnnErrorAnswer(quest, FPNN_EC_CORE_UNKNOWN_ERROR, std::string("Unknown error when calling processQuest() function, ") + _connectionInfo->str());
		}
	}

	bool questAnswered = _questProcessor->finishAnswerStatus();
	if (quest->isTwoWay())
	{
		if (questAnswered)
		{
			if (answer)
			{
				LOG_ERROR("Double answered after an advance answer sent, or async answer generated. %s", _connectionInfo->str().c_str());
			}
			return;
		}
		else if (!answer)
			answer = FpnnErrorAnswer(quest, FPNN_EC_CORE_UNKNOWN_ERROR, std::string("Twoway quest lose an answer. ") + _connectionInfo->str());
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
			FPAnswerPtr errAnswer = FpnnErrorAnswer(quest, ex.code(), std::string(ex.what()) + ", " + _connectionInfo->str());
			raw = errAnswer->raw();
		}
		catch (const std::exception& ex)
		{
			FPAnswerPtr errAnswer = FpnnErrorAnswer(quest, FPNN_EC_CORE_UNKNOWN_ERROR, std::string(ex.what()) + ", " + _connectionInfo->str());
			raw = errAnswer->raw();
		}
		catch (...)
		{
			/**  close the connection is to complex, so, return a error answer. It alway success? */

			FPAnswerPtr errAnswer = FpnnErrorAnswer(quest, FPNN_EC_CORE_UNKNOWN_ERROR, std::string("exception while do answer raw, ") + _connectionInfo->str());
			raw = errAnswer->raw();
		}

		ClientEngine::nakedInstance()->sendData(_connectionInfo->socket, _connectionInfo->token, raw);
	}
}

void TCPFPNNProtocolProcessor::dealAnswer(FPAnswerPtr answer)
{
	Config::ClientAnswerLog(answer, _connectionInfo->ip, _connectionInfo->port);

	BasicAnswerCallback* callback = ClientCenter::takeCallback(_connectionInfo->socket, answer->seqNumLE());
	if (!callback)
	{
		LOG_WARN("Recv an invalid answer, seq: %u. Peer %s:%d, Info: %s", answer->seqNumLE(),
			_connectionInfo->ip.c_str(), _connectionInfo->port, answer->info().c_str());
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

void TCPFPNNProtocolProcessor::processFPNNPackage()
{
	const char *desc = "unknown";
	try
	{
		if (Decoder::isQuest(_buffer))
		{
			desc = "TCP quest";
			FPQuestPtr quest = Decoder::decodeQuest(_buffer);
			if (quest)
				dealQuest(quest);
		}
		else
		{
			desc = "TCP answer";
			FPAnswerPtr answer = Decoder::decodeAnswer(_buffer);
			if (answer)
				dealAnswer(answer);
		}
	}
	catch (const FpnnError& ex)
	{
		LOG_ERROR("Decode %s error. Connection will be closed by server. Code: %d, error: %s.", desc, ex.code(), ex.what());
	}
	catch (...)
	{
		LOG_ERROR("Decode %s error. Connection will be closed by server.", desc);
	}

	delete _buffer;
	_buffer = new ChainBuffer(_chunkSize);
}

void TCPFPNNProtocolProcessor::process(ConnectionInfoPtr connectionInfo, const std::list<ReceivedRawData*>& dataList)
{
	_connectionInfo = connectionInfo;

	for (auto data: dataList)
		processBuffer(data->data, data->len);
}

