#include "MidGenerator.h"
#include "msec.h"
#include <arpa/inet.h>

std::atomic<uint32_t> MidGenerator::_sn(0);
int32_t MidGenerator::_pip(0);

void MidGenerator::init(const std::string& ip4){
	_pip = ntohl(inet_addr(ip4.c_str()));
	_pip &= 0XFF;
	_pip <<= 24; 
}

void MidGenerator::init(int32_t rand){
	_pip = rand;
	_pip &= 0XFF;
	_pip <<= 24; 
}

int64_t MidGenerator::genMid(){
	return (slack_real_sec() << 32) | (_pip & 0XFF000000) | (_sn++ & 0XFFFFFF);
}
