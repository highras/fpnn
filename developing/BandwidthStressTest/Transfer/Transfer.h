#ifndef Deployer_h
#define Deployer_h

#include "IQuestProcessor.h"
#include "ClientInterface.h"

using namespace fpnn;

class Sender;
typedef std::shared_ptr<Sender> SenderPtr;

class Sender: public std::enable_shared_from_this<Sender>
{
	friend class QuestResender;

	std::string _localPath;
	std::string _serverPath;
	int64_t _contentSize;
	int _unitSize;
	int _totalCount;

	ClientPtr _client;
	QuestSenderPtr _sender;
	int _timeout;
	int _concurrentCount;
	int _maxRetryCount;

	int _taskId;
	int _fd;

	std::mutex _mutex;
	uint8_t* _buffer;
	int _currSeq;
	std::atomic<int> _doneCount;
	std::atomic<int> _finishCount;
	bool _cancelled;
	
	Sender(const std::string& localPath, const std::string& serverPath, int KBSize, int taskId);
	void clientStart();

public:	//-- Interior using.
	void next();
	void realStart(int taskId);
	void failed(const std::string& reason);

public:
	static SenderPtr create(ClientPtr client, const std::string& localPath, const std::string& serverPath, int KBSize);
	static SenderPtr create(QuestSenderPtr sender, const std::string& path, int KBSize, int taskId);
	~Sender();
	void launch();
	bool done() { return _concurrentCount == _finishCount; }
	int totalCount() { return _totalCount; }
	int currDone() { return _doneCount; }
	void setConcurrentCount(int count) { _concurrentCount = count; }
	void setMaxRetryCount(int count) { _maxRetryCount = count; }
	void setTimeout(int second) { _timeout = second; }
};

class DataReceiver
{
	std::mutex _mutex;
	std::set<int> _receivedSeqs;
	int _totalCount;
	int _unitSize;
	int _fd;

public:
	DataReceiver(const std::string& savePath, int blockSize);
	~DataReceiver();
	void setTotalCount(int count);
	void processQuest(FPQuestPtr quest);
	int totalCount() { return _totalCount; }
	int currDone();
	bool done();
};
typedef std::shared_ptr<DataReceiver> DataReceiverPtr;

#endif