#include <iostream>
#include "CommandLineUtil.h"
#include "FormattedPrint.h"
#include "TCPClient.h"
#include "UDPClient.h"
#include "../Transfer/Transfer.cpp"

using namespace std;
using namespace fpnn;

int main(int argc, const char** argv)
{
	CommandLineParser::init(argc, argv);
	std::vector<std::string> mainParams = CommandLineParser::getRestParams();
	
	if (mainParams.size() != 1)
	{
		cout<<"Usgae: "<<argv[0]<<" <endpoint> <-all | -onlyDir | -onlyFile> <path> [-pem public-key] [-udp]"<<endl;
		return 1;
	}

	ClientPtr client;
	if (CommandLineParser::exist("udp"))
	{
		client = UDPClient::createClient(mainParams[0]);
		if (CommandLineParser::exist("pem"))
		{
			std::string pemFile = CommandLineParser::getString("pem");
			if (((UDPClient*)(client.get()))->enableEncryptorByPemFile(pemFile.c_str()) == false)
			{
				cout<<"Invalid PEM file: "<<pemFile<<endl;
				return 1;
			}
		}
	}
	else
	{
		client = TCPClient::createClient(mainParams[0]);
		if (CommandLineParser::exist("pem"))
		{
			std::string pemFile = CommandLineParser::getString("pem");
			if (((TCPClient*)(client.get()))->enableEncryptorByPemFile(pemFile.c_str(), false, true) == false)
			{
				cout<<"Invalid PEM file: "<<pemFile<<endl;
				return 1;
			}
		}
	}

	FPQWriter qw(2, "ls");

	if (CommandLineParser::exist("all"))
	{
		qw.param("all", true);
		qw.param("path", CommandLineParser::getString("all"));
	}
	else if (CommandLineParser::exist("onlyDir"))
	{
		qw.param("onlyDir", true);
		qw.param("path", CommandLineParser::getString("onlyDir"));
	}
	else if (CommandLineParser::exist("onlyFile"))
	{
		qw.param("onlyFile", true);
		qw.param("path", CommandLineParser::getString("onlyFile"));
	}
	else
	{
		cout<<"[Error] Bad action."<<endl;
		return 1;
	}

	FPQuestPtr quest = qw.take();
	FPAnswerPtr answer = client->sendQuest(quest);
	FPAReader ar(answer);
	if (ar.getInt("code"))
	{
		cout<<"[Error] "<<answer->json()<<endl;
		return 2;
	}

	if (CommandLineParser::exist("all"))
	{
		std::vector<std::string> contentList;
		contentList = ar.want<std::vector<std::string>>("list", contentList);

		cout<<"Content list:"<<endl;
		cout<<"--------------------------------"<<endl;
		for (auto& item: contentList)
			cout<<"  "<<item<<endl;
	}
	else if (CommandLineParser::exist("onlyDir"))
	{
		std::vector<std::string> subDirectories;
		subDirectories = ar.want<std::vector<std::string>>("subDirs", subDirectories);

		cout<<"Subdirectories list:"<<endl;
		cout<<"--------------------------------"<<endl;
		for (auto& item: subDirectories)
			cout<<"  "<<item<<endl;
	}
	else if (CommandLineParser::exist("onlyFile"))
	{
		std::map<std::string, int64_t> fileMap;
		fileMap = ar.want<std::map<std::string, int64_t>>("files", fileMap);

		std::vector<std::string> fields = {"file name", "size"};
		std::vector<std::vector<std::string>> rows;

		for (auto& pp: fileMap)
		{
			std::vector<std::string> row = { pp.first, formatBytesQuantity(pp.second, 1) };
			rows.push_back(row);
		}

		printTable(fields, rows);
	}

	return 0;
}