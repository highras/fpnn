#ifndef FPNN_Raw_Client_Interface_H
#define FPNN_Raw_Client_Interface_H

#include <atomic>
#include <list>
#include <mutex>
#include <memory>
#include <condition_variable>
#include "../ClientEngine.h"
#include "../IQuestProcessor.h"
#include "RawTransmissionCommon.h"

namespace fpnn
{
	class RawClient;
	typedef std::shared_ptr<RawClient> RawClientPtr;

	class RawTCPClient;
	typedef std::shared_ptr<RawTCPClient> RawTCPClientPtr;

	class RawUDPClient;
	typedef std::shared_ptr<RawUDPClient> RawUDPClientPtr;

	class RawClient
	{
	protected:
		enum class ConnStatus
		{
			NoConnected,
			Connecting,
			Connected,
		};

	protected:
		std::mutex _mutex;
		std::condition_variable _condition;		//-- Only for connected.
		bool _isIPv4;
		std::atomic<bool> _connected;
		ConnStatus _connStatus;
		ClientEnginePtr _engine;

		IRawDataProcessorPtr _rawDataProcessor;
		IQuestProcessorPtr _questProcessor;
		ConnectionInfoPtr _connectionInfo;
		std::string _endpoint;

		TaskThreadPool* _rawDataProcessPool;
		bool _autoReconnect;

	private:
		void onErrorOrCloseEvent(BasicConnection* connection, bool error);

		void dealReceivedRawData_standard(ConnectionInfoPtr connectionInfo, std::list<ReceivedRawData*>& rawDataList);
		void dealReceivedRawData_batch(ConnectionInfoPtr connectionInfo, std::list<ReceivedRawData*>& rawDataList);

	public:
		RawClient(const std::string& host, int port, bool autoReconnect = true);
		virtual ~RawClient();
		/*===============================================================================
		  Call by framework.
		=============================================================================== */
		void connected(BasicConnection* connection);
		void willClose(BasicConnection* connection)		//-- must done in thread pool or other thread.
		{
			onErrorOrCloseEvent(connection, false);
		}
		void errorAndWillBeClosed(BasicConnection* connection)		//-- must done in thread pool or other thread.
		{
			onErrorOrCloseEvent(connection, true);
		}

		/** MUST delete all ReceivedRawData* pointers and clear the list. */
		void dealReceivedRawData(ConnectionInfoPtr connectionInfo, std::list<ReceivedRawData*>& rawDataList);
		/*===============================================================================
		  Call by anybody.
		=============================================================================== */
		inline bool connected() { return _connected; }
		inline const std::string& endpoint() { return _endpoint; }
		inline int socket()
		{
			std::unique_lock<std::mutex> lck(_mutex);
			return _connectionInfo->socket;
		}
		inline ConnectionInfoPtr connectionInfo()
		{
			std::unique_lock<std::mutex> lck(_mutex);
			return _connectionInfo;
		}
		/*===============================================================================
		  Call by Developer. Configure Function.
		=============================================================================== */
		inline void setQuestProcessor(IQuestProcessorPtr processor)
		{
			_questProcessor = processor;
			_questProcessor->setConcurrentSender(_engine.get());
		}
		inline void setRawDataProcessor(IRawDataProcessorPtr processor)
		{ 
			_rawDataProcessor = processor;
		}
		/*===============================================================================
		  Call by Developer.
		=============================================================================== */
		virtual bool connect() = 0;
		virtual void close();
		virtual bool reconnect();

		virtual bool sendData(std::string* data) = 0;

		static RawTCPClientPtr createRawTCPClient(const std::string& host, int port, bool autoReconnect = true);
		static RawTCPClientPtr createRawTCPClient(const std::string& endpoint, bool autoReconnect = true);

		static RawUDPClientPtr createRawUDPClient(const std::string& host, int port, bool autoReconnect = true);
		static RawUDPClientPtr createRawUDPClient(const std::string& endpoint, bool autoReconnect = true);
	};
}

#endif