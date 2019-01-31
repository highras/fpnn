#include <iostream>
#include <vector>
#include <thread>
#include <assert.h>
#include "TCPClient.h"
#include "FileSystemUtil.h"
#include "CommandLineUtil.h"

using namespace std;
using namespace fpnn;

bool checkCurveName(const std::string& curve)
{
	if (curve == "secp256k1")
		return true;
	else if (curve == "secp256r1")
		return true;
	else if (curve == "secp224r1")
		return true;
	else if (curve == "secp192r1")
		return true;
	
	cout<<"Bad curve name!"<<endl;
	return false;
}

bool prepareEncrypt(TCPClientPtr client)
{
	if (CommandLineParser::exist("ssl"))
	{
		client->enableSSL();
		return true;
	}
	else if (CommandLineParser::exist("ecc-pem"))
	{
		std::string pemFile = CommandLineParser::getString("ecc-pem");
		if (client->enableEncryptorByPemFile(pemFile.c_str(), false, true) == false)
		{
			cout<<"Enable Ecc Encrypt with PEM file "<<pemFile<<" failed."<<endl;
			return false;
		}
	}
	else if (CommandLineParser::exist("ecc-der"))
	{
		std::string derFile = CommandLineParser::getString("ecc-der");
		if (client->enableEncryptorByDerFile(derFile.c_str(), false, true) == false)
		{
			cout<<"Enable Ecc Encrypt with DER file "<<derFile<<" failed."<<endl;
			return false;
		}
	}
	else if (CommandLineParser::exist("ecc-curve"))
	{
		std::string curve = CommandLineParser::getString("ecc-curve");
		if (!checkCurveName(curve))
			return false;

		std::string rawKeyFile = CommandLineParser::getString("ecc-raw-key");

		std::string key;
		if (FileSystemUtil::readFileContent(rawKeyFile, key) == false)
		{
			cout<<"Read server raw public key file "<<rawKeyFile<<" failed!"<<endl;
			return false;
		}
		client->enableEncryptor(curve, key, false, true);
	}
	return true;
}

int main(int argc, char* argv[])
{
	CommandLineParser::init(argc, argv);
	std::vector<std::string> mainParams = CommandLineParser::getRestParams();
	
	if (mainParams.size() != 4)
	{
		cout<<"Usage: "<<argv[0]<<" ip port method body(json) [-ssl] [-json] [-oneway] [-t timeout]"<<endl;
		cout<<"Usage: "<<argv[0]<<" ip port method body(json) [-ecc-pem ecc-pem-file] [-json] [-oneway] [-t timeout]"<<endl;
		cout<<"Usage: "<<argv[0]<<" ip port method body(json) [-ecc-der ecc-der-file] [-json] [-oneway] [-t timeout]"<<endl;
		cout<<"Usage: "<<argv[0]<<" ip port method body(json) [-ecc-curve ecc-curve-name -ecc-raw-key ecc-raw-public-key-file] [-json] [-oneway] [-t timeout]"<<endl;
		return 0;
	}

	std::string ip = mainParams[0];
	int port = std::stoi(mainParams[1]);
	std::string method = mainParams[2];
	std::string jsonBody = mainParams[3];

	bool isTwoWay = CommandLineParser::exist("oneway");
	bool isMsgPack = !CommandLineParser::exist("json");
	int timeout = CommandLineParser::getInt("t", 0);

	std::shared_ptr<TCPClient> client = TCPClient::createClient(ip, port);
	if (!prepareEncrypt(client))
		return 0;

	FPQWriter qw(method, jsonBody, isTwoWay, isMsgPack ? FPMessage::FP_PACK_MSGPACK : FPMessage::FP_PACK_JSON);
	FPQuestPtr quest = qw.take();

	FPAnswerPtr answer = client->sendQuest(quest, timeout);
	if (quest->isTwoWay()) {
		assert(quest->seqNum() == answer->seqNum());
		cout<<"Return:"<<answer->json()<<endl;
	}

	client->close();
	return 0;
}
