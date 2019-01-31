#include "FPReader.h"
#include "hex.h"
#include "md5.h"

using namespace fpnn;

msgpack::object FPReader::_nilObj;
intmax_t FPReader::_intDef = 0;
uintmax_t FPReader::_uintDef = 0;
double FPReader::_doubleDef = 0.0;
std::string FPReader::_stringDef = std::string("");
bool FPReader::_boolDef = false;

const msgpack::object& FPReader::_find(const char* key){
	if(_object.via.map.size > 0){
		msgpack::object_kv*  pkv = _object.via.map.ptr;
		msgpack::object_kv*  pkv_end = _object.via.map.ptr + _object.via.map.size;

		do{
			if(pkv->key.type != msgpack::type::STR) 
				throw FPNN_ERROR_CODE_FMT(FpnnProtoError, FPNN_EC_PROTO_STRING_KEY, "Key type: %d, data: %s", pkv->key.type, json().c_str());

			if(strlen(key) == pkv->key.via.str.size &&
                                !strncmp(pkv->key.via.str.ptr, key, pkv->key.via.str.size)) return pkv->val;

			//-- Optimization Candidate:
			//-- if(key[pkv->key.via.str.size] == 0 &&
            //                    !strncmp(pkv->key.via.str.ptr, key, pkv->key.via.str.size)) return pkv->val;
			//-- Principle:
			//-- 1. If key length equal to pkv->key.via.str.ptr: strncmp() will judge.
			//-- 2. If key length large than pkv->key.via.str.ptr: key[pkv->key.via.str.size] != 0
			//-- 3. If key length less than pkv->key.via.str.ptr: in range 0..pkv->key.via.str.size, '\0' must exist.
			//--     And the '\0' must not exist in pkv->key.via.str.ptr. So, strncmp can judge it.

			++pkv;
		}while (pkv < pkv_end);
	}   

	throw FPNN_ERROR_CODE_FMT(FpnnProtoError, FPNN_EC_PROTO_KEY_NOT_FOUND, "KEY: %s NOT FOUND, data: %s", key, json().c_str());
}

bool FPReader::wantFile(const char* k, FileSystemUtil::FileAttrs& attrs){
	msgpack::object obj = wantObject(k);
	FPReader fpr(obj);
	attrs.name = fpr.wantString("name");
	attrs.sign = fpr.wantString("sign");
	attrs.content = fpr.wantString("content");
	attrs.ext = fpr.wantString("ext");
	attrs.size = fpr.wantInt("size");
	attrs.atime = fpr.wantInt("atime");
	attrs.mtime = fpr.wantInt("mtime");
	attrs.ctime = fpr.wantInt("ctime");

	std::cout<<"unpack len:"<<attrs.content.size() << " size:"<<attrs.size<<std::endl;
	//check md5
	unsigned char digest[16];
	md5_checksum(digest, attrs.content.data(), attrs.content.size());
	char hexstr[32 + 1];
	Hexlify(hexstr, digest, sizeof(digest));
	std::string sign = hexstr;
	std::cout<<"sign:"<<sign<<std::endl;
	std::cout<<"unpack sign:"<<attrs.sign<<std::endl;
	if(sign.compare(attrs.sign) != 0)
		throw FPNN_ERROR_CODE_FMT(FpnnProtoError, FPNN_EC_PROTO_FILE_SIGN, "File:%s, sign not same", attrs.name.c_str());
	return true;
}

bool FPReader::wantFile(const std::string& k, FileSystemUtil::FileAttrs& attrs){
	return wantFile(k.c_str(), attrs);
}

bool FPReader::getFile(const char* k, FileSystemUtil::FileAttrs& attrs){
	try{
		return wantFile(k, attrs);
	}   
	catch(const std::exception& ex){
		LOG_ERROR("get key:%s, exception:%s", k, ex.what());
	}   
	catch(...){
		LOG_ERROR("get key:%s, unknow exception", k);
	}   
	return false;
}

bool FPReader::getFile(const std::string& k, FileSystemUtil::FileAttrs& attrs){
	return getFile(k.c_str(), attrs);
}
