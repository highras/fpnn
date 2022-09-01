#ifndef FPNN_Partitioned_Connection_Map_H
#define FPNN_Partitioned_Connection_Map_H

#include <map>
#include <set>
#include <mutex>
#include <vector>
#ifdef __APPLE__
	#include <sys/types.h>
	#include <sys/event.h>
	#include <sys/time.h>
#else
	#include <sys/epoll.h>
#endif
#include "HashMap.h"
#include "FPMessage.h"
#include "FPWriter.h"
#include "AnswerCallbacks.h"
#include "IOWorker.h"
#include "ServerIOWorker.h"		//-- only for closeAfterSent().
#include "msec.h"
#include "ClientIOWorker.h"
#include "UDPClientConnection.h"
#include "RawTransmission/RawClientIOWorker.h"

namespace fpnn
{
	class ConnectionMap
	{
		struct TCPClientKeepAliveTimeoutInfo
		{
			TCPClientConnection* conn;
			int timeout;					//-- In milliseconds
		};

		std::mutex _mutex;
		HashMap<int, BasicConnection*> _connections;

		inline bool sendData(BasicConnection* conn, std::string* data)
		{
			bool needWaitSendEvent = false;
			conn->send(needWaitSendEvent, data);
			if (needWaitSendEvent)
				conn->waitForAllEvents();

			return true;
		}

		inline bool sendData(UDPClientConnection* conn, std::string* data, int64_t expiredMS, bool discardable)
		{
			bool needWaitSendEvent = false;
			conn->sendData(needWaitSendEvent, data, expiredMS, discardable);
			if (needWaitSendEvent)
				conn->waitForAllEvents();

			return true;
		}

		inline bool sendQuest(BasicConnection* conn, std::string* data, uint32_t seqNum, BasicAnswerCallback* callback, int timeout, bool discardableUDPQuest)
		{
			if (callback)
				conn->_callbackMap[seqNum] = callback;

			bool status;
			if (conn->connectionType() != BasicConnection::UDPClientConnectionType)
				status = sendData(conn, data);
			else
				status = sendData((UDPClientConnection*)conn, data, slack_real_msec() + timeout, discardableUDPQuest);

			if (!status && callback)
				conn->_callbackMap.erase(seqNum);
			
			return status;
		}

		void sendTCPClientKeepAlivePingQuest(TCPClientSharedKeepAlivePingDatas& sharedPing, std::list<TCPClientKeepAliveTimeoutInfo>& keepAliveList)
		{
			std::unique_lock<std::mutex> lck(_mutex);

			for (auto& node: keepAliveList)
			{
				std::string* raw = new std::string(*(sharedPing.rawData));
				KeepAliveCallback* callback = new KeepAliveCallback(node.conn->_connectionInfo);

				callback->updateExpiredTime(slack_real_msec() + node.timeout);
				sendQuest(node.conn, raw, sharedPing.seqNum, callback, node.timeout, false);
				
				node.conn->updateKeepAliveMS();
				node.conn->_refCount--;
			}
		}

	public:
		ConnectionMap(size_t map_size): _connections(map_size) {
		}

		size_t count() { std::unique_lock<std::mutex> lck(_mutex); return _connections.count(); }

		BasicConnection* takeConnection(int fd)
		{
			std::unique_lock<std::mutex> lck(_mutex);
			HashMap<int, BasicConnection*>::node_type* node = _connections.find(fd);
			if (node)
			{
				BasicConnection* conn = node->data;
				_connections.remove_node(node);
				return conn;
			}
			return NULL;
		}

		BasicConnection* takeConnection(const ConnectionInfo* ci)
		{
			std::unique_lock<std::mutex> lck(_mutex);
			HashMap<int, BasicConnection*>::node_type* node = _connections.find(ci->socket);
			if (node)
			{
				BasicConnection* conn = node->data;
				if ((uint64_t)conn == ci->token)
				{
					_connections.remove_node(node);
					return conn;
				}
			}
			return NULL;
		}

