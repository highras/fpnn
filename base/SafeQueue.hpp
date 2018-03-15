#ifndef Safe_Queue_hpp
#define Safe_Queue_hpp

#include <queue>
#include <exception>
#include <mutex>
namespace fpnn {
template <typename K>
class SafeQueue
{
public:
	class EmptyException: public std::exception
	{
	public:
		virtual const char* what() const throw ()
		{
			return "Safe queue is empty.";
		}
	};
	
private:
	std::queue<K> _queue;
	std::mutex _mutex;

public:
	SafeQueue(): _queue(), _mutex()
	{
	}
	~SafeQueue()
	{
	}
	
	bool empty()
	{
		std::lock_guard<std::mutex> lck (_mutex);
		return _queue.empty();
	}
	
	size_t size()
	{
		std::lock_guard<std::mutex> lck (_mutex);
		return _queue.size();
	}
	
	void push(K &data)
	{
		std::lock_guard<std::mutex> lck (_mutex);
		_queue.push(data);
	}
	
	K pop()
	{
		std::lock_guard<std::mutex> lck (_mutex);
		if (_queue.empty())
			throw EmptyException();
			
		K k = _queue.front();
		_queue.pop();
		return k;
	}
	
	void clear()
	{
		std::lock_guard<std::mutex> lck (_mutex);
		while (_queue.size())
			_queue.pop();
	}
	
	void swap(std::queue<K> &queue)
	{
		std::lock_guard<std::mutex> lck (_mutex);
		_queue.swap(queue);
	}
	
	void swap(SafeQueue<K> &squeue)
	{
		std::lock(_mutex, squeue._mutex);
		_queue.swap(squeue._queue);
		_mutex.unlock();
		squeue._mutex.unlock();
	}
};
}
#endif
