#include <iostream>
#include <string>
#include <string.h>
#include "TCPClient.h"
#include "StringUtil.h"
#include "FileSystemUtil.h"
#include "linenoise.h"

using namespace std;
using namespace fpnn;

void showUsage(const char* appName)
{
	cout<<"Usage: "<<appName<<" ip port"<<endl;
	cout<<"Usage: "<<appName<<" ip port -pem pem-file [encrypt-mode-opt] [encrypt-strength-opt]"<<endl;
	cout<<"Usage: "<<appName<<" ip port -der der-file [encrypt-mode-opt] [encrypt-strength-opt]"<<endl;
	cout<<"Usage: "<<appName<<" ip port ecc-curve raw-public-key-file [encrypt-mode-opt] [encrypt-strength-opt]"<<endl;

	cout<<"\tecc-curve:"<<endl;
	cout<<"\t\tsecp192r1"<<endl;
	cout<<"\t\tsecp224r1"<<endl;
	cout<<"\t\tsecp256r1"<<endl;
	cout<<"\t\tsecp256k1"<<endl;
	cout<<endl;
	cout<<"\tencrypt-mode-opt:"<<endl;
	cout<<"\t\tstream"<<endl;
	cout<<"\t\tpackage"<<endl;
	cout<<endl;
	cout<<"\tencrypt-strength-opt:"<<endl;
	cout<<"\t\t128bits"<<endl;
	cout<<"\t\t256bits"<<endl;
	cout<<endl;
	cout<<"Default encrypt mode: package, default encrypt strength: 128bits."<<endl;

	exit(0);
}

bool checkCurveName(const char* name)
{
	if (strcmp(name, "secp256k1") == 0)
		return true;
	else if (strcmp(name, "secp256r1") == 0)
		return true;
	else if (strcmp(name, "secp224r1") == 0)
		return true;
	else if (strcmp(name, "secp192r1") == 0)
		return true;
	
	cout<<"Bad curve name!"<<endl;
	return false;
}

TCPClientPtr buildClient(int argc, const char* argv[])
{
	if (argc < 3 || argc > 7)
	{
		showUsage(argv[0]);
		return 0;
	}

	TCPClientPtr client = TCPClient::createClient(argv[1], atoi(argv[2]));
	if (argc == 3)
		return client;

	if (argc < 5)
	{
		showUsage(argv[0]);
		return 0;
	}

	bool package = true;
	bool reinforce = false;
	for (int i = 5; i < argc; i++)
	{
		if (strcmp(argv[i], "package") == 0)
			package = true;
		else if (strcmp(argv[i], "stream") == 0)
			package = false;
		else if (strcmp(argv[i], "128bits") == 0)
			reinforce = false;
		else if (strcmp(argv[i], "256bits") == 0)
			reinforce = true;
		else
		{
			cout<<"Bad parameter: "<<argv[i]<<endl;
			showUsage(argv[0]);
			return 0;
		}
	}

	if (strcmp("-pem", argv[3]) == 0)
	{
		if (client->enableEncryptorByPemFile(argv[4], package, reinforce) == false)
			return 0;
	}
	else if (strcmp("-der", argv[3]) == 0)
	{
		if (client->enableEncryptorByDerFile(argv[4], package, reinforce) == false)
			return 0;
	}
	else
	{
		if (checkCurveName(argv[3]) == false)
		{
			showUsage(argv[0]);
			return 0;
		}
		std::string key;
		if (FileSystemUtil::readFileContent(argv[4], key) == false)
		{
			cout<<"Read server public key file "<<argv[4]<<" failed!"<<endl;
			return 0;
		}

		client->enableEncryptor(argv[3], key, package, reinforce);
	}
	
	return client;
}

bool executeCommand(TCPClientPtr client, const std::string& cmd)
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
	FPAnswerPtr answer = client->sendQuest(qw.take(), timeout);
	if (!oneWay)
		cout<<"Return:"<<endl<<answer->json()<<endl;

	return true;
}

int main(int argc, const char* argv[])
{
	cout<<"FPNN Secure Shell v1.0"<<endl;

	TCPClientPtr client = buildClient(argc, argv);
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
