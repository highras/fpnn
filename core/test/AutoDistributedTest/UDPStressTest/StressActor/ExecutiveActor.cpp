#include <sstream>
#include "CommandLineUtil.h"
#include "../CommonConstant.h"
#include "ExecutiveActor.h"

namespace ErrorInfo
{
	const int errorBase = 20000 * 10;

	const int UnknownError = errorBase + 0;
	const int ActionMethodNotFound = errorBase + 1;
}

bool ExecutiveActor::globalInit()
{
	int timeout = CommandLineParser::getInt("timeout", 60);
	int answerPoolThread = CommandLineParser::getInt("answerPoolThread");

	ClientEngine::configQuestProcessThreadPool(0, 1, 2, 6, 0);
	ClientEngine::setQuestTimeout(timeout);

	if (answerPoolThread)
		ClientEngine::configAnswerCallbackThreadPool(answerPoolThread, 1, answerPoolThread, answerPoolThread);

	_actorInstanceName = FPNN_Stress_Actor_Name;
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

void stressLauncher(int taskId, int connections, int totalQPS, ExecutiveActor* actor, struct StressTask stressTask)
{
	if (stressTask.stressSource->launch(connections, totalQPS) == false)
		return;

	actor->addAction(taskId, stressTask);
}

void ExecutiveActor::action(int taskId, const std::string& method, const FPReaderPtr payload)
{
	if (method == "beginStress")
	{
		std::string endpoint = payload->wantString("endpoint");
		int connections = payload->wantInt("connections");
		int totalQPS = payload->wantInt("totalQPS");
		int mtu = payload->getInt("mtu");

		StressTask stressTask;

		{
			std::stringstream ss;
			ss<<"connections: " <<  connections << ", PQS: " << totalQPS;

			stressTask.taskFinisher.reset(new TaskFinisher(taskId, method, ss.str()));
			stressTask.stressSource.reset(new StressSource(taskId, _region, endpoint));
			stressTask.stressSource->checkEncryptInfo(payload);
			stressTask.stressSource->setMTU(mtu);
		}
		
		std::thread(&stressLauncher, taskId, connections, totalQPS, this, stressTask).detach();
	}
	else if (method == "stopStress")
	{
		int orgTaskId = payload->wantInt("taskId");

		TaskFinisher(taskId, "stopStress", std::string("Stop stress task ").append(std::to_string(orgTaskId)));

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