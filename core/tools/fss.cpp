#include <iostream>
#include <string>
#include <string.h>
#include "TCPClient.h"
#include "UDPClient.h"
#include "StringUtil.h"
#include "FileSystemUtil.h"
#include "CommandLineUtil.h"
#include "linenoise.h"

using namespace std;
using namespace fpnn;

void showUsage(const char* appName)
{
	cout<<"Usage: "<<appName<<" ip port"<<endl;
	cout<<"Usage: "<<appName<<" ip port -ssl"<<endl;
	cout<<"Usage: "<<appName<<" ip port -pem pem-file [-encrypt-mode encrypt-mode-opt] [-reinforce]"<<endl;
	cout<<"Usage: "<<appName<<" ip port -der der-file [-encrypt-mode encrypt-mode-opt] [-reinforce]"<<endl;
	cout<<"Usage: "<<appName<<" ip port -curve ecc-curve -rawKey raw-public-key-file [-encrypt-mode encrypt-mode-opt] [-reinforce]"<<endl;
	cout<<"Usage: "<<appName<<" ip port -udp"<<endl;
	cout<<"Usage: "<<appName<<" ip port -udp -pem pem-file [-packageReinforce] [-dataEnhance [-dataReinforce]]"<<endl;
	cout<<"Usage: "<<appName<<" ip port -udp -der der-file [-packageReinforce] [-dataEnhance [-dataReinforce]]"<<endl;
	cout<<"Usage: "<<appName<<" ip port -udp -curve ecc-curve -rawKey raw-public-key-file [-packageReinforce] [-dataEnhance [-dataReinforce]]"<<endl;

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
	cout<<"Default encrypt mode: package."<<endl;

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

ClientPtr buildTCPClient(const char* ip, int port)
{
	TCPClientPtr client = TCPClient::createClient(ip, port);

	if (CommandLineParser::exist("ssl"))
	{
		client->enableSSL();
		return client;
	}

	bool reinforce = CommandLineParser::getBool("reinforce", false);
	std::string mode = CommandLineParser::getString("encrypt-mode");
	bool package = true;
	if (mode == "package")
		package = true;
	else if (mode == "stream")
		package = false;
	
	if (CommandLineParser::exist("pem"))
	{
		std::string pemFile = CommandLineParser::getString("pem");
		if (client->enableEncryptorByPemFile(pemFile.c_str(), package, reinforce) == false)
		{
			cout<<"Invalid PEM file: "<<pemFile<<endl;
			return 0;
		}
	}
	else if (CommandLineParser::exist("der"))
	{
		std::string derFile = CommandLineParser::getString("der");
		if (client->enableEncryptorByDerFile(derFile.c_str(), package, reinforce) == false)
		{
			cout<<"Invalid DER file: "<<derFile<<endl;
			return 0;
		}
	}
	else if (CommandLineParser::exist("curve") && CommandLineParser::exist("rawKey"))
	{
		std::string curve = CommandLineParser::getString("curve");
		if (checkCurveName(curve.c_str()) == false)
		{
			cout<<"Invalid curve: "<<curve<<endl;
			return 0;
		}

		std::string key;
		std::string rawKeyFile = CommandLineParser::getString("rawKey");
		if (FileSystemUtil::readFileContent(rawKeyFile, key) == false)
		{
			cout<<"Read server public key file "<<rawKeyFile<<" failed!"<<endl;
			return 0;
		}

		client->enableEncryptor(curve, key, package, reinforce);
	}
	else if (CommandLineParser::getRestParams().size() > 0)
	{
		cout<<"Bad parameters."<<endl;
		return 0;
	}

	return client;
}

ClientPtr buildUDPClient(const char* ip, int port)
{
	UDPClientPtr client = UDPClient::createClient(ip, port);

	bool packageReinforce = CommandLineParser::getBool("packageReinforce", false);
	bool dataReinforce = CommandLineParser::getBool("dataReinforce", false);
	bool dataEnhance = CommandLineParser::getBool("dataEnhance", false);

	if (CommandLineParser::exist("pem"))
	{
		std::string pemFile = CommandLineParser::getString("pem");
		if (client->enableEncryptorByPemFile(pemFile.c_str(), packageReinforce, dataEnhance, dataReinforce) == false)
		{
			cout<<"Invalid PEM file: "<<pemFile<<endl;
			return 0;
		}
	}
	else if (CommandLineParser::exist("der"))
	{
		std::string derFile = CommandLineParser::getString("der");
		if (client->enableEncryptorByDerFile(derFile.c_str(), packageReinforce, dataEnhance, dataReinforce) == false)
		{
			cout<<"Invalid DER file: "<<derFile<<endl;
			return 0;
		}
	}
	else if (CommandLineParser::exist("curve") && CommandLineParser::exist("rawKey"))
	{
		std::string curve = CommandLineParser::getString("curve");
		if (checkCurveName(curve.c_str()) == false)
		{
			cout<<"Invalid curve: "<<curve<<endl;
			return 0;
		}

		std::string key;
		std::string rawKeyFile = CommandLineParser::getString("rawKey");
		if (FileSystemUtil::readFileContent(rawKeyFile, key) == false)
		{
			cout<<"Read server public key file "<<rawKeyFile<<" failed!"<<endl;
			return 0;
		}

		client->enableEncryptor(curve, key, packageReinforce, dataEnhance, dataReinforce);
	}
	else if (CommandLineParser::getRestParams().size() > 0)
	{
		cout<<"Bad parameters."<<endl;
		return 0;
	}

	return client;
}

ClientPtr buildClient(int argc, const char* argv[])
{
	if (argc < 3 || argc > 11)
	{
		showUsage(argv[0]);
		return 0;
	}

	CommandLineParser::init(argc, argv, 3);
	if (CommandLineParser::exist("udp"))
		return buildUDPClient(argv[1], atoi(argv[2]));
	else
		return buildTCPClient(argv[1], atoi(argv[2]));
}

bool executeCommand(ClientPtr client, const std::string& cmd)
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
	ClientPtr client = buildClient(argc, argv);
	if (!client)
		return 0;

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
