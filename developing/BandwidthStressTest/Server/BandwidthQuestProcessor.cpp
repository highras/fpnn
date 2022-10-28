#include "BandwidthQuestProcessor.h"
#include "../Transfer/Transfer.cpp"

FPAnswerPtr BandwidthQuestProcessor::dl(const FPReaderPtr args, const FPQuestPtr quest, const ConnectionInfo& ci)
{
	std::string path = args->wantString("path");
	int taskId = args->wantInt("taskId");
	int blockSize = args->wantInt("blockSize");
	int timeout = args->getInt("timeout");
	int concurrent = args->getInt("concurrentCount", 10);
	int maxRetryCount = args->getInt("maxRetryCount", 5);

	QuestSenderPtr sender = genQuestSender(ci);
	SenderPtr DataSender = Sender::create(sender, path, blockSize, taskId);
	DataSender->setMaxRetryCount(maxRetryCount);
	DataSender->setConcurrentCount(concurrent);
	DataSender->setTimeout(timeout);

	FPAWriter aw(1, quest);
	aw.param("count", DataSender->totalCount());

	sendAnswer(aw.take());

	DataSender->launch();

	return nullptr;
}

FPAnswerPtr BandwidthQuestProcessor::up(const FPReaderPtr args, const FPQuestPtr quest, const ConnectionInfo& ci)
{
	std::string path = args->wantString("path");
	int blockSize = args->wantInt("blockSize");
	int count = args->wantInt("count");

	DataReceiverPtr receiver(new DataReceiver(path, blockSize));
	receiver->setTotalCount(count);

	int taskId;
	{
		std::unique_lock<std::mutex> lck(_mutex);

		taskId = ++_currTaskId;
		_receiverMap[taskId] = receiver;
	}

	FPAWriter aw(1, quest);
	aw.param("taskId", taskId);
	return aw.take();
}

FPAnswerPtr BandwidthQuestProcessor::ls(const FPReaderPtr args, const FPQuestPtr quest, const ConnectionInfo& ci)
{
	std::string path = args->wantString("path");

	if (args->getBool("all"))
	{
		std::vector<std::string> alldata = FileSystemUtil::getFilesInDirectories(path);
		FPAWriter aw(1, quest);
		aw.param("list", alldata);
		return aw.take();
	}

	if (args->getBool("onlyDir"))
	{
		std::vector<std::string> subDirs = FileSystemUtil::getSubDirectories(path);
		FPAWriter aw(1, quest);
		aw.param("subDirs", subDirs);
		return aw.take();
	}
	
	if (args->getBool("onlyFile"))
	{
		std::vector<std::string> files = FileSystemUtil::getFilesInDirectory(path);
		std::map<std::string, int64_t> fileMap;
		for (auto& name: files)
		{
			FileSystemUtil::FileAttrs attrs;
			if (FileSystemUtil::readFileAttrs(path + "/" + name, attrs))
				fileMap[name] = attrs.size;
			else
				fileMap[name] = -1;
		}
		FPAWriter aw(1, quest);
		aw.param("files", fileMap);
		return aw.take();
	}

	return FpnnErrorAnswer(quest, FPNN_EC_CORE_UNKNOWN_METHOD, "Unsupported action.");
}

FPAnswerPtr BandwidthQuestProcessor::data(const FPReaderPtr args, const FPQuestPtr quest, const ConnectionInfo& ci)
{
	int taskId = args->wantInt("taskId");
	DataReceiverPtr receiver;

	{
		std::unique_lock<std::mutex> lck(_mutex);
		auto it = _receiverMap.find(taskId);
		if (it != _receiverMap.end())
			receiver = it->second;
		else
			return FpnnErrorAnswer(quest, 100001, "receiver miss.");
	}

	receiver->processQuest(quest);
	if (receiver->done())
	{
		std::unique_lock<std::mutex> lck(_mutex);
		_receiverMap.erase(taskId);
	}
	return FPAWriter::emptyAnswer(quest);
}

FPAnswerPtr BandwidthQuestProcessor::stream(const FPReaderPtr args, const FPQuestPtr quest, const ConnectionInfo& ci)
{
	std::string data = args->wantString("data");

	FPAWriter aw(1, quest);
	aw.param("size", data.length());
	return aw.take();
}