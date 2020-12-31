#include <sys/time.h>
#include <sys/resource.h>
#include <sstream>
#include "CommandLineUtil.h"
#include "../CommonConstant.h"
#include "ExecutiveActor.h"

namespace ErrorInfo
{
	const int errorBase = 20000 * 10;

	const int UnknownError = errorBase + 0;
	const int ActionMethodNotFound = errorBase + 1;
	const int InvalidParameters = errorBase + 2;
}

bool ExecutiveActor::globalInit()
{
	struct rlimit rlim;
	rlim.rlim_cur = 100000;
	rlim.rlim_max = 100000;
	setrlimit(RLIMIT_NOFILE, &rlim);

	int timeout = CommandLineParser::getInt("timeout", 300);
	int answerPoolThread = CommandLineParser::getInt("answerPoolThread");

	ClientEngine::configQuestProcessThreadPool(0, 1, 2, 6, 0);
	ClientEngine::setQuestTimeout(timeout);

	if (answerPoolThread)
		ClientEngine::configAnswerCallbackThreadPool(answerPoolThread, 1, answerPoolThread, answerPoolThread);

	_actorInstanceName = FPNN_Massive_Connections_Actor_Name;
	std::string sign = CommandLineParser::getString("uniqueId");
	if (sign.size())
		_actorInstanceName.append("-").append(sign);

	return true;
}

bool ExecutiveActor::actorStopped()
{
	return !_running;
}

std::string ExecutiveActor::actorName()
{
	return _actorInstanceName;
}

std::string ExecutiveActor::customParamsUsage()
{
	return " [--answerPoolThread client_work_thread] [--timeout questTimeoutInSeconds] [--uniqueId uniqueInstanceId]";
}

void ExecutiveActor::addAction(int taskId, struct StressTask& stressTask)
{
	std::unique_lock<std::mutex> lck(_mutex);
	_taskMap[taskId] = stressTask;
}

void stressLauncher(int taskId, int threadCount, int clientCount, double perClientQPS, ExecutiveActor* actor, struct StressTask stressTask)
{
	if (stressTask.stressSource->launch(threadCount, clientCount, perClientQPS) == false)
		return;

	actor->addAction(taskId, stressTask);
}

void ExecutiveActor::action(int taskId, const std::string& method, const FPReaderPtr payload)
{
	if (method == "beginStress")
	{
		std::string endpoint = payload->wantString("endpoint");
		int threadCount = payload->wantInt("threadCount");
		int clientCount = payload->wantInt("clientCount");
		double perClientQPS = payload->wantDouble("perClientQPS");

		if (clientCount == 0 || threadCount == 0)
			throw FPNN_ERROR_CODE_MSG(FpnnLogicError, ErrorInfo::InvalidParameters, "Invalid parameters of beginStress method.");

		StressTask stressTask;

		{
			std::stringstream ss;
			ss<<"threadCount: " <<  threadCount << "clientCount: " <<  clientCount << ", perClientQPS: " << perClientQPS;

			stressTask.taskFinisher.reset(new TaskFinisher(taskId, method, ss.str()));
			stressTask.stressSource.reset(new StressSource(taskId, _region, endpoint));
		}
		
		std::thread(&stressLauncher, taskId, threadCount, clientCount, perClientQPS, this, stressTask).detach();
	}
	if (method == "autoBoostStress")
	{
		std::string endpoint = payload->wantString("endpoint");
		int threadCount = payload->getInt("threadCount", 100);
		int clientCount = payload->getInt("clientCount", 10000);
		double perClientQPS = payload->getDouble("perClientQPS", 0.02);
		int firstWaitMinute = payload->getInt("firstWaitMinute", 10);
		int intervalMinute = payload->getInt("intervalMinute", 10);
		int decSleepMsec = payload->getInt("decSleepMsec", 5);
		int minSleepMsec = payload->getInt("minSleepMsec", 5);

		if (clientCount == 0 || threadCount == 0)
			throw FPNN_ERROR_CODE_MSG(FpnnLogicError, ErrorInfo::InvalidParameters, "Invalid parameters of beginStress method.");

		StressTask stressTask;

		{
			std::stringstream ss;
			ss<<"threadCount: " <<  threadCount << "clientCount: " <<  clientCount << ", perClientQPS: " << perClientQPS;
			ss<<", intervalMinute: "<< intervalMinute<<", decSleepMsec: "<<decSleepMsec;

			stressTask.taskFinisher.reset(new TaskFinisher(taskId, method, ss.str()));
			stressTask.stressSource.reset(new StressSource(taskId, _region, endpoint, firstWaitMinute, intervalMinute, decSleepMsec, minSleepMsec));
		}
		
		std::thread(&stressLauncher, taskId, threadCount, clientCount, perClientQPS, this, stressTask).detach();
	}
	else if (method == "stopStress")
	{
		int orgTaskId = payload->wantInt("taskId");

		TaskFinisher(taskId, "stopStress", std::string("Stop massive connections task ").append(std::to_string(orgTaskId)));

		std::unique_lock<std::mutex> lck(_mutex);
		_taskMap.erase(orgTaskId);
	}
	else if (method == "quit")
	{
		_running = false;
	}
	else
		throw FPNN_ERROR_CODE_FMT(FpnnLogicError, ErrorInfo::ActionMethodNotFound, "Action method %s is not found.", method.c_str());
}