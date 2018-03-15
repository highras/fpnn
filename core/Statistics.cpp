#include "Statistics.h"

using namespace fpnn;

StatMap Statistics::_statMap(255);

void Statistics::initMethod(const std::string& method){
	struct Stat st;
	_statMap.insert(method, st);

}
void Statistics::stat(const std::string& method, uint16_t status, uint32_t respTime){
	if(method[0] == '*') return; // do not stat *infos, *status, *tune

	uint32_t now = slack_real_sec() / 60;
	StatMap::node_type* node = _statMap.find(method);
	if(!node){
		//really??
		//unknow method can not reach here
		return;
	}

	node->data.quest++;
	node->data.answer++;
	if(!(status == 0 || status == 200))
		node->data.errorAnswer++;
	if(node->data.statTime == now){
		node->data.lastMinuteQuest++;
		node->data.lastMinuteResp += respTime;
	}
	else{
		node->data.statTime = now;
		if(node->data.lastMinuteQuest.load() > 0)
			node->data.AVGResp = node->data.lastMinuteResp.load() / node->data.lastMinuteQuest;
		else
			node->data.AVGResp = 0;
		node->data.lastMinuteResp = 0;

		node->data.QPS = node->data.lastMinuteQuest.load() / 60;
		node->data.lastMinuteQuest = 1;
	}
}
std::string Statistics::str(){
	std::stringstream ss;
	ss << "{";
	StatMap::node_type* node = _statMap.next_node(NULL);
	while(node){
		ss << "\"" << node->key << "\":" << node->data.str();
		node = _statMap.next_node(node);
		if(node) ss << ",";
	}

	ss << "}";

	return ss.str();
}

