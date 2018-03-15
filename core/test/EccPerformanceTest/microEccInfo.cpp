// g++ -o microEccInfo microEccInfo.cpp ../../micro-ecc/uECC.c
#include <iostream>
#include "../../micro-ecc/uECC.h"

using namespace std;

int main()
{
	cout<<"uECC_secp192r1: private: "<<uECC_curve_private_key_size(uECC_secp192r1())<<", public: "<<uECC_curve_public_key_size(uECC_secp192r1())<<endl;
	cout<<"uECC_secp224r1: private: "<<uECC_curve_private_key_size(uECC_secp224r1())<<", public: "<<uECC_curve_public_key_size(uECC_secp224r1())<<endl;
	cout<<"uECC_secp256r1: private: "<<uECC_curve_private_key_size(uECC_secp256r1())<<", public: "<<uECC_curve_public_key_size(uECC_secp256r1())<<endl;
	cout<<"uECC_secp256k1: private: "<<uECC_curve_private_key_size(uECC_secp256k1())<<", public: "<<uECC_curve_public_key_size(uECC_secp256k1())<<endl;

	return 0;	
}
