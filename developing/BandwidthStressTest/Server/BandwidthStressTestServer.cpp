#include <iostream>
#include "TCPEpollServer.h"
#include "UDPEpollServer.h"
#include "BandwidthQuestProcessor.h"
#include "Setting.h"

void runServer(ServerPtr server, IQuestProcessorPtr processor)
{
	server->setQuestProcessor(processor);
	if (server->startup())
		server->run();
}

int main(int argc, char* argv[])
{
	if (argc != 2)
	{
		std::cout<<"Usage: "<<argv[0]<<" config"<<std::endl;
		return 0;
	}
	if(!Setting::load(argv[1])){
		std::cout<<"Config file error:"<< argv[1]<<std::endl;
		return 1;
	}

	IQuestProcessorPtr processor = std::make_shared<BandwidthQuestProcessor>();
	std::thread tcpLoop(runServer, TCPEpollServer::create(), processor);
	runServer(UDPEpollServer::create(), processor);
	tcpLoop.join();

	return 0;
}