		void closeAfterSent(const ConnectionInfo* ci)
		{
			bool needWaitSendEvent = false;
			std::unique_lock<std::mutex> lck(_mutex);

			HashMap<int, BasicConnection*>::node_type* node = _connections.find(ci->socket);
			if (node)
			{
				BasicConnection* conn = node->data;
				if ((uint64_t)conn == ci->token)
				{
					if (conn->connectionType() == BasicConnection::TCPServerConnectionType)
					{
						TCPServerConnection *sconn = (TCPServerConnection*)conn;
						sconn->closeAfterSent(needWaitSendEvent);
						if (needWaitSendEvent)
							conn->waitForAllEvents();
					}
				}
			}
		}

		bool insert(int fd, BasicConnection* connection)
		{
			std::unique_lock<std::mutex> lck(_mutex);
			return (bool)_connections.insert(fd, connection);
		}

		void remove(int fd)
		{
			std::unique_lock<std::mutex> lck(_mutex);
			_connections.remove(fd);
		}

#ifdef __APPLE__
		BasicConnection* signConnection(int fd, uint16_t filter)
#else
		BasicConnection* signConnection(int fd, uint32_t events)
#endif
		{
			BasicConnection* connection = NULL;
			std::unique_lock<std::mutex> lck(_mutex);
			HashMap<int, BasicConnection*>::node_type* node = _connections.find(fd);
			if (node)
			{
				connection = node->data;

#ifdef __APPLE__
				if (filter & EVFILT_READ)
					connection->setNeedRecvFlag();
				if (filter & EVFILT_WRITE)
					connection->setNeedSendFlag();
#else
				if (events & EPOLLIN)
					connection->setNeedRecvFlag();
				if (events & EPOLLOUT)
					connection->setNeedSendFlag();
#endif

				connection->_refCount++;
			}
			return connection;
		}

		void waitForEmpty()
		{
			while (_connections.count() > 0)
				usleep(20000);
		}

		void getAllSocket(std::set<int>& fdSet)
		{
			HashMap<int, BasicConnection*>::node_type* node = NULL;
			std::unique_lock<std::mutex> lck(_mutex);
			while ((node = _connections.next_node(node)))
				fdSet.insert(node->key);
		}

		bool sendData(int socket, uint64_t token, std::string* data)
		{
			std::unique_lock<std::mutex> lck(_mutex);
			HashMap<int, BasicConnection*>::node_type* node = _connections.find(socket);
			if (node)
			{
				BasicConnection* connection = node->data;
				if (token == (uint64_t)connection)
					return sendData(connection, data);
			}
			return false;
		}

		bool sendUDPData(int socket, uint64_t token, std::string* data, int64_t expiredMS, bool discardable)
		{
			std::unique_lock<std::mutex> lck(_mutex);
			HashMap<int, BasicConnection*>::node_type* node = _connections.find(socket);
			if (node)
			{
				BasicConnection* connection = node->data;
				if (token == (uint64_t)connection)
					return sendData((UDPClientConnection*)connection, data, expiredMS, discardable);
			}
			return false;
		}

		bool sendQuest(int socket, uint64_t token, std::string* data, uint32_t seqNum, BasicAnswerCallback* callback, int timeout, bool discardableUDPQuest)
		{
			std::unique_lock<std::mutex> lck(_mutex);
			HashMap<int, BasicConnection*>::node_type* node = _connections.find(socket);
			if (node)
			{
				BasicConnection* connection = node->data;
				if (token == (uint64_t)connection)
				{
					bool ss = sendQuest(connection, data, seqNum, callback, timeout, discardableUDPQuest);
					return ss;
				}
			}

			return false;
		}

		void keepAlive(int socket, bool keepAlive)
		{
			std::unique_lock<std::mutex> lck(_mutex);
			HashMap<int, BasicConnection*>::node_type* node = _connections.find(socket);
			if (node)
			{
				BasicConnection* connection = node->data;
				if (keepAlive && connection->connectionType() == BasicConnection::UDPClientConnectionType)
				{
					((UDPClientConnection*)connection)->enableKeepAlive();
				}
			}
		}

