#include <iostream>
#include <string>
#include "FileSystemUtil.h"
#include "Setting.h"
#include "TCPClient.h"

using namespace std;
using namespace fpnn;

struct ClientEncryptConfig
{
	bool _encrypted;
	bool _package;
	bool _reinforce;
	std::string _eccCurve;
	std::string _serverKey;

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

public:
	ClientEncryptConfig(): _encrypted(false) {}

	bool enableEncryption(const std::string& configFile)
	{
		if(!Setting::load(configFile)){
			cout<<"Config file error:"<<configFile<<endl;
			return false;
		}

		_encrypted = true;

		std::string keyFile = Setting::getString("publicFile");
		if (FileSystemUtil::readFileContent(keyFile, _serverKey) == false)
		{
			cout<<"Read server public key file "<<keyFile<<" failed!"<<endl;
			return false;
		}

		_eccCurve = Setting::getString("curve");
		if (!checkCurveName(_eccCurve))
			return false;

		std::string mode = Setting::getString("encryptMode");
		if (mode == "package")
			_package = true;
		else if (mode == "stream")
			_package = false;
		else
		{
			cout<<"Bad encrypt mode"<<endl;
			return false;
		}

		std::string keyBits = Setting::getString("keyBits");
		if (keyBits == "128")
			_reinforce = false;
		else if (keyBits == "256")
			_reinforce = true;
		else
		{
			cout<<"Bad key Bits"<<endl;
			return false;
		}

		return true;
	}

	inline void process(TCPClientPtr client)
	{
		if (_encrypted)
			client->enableEncryptor(_eccCurve, _serverKey, _package, _reinforce);
	}
};