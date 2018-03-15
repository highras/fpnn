#ifndef FPNN_Partitioned_Connection_Map_H
#define FPNN_Partitioned_Connection_Map_H

#include <map>
#include <set>
#include <mutex>
#include <vector>
#include <sys/epoll.h>
#include "HashMap.h"
#include "FPMessage.h"
#include "FPWriter.h"
#include "AnswerCallbacks.h"
#include "IOWorker.h"
#include "msec.h"

namespace fpnn
{
	class ConnectionMap
	{
		std::mutex _mutex;
		HashMap<int, TCPBasicConnection*> _connections;

		inline bool sendData(TCPBasicConnection* conn, std::string* data)
		{
			bool needWaitSendEvent = false;
			conn->send(needWaitSendEvent, data);
			if (needWaitSendEvent)
				conn->waitForAllEvents();

			return true;
		}

		inline bool sendQuest(TCPBasicConnection* conn, std::string* data, uint32_t seqNum, BasicAnswerCallback* callback)
		{
			if (callback)
				conn->_callbackMap[seqNum] = callback;

			bool status = sendData(conn, data);
			if (!status && callback)
				conn->_callbackMap.erase(seqNum);
			
			return status;
		}

	public:
		ConnectionMap(size_t map_size): _connections(map_size) {}

		TCPBasicConnection* takeConnection(int fd)
		{
			std::unique_lock<std::mutex> lck(_mutex);
			HashMap<int, TCPBasicConnection*>::node_type* node = _connections.find(fd);
			if (node)
			{
				TCPBasicConnection* conn = node->data;
				_connections.remove_node(node);
				return conn;
			}
			return NULL;
		}

		TCPBasicConnection* takeConnection(const ConnectionInfo* ci)
		{
			std::unique_lock<std::mutex> lck(_mutex);
			HashMap<int, TCPBasicConnection*>::node_type* node = _connections.find(ci->socket);
			if (node)
			{
				TCPBasicConnection* conn = node->data;
				if ((uint64_t)conn == ci->token)
				{
					_connections.remove_node(node);
					return conn;
				}
			}
			return NULL;
		}

		bool insert(int fd, TCPBasicConnection* connection)
		{
			std::unique_lock<std::mutex> lck(_mutex);
			return (bool)_connections.insert(fd, connection);
		}

		void remove(int fd)
		{
			std::unique_lock<std::mutex> lck(_mutex);
			_connections.remove(fd);
		}

		TCPBasicConnection* signConnection(int fd, uint32_t events)
		{
			TCPBasicConnection* connection = NULL;
			std::unique_lock<std::mutex> lck(_mutex);
			HashMap<int, TCPBasicConnection*>::node_type* node = _connections.find(fd);
			if (node)
			{
				connection = node->data;

				if (events & EPOLLIN)
					connection->setNeedRecvFlag();
				if (events & EPOLLOUT)
					connection->setNeedSendFlag();

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
			HashMap<int, TCPBasicConnection*>::node_type* node = NULL;
			std::unique_lock<std::mutex> lck(_mutex);
			while ((node = _connections.next_node(node)))
				fdSet.insert(node->key);
		}

		bool sendData(int socket, uint64_t token, std::string* data)
		{
			std::unique_lock<std::mutex> lck(_mutex);
			HashMap<int, TCPBasicConnection*>::node_type* node = _connections.find(socket);
			if (node)
			{
				TCPBasicConnection* connection = node->data;
				if (token == (uint64_t)connection)
					return sendData(connection, data);
			}
			return false;
		}

		bool sendQuest(int socket, uint64_t token, std::string* data, uint32_t seqNum, BasicAnswerCallback* callback)
		{
			std::unique_lock<std::mutex> lck(_mutex);
			HashMap<int, TCPBasicConnection*>::node_type* node = _connections.find(socket);
			if (node)
			{
				TCPBasicConnection* connection = node->data;
				if (token == (uint64_t)connection)
					return sendQuest(connection, data, seqNum, callback);
			}
			return false;
		}

		BasicAnswerCallback* takeCallback(int socket, uint32_t seqNum);
		void extractTimeoutedCallback(int64_t threshold, std::list<std::map<uint32_t, BasicAnswerCallback*> >& timeouted);
		void extractTimeoutedConnections(int64_t threshold, std::list<TCPBasicConnection*>& timeouted);
	};

	class PartitionedConnectionMap
	{
		int _count;
		std::vector<ConnectionMap *> _array;

		bool sendQuestWithBasicAnswerCallback(int socket, uint64_t token, FPQuestPtr quest, BasicAnswerCallback* callback, int timeout);

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

		TCPBasicConnection* takeConnection(int fd)
		{
			int idx = fd % _count;
			return _array[idx]->takeConnection(fd);
		}

		TCPBasicConnection* takeConnection(const ConnectionInfo* ci)
		{
			int idx = ci->socket % _count;
			return _array[idx]->takeConnection(ci);
		}

		bool insert(int fd, TCPBasicConnection* conn)
		{
			int idx = fd % _count;
			return _array[idx]->insert(fd, conn);
		}

		void remove(int fd)
		{
			int idx = fd % _count;
			_array[idx]->remove(fd);
		}

		TCPBasicConnection* signConnection(int fd, uint32_t events)
		{
			int idx = fd % _count;
			return _array[idx]->signConnection(fd, events);
		}

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

		/**
			All SendQuest():
				If return false, caller must free quest & callback.
				If return true, don't free quest & callback.
		*/
		FPAnswerPtr sendQuest(int socket, uint64_t token, std::mutex* mutex, FPQuestPtr quest, int timeout);

		inline bool sendQuest(int socket, uint64_t token, FPQuestPtr quest, AnswerCallback* callback, int timeout)
		{
			return sendQuestWithBasicAnswerCallback(socket, token, quest, callback, timeout);
		}
		inline bool sendQuest(int socket, uint64_t token, FPQuestPtr quest, std::function<void (FPAnswerPtr answer, int errorCode)> task, int timeout)
		{
			BasicAnswerCallback* t = new FunctionAnswerCallback(std::move(task));
			if (sendQuestWithBasicAnswerCallback(socket, token, quest, t, timeout))
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

		void extractTimeoutedConnections(int64_t threshold, std::list<TCPBasicConnection*>& timeouted)
		{
			for (int i = 0; i < _count; i++)
			{
				_array[i]->extractTimeoutedConnections(threshold, timeouted);
			}
		}
	};
}

#endif

