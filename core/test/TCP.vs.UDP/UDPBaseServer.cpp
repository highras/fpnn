#include <iostream>
#include "TCPEpollServer.h"
#include "UDPEpollServer.h"
#include "UDPTestProcessor.h"
#include "Setting.h"

void ServerLoop(ServerPtr server)
{
	server->startup();
	server->run();
}

int main(int argc, char* argv[])
{
	try{
		if (argc != 2){
			std::cout<<"Usage: "<<argv[0]<<" config"<<std::endl;
			return 0;
		}
		if(!Setting::load(argv[1])){
			std::cout<<"Config file error:"<< argv[1]<<std::endl;
			return 1;
		}

		IQuestProcessorPtr questProcessor = std::make_shared<UDPTestProcessor>();
		ServerPtr tcpServer = TCPEpollServer::create();
		tcpServer->setQuestProcessor(questProcessor);
		
		ServerPtr udpServer = UDPEpollServer::create();
		udpServer->setQuestProcessor(questProcessor);

		std::thread udpLoop = std::thread(&ServerLoop, udpServer);

		ServerLoop(tcpServer);
		udpLoop.join();
	}
	catch(const std::exception& ex){
		std::cout<<"exception:"<<ex.what()<<std::endl;
	}
	catch(...){
		std::cout<<"Unknow exception."<<std::endl;
	}

	return 0;
}