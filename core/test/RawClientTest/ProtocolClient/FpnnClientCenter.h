#ifndef FPNN_Callbacks_Center_h
#define FPNN_Callbacks_Center_h

#include <mutex>
#include <atomic>
#include <memory>
#include <thread>
#include <unordered_map>
#include <stdint.h>
#include "../../../AnswerCallbacks.h"
#include "FpnnUDPCodec.h"

namespace fpnn
{
	class ClientCenter;
	typedef std::shared_ptr<ClientCenter> ClientCenterPtr;

	class ClientCenter
	{
		static std::mutex _mutex;
		
		std::atomic<bool> _running;
		int64_t _timeoutQuest;
		std::thread _loopThread;
		std::unordered_map<int, std::unordered_map<uint32_t, BasicAnswerCallback*>> _callbackMap;
		std::unordered_set<UDPFPNNProtocolProcessorPtr> _udpProcessors;

		ClientCenter();
		void loopThread();
		static void cleanCallbacks(std::unordered_map<uint32_t, BasicAnswerCallback*>& callbackMap);

	public:
		virtual ~ClientCenter();
		static ClientCenterPtr instance();

		inline static void setQuestTimeout(int64_t seconds)
		{
			instance()->_timeoutQuest = seconds * 1000;
		}
		inline static int64_t getQuestTimeout()
		{
			return instance()->_timeoutQuest / 1000;
		}

		//-- For TCP
		// static void registerConnection(int socket);	Nothing need to do.
		static void unregisterConnection(int socket);

		static BasicAnswerCallback* takeCallback(int socket, uint32_t seqNum);
		static void registerCallback(int socket, uint32_t seqNum, BasicAnswerCallback* callback);
		static void runCallback(FPAnswerPtr answer, int errorCode, BasicAnswerCallback* callback);

		//-- For UDP
		static void registerUDPProcessor(UDPFPNNProtocolProcessorPtr processor);
		static void unregisterUDPProcessor(UDPFPNNProtocolProcessorPtr processor);
	};
}

#endif