		void setUDPUntransmittedSeconds(int socket, int untransmittedSeconds)
		{
			std::unique_lock<std::mutex> lck(_mutex);
			HashMap<int, BasicConnection*>::node_type* node = _connections.find(socket);
			if (node)
			{
				BasicConnection* connection = node->data;
				if (connection->connectionType() == BasicConnection::UDPClientConnectionType)
				{
					((UDPClientConnection*)connection)->setUntransmittedSeconds(untransmittedSeconds);
				}
			}
		}

		void executeConnectionAction(int socket, std::function<void (BasicConnection* conn)> action)
		{
			std::unique_lock<std::mutex> lck(_mutex);
			HashMap<int, BasicConnection*>::node_type* node = _connections.find(socket);
			if (node)
			{
				BasicConnection* connection = node->data;
				action(connection);
			}
		}

		void periodUDPSendingCheck(std::unordered_set<UDPClientConnection*>& invalidOrExpiredConnections)
		{
			std::set<UDPClientConnection*> udpConnections;
			std::unordered_set<UDPClientConnection*> invalidConns;
			{
				HashMap<int, BasicConnection*>::node_type* node = NULL;
				std::unique_lock<std::mutex> lck(_mutex);
				while ((node = _connections.next_node(node)))
				{
					BasicConnection* connection = node->data;
					if (connection->connectionType() == BasicConnection::UDPClientConnectionType)
					{
						UDPClientConnection* conn = (UDPClientConnection*)connection;
						if (conn->isRequireClose() == false)
						{
							conn->_refCount++;
							udpConnections.insert(conn);
						}
						else
						{
							invalidConns.insert(conn);
							invalidOrExpiredConnections.insert(conn);
						}
					}
				}
				for (UDPClientConnection* conn: invalidConns)
					_connections.remove(conn->_connectionInfo->socket);
			}

			for (auto conn: udpConnections)
			{
				bool needWaitSendEvent = false;
				conn->sendCachedData(needWaitSendEvent, false);
				if (needWaitSendEvent)
					conn->waitForAllEvents();

				conn->_refCount--;
			}
		}

		BasicAnswerCallback* takeCallback(int socket, uint32_t seqNum);
		void extractTimeoutedCallback(int64_t threshold, std::list<std::map<uint32_t, BasicAnswerCallback*> >& timeouted);
		void extractTimeoutedConnections(int64_t threshold, std::list<BasicConnection*>& timeouted);
		void TCPClientKeepAlive(TCPClientSharedKeepAlivePingDatas& sharedPing, std::list<TCPClientConnection*>& invalidConnections);
	};

	class PartitionedConnectionMap
	{
		int _count;
		std::vector<ConnectionMap *> _array;

		bool sendQuestWithBasicAnswerCallback(int socket, uint64_t token, FPQuestPtr quest, BasicAnswerCallback* callback, int timeout, bool discardableUDPQuest);

	public:
		PartitionedConnectionMap(): _count(0) {}
		~PartitionedConnectionMap()
		{
			clear();
		}

		void clear()
		{
			for (size_t i = 0; i < _array.size(); i++)
				delete _array[i];

			_array.clear();
			_count = 0;
		}

		void init(int count, size_t per_map_size)
		{
			if (_array.size())
				clear();

			for (int i = 0; i < count; i++)
			{
				ConnectionMap* cm = new ConnectionMap(per_map_size);
				_array.push_back(cm);
			}

			_count = count;
		}

		bool inited()
		{
			return (bool)_array.size();
		}

		BasicConnection* takeConnection(int fd)
		{
			int idx = fd % _count;
			return _array[idx]->takeConnection(fd);
		}

		BasicConnection* takeConnection(const ConnectionInfo* ci)
		{
			int idx = ci->socket % _count;
			return _array[idx]->takeConnection(ci);
		}

		void closeAfterSent(const ConnectionInfo* ci)
		{
			int idx = ci->socket % _count;
			return _array[idx]->closeAfterSent(ci);
		}

		bool insert(int fd, BasicConnection* conn)
		{
			int idx = fd % _count;
			return _array[idx]->insert(fd, conn);
		}

