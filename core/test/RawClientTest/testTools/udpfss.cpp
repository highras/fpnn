#include <iostream>
#include <string>
#include <string.h>
#include "FPLog.h"
#include "StringUtil.h"
#include "linenoise.h"
#include "../ProtocolClient/FpnnUDPClient.h"

using namespace std;
using namespace fpnn;

bool executeCommand(FpnnUDPClient& client, const std::string& cmd)
{
	size_t pos = cmd.find_first_of('{', 0);
	if (pos == std::string::npos)
		return false;

	std::string method = cmd.substr(0, pos);
	method = StringUtil::trim(method);
	
	std::string jsonBody = cmd.substr(pos);

	jsonBody = StringUtil::trim(jsonBody);
	if (jsonBody[0] != '{')
		return false;

	pos = jsonBody.find_last_of('}');
	if (pos == std::string::npos)
		return false;

	int timeout = 0;
	bool oneWay = false;
	if (pos + 1 < jsonBody.length())
	{
		std::string params = jsonBody.substr(pos + 1);
		jsonBody = jsonBody.substr(0, pos + 1);

		std::vector<std::string> paramVec;
		StringUtil::split(params, " ", paramVec);
		if (paramVec.size() > 2)
			return false;

		for (size_t i = 0; i < paramVec.size(); i++)
		{
			if (paramVec[i] == "oneway")
				oneWay = true;
			else if (strncmp(paramVec[i].c_str(), "timeout=", 8) == 0)
				timeout = std::stoi(paramVec[i].substr(8));
			else
				return false;
		}
	}

	FPQWriter qw(method, jsonBody, oneWay);
	FPAnswerPtr answer = client.sendQuest(qw.take(), timeout);
	if (!oneWay)
		cout<<"Return:"<<endl<<answer->json()<<endl;

	return true;
}

int main(int argc, const char* argv[])
{
	if (argc != 3)
	{
		cout<<"Usage: "<<argv[0]<<" ip port"<<endl;
		return 0;
	}

	FPLog::init("std::cout", "FPNN.TEST", "INFO", "udpfss");

	std::string endpoint(argv[1]);
	endpoint.append(":").append(argv[2]);

	FpnnUDPClient client(endpoint);

	cout<<"FPNN Secure Shell v1.1"<<endl;
	cout<<"Command format: method json-body [oneway] [timeout=xxx]"<<endl<<endl;

	char *rawline;
	while ((rawline = linenoise("FSS> ")) != NULL)
	{
		linenoiseHistoryAdd(rawline);
		linenoiseHistorySave(".fss.cmd.log");
		char *l = StringUtil::trim(rawline);
		std::string line(l);
		free(rawline);

		if (line.empty())
			continue;

		if (line == "exit" || line == "quit")
			break;

		if (!executeCommand(client, line))
			cout<<"Bad command."<<endl;
	}

	return 0;
}
