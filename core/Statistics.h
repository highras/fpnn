#ifndef FPNN_Statistics_h
#define FPNN_Statistics_h
#include <string>
#include <atomic>
#include <sstream>
#include <memory>
#include "msec.h"
#include <HashMap.h>

namespace fpnn
{
	struct Stat{
		std::atomic<uint64_t> quest;
		std::atomic<uint64_t> answer;
		std::atomic<uint64_t> errorAnswer;
		std::atomic<uint32_t> lastMinuteQuest;
		std::atomic<uint32_t> QPS;
		std::atomic<uint32_t> lastMinuteResp;
		std::atomic<uint32_t> AVGResp;
		std::atomic<uint32_t> statTime;
		Stat(uint64_t quest = 0, uint64_t answer = 0, uint64_t errorAnswer = 0, uint32_t lastMinuteQuest = 0, uint32_t lastMinuteResp = 0){
			this->quest = quest;
			this->answer = answer;
			this->errorAnswer = errorAnswer;
			this->lastMinuteQuest = lastMinuteQuest;
			this->QPS = 0;
			this->lastMinuteResp = lastMinuteResp;
			this->AVGResp = 0;
			this->statTime = slack_real_sec() / 60;
		};

		Stat(const Stat& r): quest(r.quest.load()), answer(r.answer.load()), errorAnswer(r.errorAnswer.load()), lastMinuteQuest(r.lastMinuteQuest.load()), QPS(r.QPS.load()), lastMinuteResp(r.lastMinuteResp.load()), AVGResp(r.AVGResp.load()), statTime(r.statTime.load()) {};

		std::string str(){
			if(uint32_t(slack_real_sec() / 60) != statTime) {
				QPS = 0;
				AVGResp = 0;
			}
			std::stringstream ss;
			ss << "{";
			ss << "\"Quest\":"<<quest<<",";
			ss << "\"Answer\":"<<answer<<",";
			ss << "\"ErrorAnswer\":"<<errorAnswer<<",";
			ss << "\"QPS\":"<<QPS<<",";
			ss << "\"AVGResp\":"<<AVGResp;
			ss << "}";
			return ss.str();
		};
	};

	typedef HashMap<std::string, struct Stat> StatMap;

	class Statistics{
		static StatMap _statMap;
	public:
		static void initMethod(const std::string& method);
		static void stat(const std::string& method, uint16_t status, uint32_t respTime);
		static std::string str();
	};
}

#endif
