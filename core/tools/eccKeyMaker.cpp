#include <fstream>
#include <iostream>
#include <string>
#include <string.h>
#include "micro-ecc/uECC.h"
#include "PemGenerator.h"

using namespace std;

int main(int argc, const char* argv[])
{
	if (argc != 3)
	{
		cout<<"Usage: "<<argv[0]<<" <ecc-curve> key-pair-name"<<endl;
		cout<<"\tecc-curve:"<<endl;
		cout<<"\t\tsecp192r1"<<endl;
		cout<<"\t\tsecp224r1"<<endl;
		cout<<"\t\tsecp256r1"<<endl;
		cout<<"\t\tsecp256k1"<<endl;
		return 0;
	}

	int _secertLen;
	uECC_Curve _curve;

	if (strcmp(argv[1], "secp256k1") == 0)
	{
		_curve = uECC_secp256k1();
		_secertLen = 32;
	}
	else if (strcmp(argv[1], "secp256r1") == 0)
	{
		_curve = uECC_secp256r1();
		_secertLen = 32;
	}
	else if (strcmp(argv[1], "secp224r1") == 0)
	{
		_curve = uECC_secp224r1();
		_secertLen = 28;
	}
	else if (strcmp(argv[1], "secp192r1") == 0)
	{
		_curve = uECC_secp192r1();
		_secertLen = 24;
	}
	else
	{
		cout<<"Unsupported ECC curve."<<endl;
		return 0;
	}

	uint8_t privateKey[32];
	uint8_t publicKey[64];

	if (!uECC_make_key(publicKey, privateKey, _curve))
	{
		cout<<"Gen public key & private key failed."<<endl;
		return 0;
	}

	//-- Generated raw private & public key.

	std::ofstream prvOut(std::string(argv[2]).append("-private.key"), std::ofstream::binary);
	if(prvOut.is_open()) {
		prvOut.write((char*)privateKey, _secertLen);
		prvOut.close();
	}
	else
		cout<<"Create & write "<<argv[2]<<"-private.key failed."<<endl;

	std::ofstream pubOut(std::string(argv[2]).append("-public.key"), std::ofstream::binary);
	if(pubOut.is_open()) {
		pubOut.write((char*)publicKey, _secertLen * 2);
		pubOut.close();
	}
	else
		cout<<"Create & write "<<argv[2]<<"-public.key failed."<<endl;

	//-- Generated compressed public key.

	uint8_t compressedPublicKey[64];
	uECC_compress(publicKey, compressedPublicKey, _curve);

	std::ofstream comPubOut(std::string(argv[2]).append("-compressed-public.key"), std::ofstream::binary);
	if(comPubOut.is_open()) {
		comPubOut.write((char*)compressedPublicKey, _secertLen + 1);
		comPubOut.close();
	}
	else
		cout<<"Create & write "<<argv[2]<<"-compressed-public.key failed."<<endl;

	//-- Generated der & pem public key.

	PemGenerator pemGen;
	pemGen.build(argv[1], publicKey);

	std::ofstream derPubOut(std::string(argv[2]).append("-public.der"), std::ofstream::binary);
	if(derPubOut.is_open()) {
		std::string content = pemGen.getDerContent();
		derPubOut.write(content.data(), content.length());
		derPubOut.close();
	}
	else
		cout<<"Create & write "<<argv[2]<<"-public.der failed."<<endl;

	std::ofstream pemPubOut(std::string(argv[2]).append("-public.pem"), std::ofstream::binary);
	if(pemPubOut.is_open()) {
		std::string content = pemGen.getPemContent();
		pemPubOut.write(content.data(), content.length());
		pemPubOut.close();
	}
	else
		cout<<"Create & write "<<argv[2]<<"-public.pem failed."<<endl;

	cout<<"Completed!"<<endl;
	return 0;
}
