#ifndef FPNN_FPNN_TCP_Codec_h
#define FPNN_FPNN_TCP_Codec_h

#include "ChainBuffer.h"
#include "../../../IQuestProcessor.h"
#include "../../../RawTransmission/RawTransmissionCommon.h"
#include "FpnnClientCenter.h"

namespace fpnn
{
	class TCPFPNNProtocolProcessor: public IRawDataBatchProcessor
	{
		int _curr;
		int _total;
		int _chunkSize;
		std::mutex _mutex;
		ChainBuffer* _buffer;
		IQuestProcessorPtr _questProcessor;
		ConnectionInfoPtr _connectionInfo;

		int packageTotalLength();
		void processBuffer(uint8_t* buffer, int len);
		void dealQuest(FPQuestPtr quest);
		void dealAnswer(FPAnswerPtr answer);
		void processFPNNPackage();

	public:
		TCPFPNNProtocolProcessor(): _curr(0), _total(FPMessage::_HeaderLength), _chunkSize(256)
		{
			_buffer = new ChainBuffer(_chunkSize);
		}
		virtual ~TCPFPNNProtocolProcessor()
		{
			if (_buffer)
				delete _buffer;
		}

		virtual void process(ConnectionInfoPtr connectionInfo, const std::list<ReceivedRawData*>& dataList);
		inline void setQuestProcessor(IQuestProcessorPtr processor)
		{
			_questProcessor = processor;
		}
	};
}

#endif