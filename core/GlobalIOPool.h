#ifndef FPNN_Global_IO_Pool_H
#define FPNN_Global_IO_Pool_H

#include "ParamTemplateThreadPoolArray.h"
#include "ClientIOWorker.h"
#include "ServerIOWorker.h"
#include "FPLog.h"

namespace fpnn
{
	class TCPEpollIOWorker: public ParamTemplateThreadPool<TCPBasicConnection *>::IProcessor
	{
		std::shared_ptr<TCPClientIOWorker> _clientIOWorker;
		std::shared_ptr<TCPServerIOWorker> _serverIOWorker;

	public:
		virtual void run(TCPBasicConnection * connection)
		{
			if (connection->connectionType() == TCPBasicConnection::ServerConnectionType)
				_serverIOWorker->run((TCPServerConnection*)connection);
			else
				_clientIOWorker->run((TCPClientConnection*)connection);
		}

		/** All safe without locker.
		* Client IO Worker will be set by ClientEngine starting. ClientEngine is unique in global.
			If ClientEngine is not started, no client connection can come in.
		* Server IO Worker will be set by TCPEpollServer starting. TCPEpollServer is unique in global.
			If TCPEpollServer is not started, no server connection can come in. */
		void setClientIOWorker(std::shared_ptr<TCPClientIOWorker> clientIOWorker) { _clientIOWorker = clientIOWorker; }
		void setServerIOWorker(std::shared_ptr<TCPServerIOWorker> serverIOWorker) { _serverIOWorker = serverIOWorker; }
	};

	class GlobalIOPool;
	typedef std::shared_ptr<GlobalIOPool> GlobalIOPoolPtr;

	class GlobalIOPool
	{
		static std::mutex _mutex;
		std::shared_ptr<TCPEpollIOWorker> _ioWorker;
		ParamTemplateThreadPoolArray<TCPBasicConnection*> _ioPool;
		FPLogBasePtr _heldLogger;

		GlobalIOPool();

	public:
		static GlobalIOPool* nakedInstance();
		static GlobalIOPoolPtr instance();

		inline std::string ioPoolStatus() { return _ioPool.infos(); }
		inline void setClientIOWorker(std::shared_ptr<TCPClientIOWorker> clientIOWorker)
					{ _ioWorker->setClientIOWorker(clientIOWorker); }
		inline void setServerIOWorker(std::shared_ptr<TCPServerIOWorker> serverIOWorker)
					{ _ioWorker->setServerIOWorker(serverIOWorker); }

		inline void init(int32_t initCount, int32_t perAppendCount, int32_t perfectCount, int32_t maxCount)
		{
			std::unique_lock<std::mutex> lck(_mutex);

			if (_ioPool.inited())
				return;

			_ioPool.init(initCount, perAppendCount, perfectCount, maxCount);
			_heldLogger = FPLog::instance();
		}

		inline bool wakeUp(TCPBasicConnection* connection)
		{
			return _ioPool.wakeUp(connection);
		}

		inline void status(int32_t &normalThreadCount, int32_t &temporaryThreadCount, int32_t &busyThreadCount,
			int32_t &taskQueueSize, int32_t& min, int32_t& max, int32_t& maxQueue)
		{
			_ioPool.status(normalThreadCount, temporaryThreadCount, busyThreadCount, taskQueueSize, min, max, maxQueue);
		}

		inline void updateHeldLogger()
		{
			_heldLogger = FPLog::instance();
		}

		~GlobalIOPool()
		{
			//-- Although without destructor function, the release() of ioPool will also be called.
			//-- But do this explicitly, just ensure the heldLogger destroyed after tasks in ioPool.
			_ioPool.release();
		}
	};
}

#endif