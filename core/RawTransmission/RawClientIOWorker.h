#ifndef FPNN_Raw_Client_IO_Worker_H
#define FPNN_Raw_Client_IO_Worker_H

#include "ParamTemplateThreadPoolArray.h"
#include "../Config.h"
#include "../IOWorker.h"
#include "RawTransmissionCommon.h"

namespace fpnn
{
	class RawClient;
	typedef std::shared_ptr<RawClient> RawClientPtr;

	class RawClientBasicConnection;
	typedef std::shared_ptr<RawClientBasicConnection> RawClientBasicConnectionPtr;
	
	//=================================================================//
	//- RawClientBasicConnection
	//=================================================================//
	class RawClientBasicConnection: public BasicConnection
	{
	protected:
		std::mutex _mutex;
		std::weak_ptr<RawClient> _client;

		RawDataReceiver* _receiver;
		RawDataSender* _sender;

	private:
		void dealReceivedRawData(std::list<ReceivedRawData*>& rawDataList);

	public:
		virtual bool waitForAllEvents();
		virtual enum ConnectionType connectionType() { return BasicConnection::RawClientConnectionType; }
		RawClientPtr client() { return _client.lock(); }

		RawClientBasicConnection(RawClientPtr client, ConnectionInfoPtr connectionInfo):
			BasicConnection(connectionInfo), _client(client), _receiver(NULL), _sender(NULL)
		{
			_connectionInfo->token = (uint64_t)this;	//-- if using Virtual Derive, must do this. Else, this is just redo the action in base class.
		}

		virtual ~RawClientBasicConnection()
		{
			if (_receiver) delete _receiver;
			if (_sender) delete _sender;
		}

		bool recv();
		virtual int send(bool& needWaitSendEvent, std::string* data = NULL);
	};

	//=================================================================//
	//- RawClientIOWorker
	//=================================================================//
	class RawClientIOWorker: public ParamTemplateThreadPool<RawClientBasicConnection *>::IProcessor
	{
		void closeConnection(RawClientBasicConnection * connection);

	public:
		RawClientIOWorker() {}
		virtual ~RawClientIOWorker() {}
		virtual void run(RawClientBasicConnection * connection);
	};

	//=================================================================//
	//- RawTCPClientConnection
	//=================================================================//
	class RawTCPClientConnection: public RawClientBasicConnection
	{
	public:
		RawTCPClientConnection(RawClientPtr client, uint32_t receivingChunkSize, ConnectionInfoPtr connectionInfo):
			RawClientBasicConnection(client, connectionInfo)
		{
			_connectionInfo->token = (uint64_t)this;	//-- if using Virtual Derive, must do this. Else, this is just redo the action in base class.

			_receiver = new TCPRawDataReceiver(receivingChunkSize);
			_receiver->setMutex(&_mutex);

			_sender = new TCPRawDataSender();
			_sender->setMutex(&_mutex);
		}
		virtual ~RawTCPClientConnection() {}
	};

	//=================================================================//
	//- RawUDPClientConnection
	//=================================================================//
	class RawUDPClientConnection: public RawClientBasicConnection
	{
	public:
		RawUDPClientConnection(RawClientPtr client, ConnectionInfoPtr connectionInfo):
			RawClientBasicConnection(client, connectionInfo)
		{
			_connectionInfo->token = (uint64_t)this;	//-- if using Virtual Derive, must do this. Else, this is just redo the action in base class.

			_receiver = new UDPRawDataReceiver(FPNN_UDP_MAX_DATA_LENGTH);
			_receiver->setMutex(&_mutex);
			
			_sender = new UDPRawDataSender();
			_sender->setMutex(&_mutex);
		}

		virtual ~RawUDPClientConnection() {}
	};
}

#endif