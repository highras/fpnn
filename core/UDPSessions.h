#ifndef FPNN_UDP_Sessions_H
#define FPNN_UDP_Sessions_H

#include <list>
#include <mutex>
#include <vector>
#include <unordered_map>
#include "IQuestProcessor.h"

namespace fpnn
{
	class UDPCallbackMap
	{
		std::mutex _mutex;
		std::unordered_map<uint64_t, std::unordered_map<uint32_t, BasicAnswerCallback*>> _callbackMap;

	public:
		bool insert(ConnectionInfoPtr ci, uint32_t seqNum, BasicAnswerCallback* callback);
		BasicAnswerCallback* takeCallback(ConnectionInfoPtr ci, uint32_t seqNum);
		void extractTimeoutedCallback(int64_t now, std::list<std::map<uint32_t, BasicAnswerCallback*> >& timeouted);
		void clearAndFetchAllRemnantCallbacks(std::set<BasicAnswerCallback*> &callbacks);
	};

	class PartitionedCallbackMap
	{
		int _count;
		std::vector<UDPCallbackMap *> _array;

	public:
		PartitionedCallbackMap(): _count(0) {}
		~PartitionedCallbackMap()
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

		void init(int count)
		{
			if (_array.size())
				clear();

			for (int i = 0; i < count; i++)
			{
				UDPCallbackMap* si = new UDPCallbackMap();
				_array.push_back(si);
			}

			_count = count;
		}

		bool inited()
		{
			return (bool)_array.size();
		}

		/*===============================================================================
		  Business interfaces.
		=============================================================================== */
		bool insert(ConnectionInfoPtr ci, uint32_t seqNum, BasicAnswerCallback* callback)
		{
			int idx = (int)(ci->token % _count);
			return _array[idx]->insert(ci, seqNum, callback);
		}

		BasicAnswerCallback* takeCallback(ConnectionInfoPtr ci, uint32_t seqNum)
		{
			int idx = ci->token % _count;
			return _array[idx]->takeCallback(ci, seqNum);
		}

		void extractTimeoutedCallback(int64_t now, std::list<std::map<uint32_t, BasicAnswerCallback*> >& timeouted)
		{
			for (int i = 0; i < _count; i++)
				_array[i]->extractTimeoutedCallback(now, timeouted);
		}
		void clearAndFetchAllRemnantCallbacks(std::set<BasicAnswerCallback*> &callbacks)
		{
			for (int i = 0; i < _count; i++)
				_array[i]->clearAndFetchAllRemnantCallbacks(callbacks);
		}
	};
}

#endif