		void remove(int fd)
		{
			int idx = fd % _count;
			_array[idx]->remove(fd);
		}

#ifdef __APPLE__
		BasicConnection* signConnection(int fd, uint16_t filter)
		{
			int idx = fd % _count;
			return _array[idx]->signConnection(fd, filter);
		}
#else
		BasicConnection* signConnection(int fd, uint32_t events)
		{
			int idx = fd % _count;
			return _array[idx]->signConnection(fd, events);
		}
#endif

		void waitForEmpty()
		{
			for (int i = 0; i < _count; i++)
				_array[i]->waitForEmpty();
		}

		void getAllSocket(std::set<int>& fdSet)
		{
			for (int i = 0; i < _count; i++)
				_array[i]->getAllSocket(fdSet);
		}

		bool sendData(int socket, uint64_t token, std::string* data)
		{
			int idx = socket % _count;
			return _array[idx]->sendData(socket, token, data);
		}

		bool sendUDPData(int socket, uint64_t token, std::string* data, int64_t expiredMS, bool discardable)
		{
			int idx = socket % _count;
			return _array[idx]->sendUDPData(socket, token, data, expiredMS, discardable);
		}

		/**
			All SendQuest():
				If return false, caller must free quest & callback.
				If return true, don't free quest & callback.
		*/
		FPAnswerPtr sendQuest(int socket, uint64_t token, std::mutex* mutex, FPQuestPtr quest, int timeout, bool discardableUDPQuest = false);

		inline bool sendQuest(int socket, uint64_t token, FPQuestPtr quest, AnswerCallback* callback, int timeout, bool discardableUDPQuest = false)
		{
			return sendQuestWithBasicAnswerCallback(socket, token, quest, callback, timeout, discardableUDPQuest);
		}
		inline bool sendQuest(int socket, uint64_t token, FPQuestPtr quest, std::function<void (FPAnswerPtr answer, int errorCode)> task, int timeout, bool discardableUDPQuest = false)
		{
			BasicAnswerCallback* t = new FunctionAnswerCallback(std::move(task));
			if (sendQuestWithBasicAnswerCallback(socket, token, quest, t, timeout, discardableUDPQuest))
				return true;
			else
			{
				delete t;
				return false;
			}
		}

		BasicAnswerCallback* takeCallback(int socket, uint32_t seqNum)
		{
			int idx = socket % _count;
			return _array[idx]->takeCallback(socket, seqNum);
		}

		void extractTimeoutedCallback(int64_t threshold, std::list<std::map<uint32_t, BasicAnswerCallback*> >& timeouted)
		{
			for (int i = 0; i < _count; i++)
			{
				_array[i]->extractTimeoutedCallback(threshold, timeouted);
			}
		}

		void extractTimeoutedConnections(int64_t threshold, std::list<BasicConnection*>& timeouted)
		{
			for (int i = 0; i < _count; i++)
			{
				_array[i]->extractTimeoutedConnections(threshold, timeouted);
			}
		}

		size_t count()
		{
			size_t count = 0;
			for (int i = 0; i < _count; i++)
			{
				count += _array[i]->count();
			}
			return count;
		}

		//-- Only for ARQ UDP
		void keepAlive(int socket, bool keepAlive)
		{
			int idx = socket % _count;
			_array[idx]->keepAlive(socket, keepAlive);
		}

		//-- Only for ARQ UDP
		void setUDPUntransmittedSeconds(int socket, int untransmittedSeconds)
		{
			int idx = socket % _count;
			_array[idx]->setUDPUntransmittedSeconds(socket, untransmittedSeconds);
		}

		void executeConnectionAction(int socket, std::function<void (BasicConnection* conn)> action)
		{
			int idx = socket % _count;
			_array[idx]->executeConnectionAction(socket, std::move(action));
		}

		void periodUDPSendingCheck(std::unordered_set<UDPClientConnection*>& invalidOrExpiredConnections)
		{
			for (int i = 0; i < _count; i++)
				_array[i]->periodUDPSendingCheck(invalidOrExpiredConnections);
		}

		void TCPClientKeepAlive(std::list<TCPClientConnection*>& invalidConnections)
		{
			TCPClientSharedKeepAlivePingDatas sharedPing;

			for (int i = 0; i < _count; i++)
				_array[i]->TCPClientKeepAlive(sharedPing, invalidConnections);
		}
	};
}

#endif

