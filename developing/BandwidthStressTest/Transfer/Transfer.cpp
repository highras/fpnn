#include <atomic>
#include <time.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <iostream>
#include "FileSystemUtil.h"
#include "Transfer.h"

using namespace std;

std::mutex gc_printMutex;

void checkFilePath(const std::string& filePath)
{
	size_t pos = filePath.find_last_of("/");
	if (pos != std::string::npos)
	{
		std::string folders = filePath.substr(0, pos);
		FileSystemUtil::createDirectories(folders);
	}
}

Sender::Sender(const std::string& localPath, const std::string& serverPath, int blockSize, int taskId):
	_localPath(localPath), _serverPath(serverPath), _timeout(0), _concurrentCount(10), _maxRetryCount(5),
	_taskId(taskId), _doneCount(0), _finishCount(0), _cancelled(false)
{
	_unitSize = blockSize;

	FileSystemUtil::FileAttrs attrs;
	if (FileSystemUtil::readFileAttrs(localPath, attrs))
		_contentSize = attrs.size;
	else
		_contentSize = 0;

	_totalCount = _contentSize / _unitSize;
	if (_totalCount * (int64_t)_unitSize < _contentSize)
		_totalCount += 1;

	_buffer = (uint8_t*)malloc(_unitSize);
	_fd = open(localPath.c_str(), O_RDONLY);
	_currSeq = 0;
}

Sender::~Sender()
{
	close(_fd);
	free(_buffer);
}

SenderPtr Sender::create(ClientPtr client, const std::string& localPath, const std::string& serverPath, int blockSize)
{
	SenderPtr sender(new Sender(localPath, serverPath, blockSize, 0));
	sender->_client = client;

	return sender;
}

SenderPtr Sender::create(QuestSenderPtr questSender, const std::string& path, int blockSize, int taskId)
{
	SenderPtr sender(new Sender(path, "", blockSize, taskId));
	sender->_sender = questSender;

	return sender;
}

void Sender::failed(const std::string& reason)
{
	_finishCount = _concurrentCount;

	std::unique_lock<std::mutex> lck(gc_printMutex);
	cout<<"[Task: "<<_taskId<<"] Launch transform file \""<<_localPath<<"\" failed. reason: "<<reason<<endl;
}

void Sender::clientStart()
{
	FPQWriter qw(3, "up");
	qw.param("path", _serverPath);
	qw.param("blockSize", _unitSize);
	qw.param("count", _totalCount);

	SenderPtr self = shared_from_this();
	bool status = _client->sendQuest(qw.take(), [self](FPAnswerPtr answer, int errorCode){
		if (errorCode != FPNN_EC_OK)
		{
			self->failed(std::string("'up' quest answer error, code: ").append(std::to_string(errorCode)));
		}
		else
		{
			FPAReader ar(answer);
			int taskId = ar.wantInt("taskId");
			self->realStart(taskId);
		}
	}, _timeout);

	if (!status)
		failed("send 'up' quest failed.");
}

void Sender::launch()
{
	if (_contentSize == 0)
		return;

	if (_client)
		clientStart();
	else
		realStart(_taskId);
}

class QuestResender
{
	SenderPtr _sender;
	FPQuestPtr _quest;
	int _retryCount;
	int _seq;

	void failedCheck(int errorCode)
	{
		_retryCount += 1;
		if (_retryCount <= _sender->_maxRetryCount)
			run();
		else
			finalFailed(errorCode);
	}

	void finalFailed(int errorCode)
	{
		{
			std::unique_lock<std::mutex> lck(gc_printMutex);
			cout<<"[Error][Task: "<<_sender->_taskId<<"] Transform data for file \""<<_sender->_localPath<<"\" failed ("<<_sender->_finishCount<<"/"<<_sender->_concurrentCount<<"). seq: "<<_seq<<", retry: "<<_retryCount<<"/"<<_sender->_maxRetryCount<<", error code: "<<errorCode<<endl;
		}
		_sender->_finishCount++;
		_sender->_cancelled = true;
		delete this;
	}

	void success()
	{
		if (_retryCount > 0)
		{
			std::unique_lock<std::mutex> lck(gc_printMutex);
			cout<<"[Info][Task: "<<_sender->_taskId<<" seq: "<<_seq<<" retry: "<<_retryCount<<"/"<<_sender->_maxRetryCount<<"]"<<endl;
		}

		_sender->_doneCount++;
		_sender->next();
		delete this;
	}

public:
	QuestResender(SenderPtr sender, FPQuestPtr quest, int seq):
		_sender(sender), _quest(quest), _retryCount(0), _seq(seq)
	{}
	void run()
	{
		bool status;
		QuestResender* self = this;
		if (_sender->_client)
		{
			status = _sender->_client->sendQuest(_quest, [self](FPAnswerPtr answer, int errorCode){
				if (errorCode == FPNN_EC_OK)
					self->success();
				else
					self->failedCheck(errorCode);
			}, _sender->_timeout);
		}
		else
		{
			status = _sender->_sender->sendQuest(_quest, [self](FPAnswerPtr answer, int errorCode){
				if (errorCode == FPNN_EC_OK)
					self->success();
				else
					self->failedCheck(errorCode);
			}, _sender->_timeout);
		}

		if (!status)
			finalFailed(FPNN_EC_CORE_SEND_ERROR);
	}
};

void Sender::next()
{
	int seq;
	FPQuestPtr quest;
	{
		std::unique_lock<std::mutex> lck(_mutex);
		if (_currSeq == _totalCount || _cancelled)
		{
			_finishCount++;
			return;
		}

		seq = _currSeq;
		ssize_t readBytes = read(_fd, _buffer, _unitSize);

		FPQWriter qw(3, "data");
		qw.param("taskId", _taskId);
		qw.param("seq", seq);
		qw.paramBinary("data", _buffer, readBytes);
		quest = qw.take();

		_currSeq += 1;
	}

	QuestResender* resender = new QuestResender(shared_from_this(), quest, seq);
	resender->run();
}

void Sender::realStart(int taskId)
{
	_taskId = taskId;

	for (int i = 0; i< _concurrentCount; i++)
		next();
}


DataReceiver::DataReceiver(const std::string& savePath, int blockSize): _totalCount(-1), _unitSize(blockSize)
{
	checkFilePath(savePath);
	_fd = open(savePath.c_str(), O_WRONLY | O_TRUNC | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
}

DataReceiver::~DataReceiver()
{
	close(_fd);
}

void DataReceiver::setTotalCount(int count)
{
	std::unique_lock<std::mutex> lck(_mutex);
	_totalCount = count;
}

bool DataReceiver::done()
{
	std::unique_lock<std::mutex> lck(_mutex);
	return _totalCount == (int)_receivedSeqs.size();
}

int DataReceiver::currDone()
{
	std::unique_lock<std::mutex> lck(_mutex);
	return (int)_receivedSeqs.size();
}

void DataReceiver::processQuest(FPQuestPtr quest)
{
	FPQReader qr(quest);

	std::string data = qr.wantString("data");
	int seq = qr.wantInt("seq");

	std::unique_lock<std::mutex> lck(_mutex);
	if (_receivedSeqs.find(seq) == _receivedSeqs.end())
	{
		int64_t offset = seq * (int64_t)_unitSize;

		lseek(_fd, offset, SEEK_SET);
		write(_fd, data.data(), data.length());

		_receivedSeqs.insert(seq);
	}
}
