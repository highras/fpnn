#include <string.h>
#include <stdlib.h>
#include <iostream>
#include "rijndael.h"

using namespace std;


int main()
{

	//const char* key = "qazwsxedcrfvtgbyhnujmikolp123456";
	const char* key = "aaaaaaaaaaaaaaaa";
	//const char* iv = "qazwsxedcrfvtgby";
	const char* iv = "aaaaaaaaaaaaaaaa";

	uint8_t iva[16];
	uint8_t ivb[16];

	memcpy(iva, iv, 16);
	memcpy(ivb, iv, 16);

	rijndael_context enCtx;
	rijndael_context deCtx;

	rijndael_setup_encrypt(&enCtx, (const uint8_t *)key, 16);
	rijndael_setup_encrypt(&deCtx, (const uint8_t *)key, 16);

	const char* data = "dasdsa as dsadasd sadsad eesewfsfsf sdfdssfsdfsdf wrwerfw fea fsfdfdsf ewrsfds";
	size_t srclen = strlen(data);
	char *buf = (char *)malloc(srclen);
	char *buf2 = (char *)malloc(srclen);
	size_t pos = 0;

	rijndael_cfb_encrypt(&enCtx, true, (const uint8_t *)data, (uint8_t *)buf, srclen, iva, &pos);

	pos = 0;
	rijndael_cfb_encrypt(&deCtx, false, (const uint8_t *)buf, (uint8_t *)buf2, srclen, ivb, &pos);

	cout<<"compare: "<<memcmp(data, buf2, srclen)<<endl;

	return 0;
}
