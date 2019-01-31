#include <sstream>
#include "FPMessage.h"
#include "JSONConvert.h"
#include "httpcode.h"
#include "msec.h"
#include "TimeUtil.h"
#include "base64.h"
#include "sha1.h"
#include "gzpipe.h"

using namespace fpnn;

FPMessage::fpnn_magic FPMessage::_TCP_magic = { {'F','P','N','N'} };
FPMessage::fpnn_magic FPMessage::_POST_magic = { {'P','O','S','T'} };
FPMessage::fpnn_magic FPMessage::_GET_magic = { {'G','E','T',' '} };

FPMessage::Header FPMessage::_default_header = { {{'F','P','N','N'}}, FPNN_PROTO_CURRENT_VERSION, 0, 0, 0, 0 };

const char* FPMessage::_CRLF = "\r\n";
const int16_t FPMessage::_CRLF_LEN = 2;
std::string FPMessage::_emptyString = std::string("");

const int16_t FPMessage::_MagicFieldLength = sizeof(fpnn_magic);
const int16_t FPMessage::_HeaderLength = sizeof(Header);

uint32_t FPMessage::BodyLen(const char* header){ 
	Header *hdr = (Header*)header;
	uint32_t len = le32toh(hdr->psize);

	if(hdr->mtype == FP_MT_TWOWAY){
		len += hdr->ss;
		len += sizeof(uint32_t);//seq 
	}
	else if(hdr->mtype == FP_MT_ANSWER){
		len += sizeof(uint32_t);//seq
	}
	else if(hdr->mtype == FP_MT_ONEWAY){
		len += hdr->ss;
	}
	else 
		throw FPNN_ERROR_CODE_FMT(FpnnProtoError, FPNN_EC_PROTO_METHOD_TYPE, "Unknow mtype:%d", hdr->mtype);
	return len;
}
bool FPMessage::isQuest(const char* header){
	Header *hdr = (Header*)header;
	if(hdr->mtype == FP_MT_TWOWAY || hdr->mtype == FP_MT_ONEWAY){
		return true;
	}
	else if(hdr->mtype == FP_MT_ANSWER){
		return false;
	}
	else 
		throw FPNN_ERROR_CODE_FMT(FpnnProtoError, FPNN_EC_PROTO_METHOD_TYPE, "Unknow mtype:%d", hdr->mtype);
}

std::string FPMessage::json(){
	try{
		return JSONConvert::Msgpack2Json(_payload);
	}
	catch(const std::exception& ex){
		LOG_ERROR("EXCEPTION:%s", ex.what());
	}
	catch(...){
		LOG_ERROR("Unknow Exception");
	}
	return "";
}

void FPMessage::printHttpInfo(){
	if(!_httpInfos) return;
	for(StringMap::iterator it = _httpInfos->begin(); it != _httpInfos->end(); ++it){
		LOG_INFO("%s=>%s", it->first.c_str(), it->second.c_str());	
	}
}

void FPQuest::_create(const std::string& method, bool oneway, FP_Pack_Type ptype){
	_hdr = _default_header;
	if(ptype == FP_PACK_MSGPACK) setFlag(FP_FLAG_MSGPACK);
	else if(ptype == FP_PACK_JSON) setFlag(FP_FLAG_JSON);
	else throw FPNN_ERROR_CODE_FMT(FpnnProtoError, FPNN_EC_PROTO_PROTO_TYPE, "Create Quest for unknow ptype:%d", ptype);

	setMType(oneway?FP_MT_ONEWAY:FP_MT_TWOWAY);
	setSS(method.size());
	if(!oneway) setSeqNum(nextSeqNum());
	setMethod(method);
}

void FPQuest::_create(const Header& hdr, uint32_t seq, const std::string& method, const std::string& payload){
	_hdr = hdr;
	if(isTwoWay()) setSeqNum(seq);
	else if(!isQuest()) 
		throw FPNN_ERROR_CODE_MSG(FpnnProtoError, FPNN_EC_PROTO_METHOD_TYPE, "Create Quest from raw, But not a quest package");

	if(!isSupportPack()) 
		throw FPNN_ERROR_CODE_MSG(FpnnProtoError, FPNN_EC_PROTO_NOT_SUPPORTED, "Create Quest from raw, Not Json OR Msgpack");
	if(!isSupportProto()) 
		throw FPNN_ERROR_CODE_MSG(FpnnProtoError, FPNN_EC_PROTO_NOT_SUPPORTED, "Create Quest from raw, Not TCP OR HTTP");

	setMethod(method);
	if(isMsgPack()){
		setPayload(payload);
	}
	else{
		setPayload(JSONConvert::Json2Msgpack(payload));
	}
	setPayloadSize(this->payload().size());
}

void FPQuest::_create(const char* data, size_t len){
	if(len < sizeof(_hdr)) 
		throw FPNN_ERROR_CODE_FMT(FpnnProtoError, FPNN_EC_PROTO_INVALID_PACKAGE, "hdr len:%d, but intput len:%d", sizeof(_hdr), len);
	const char* p = data;
	memcpy(&_hdr, p, sizeof(_hdr));
	p += sizeof(_hdr);
	len -= sizeof(_hdr);
	if(len <= 0) 
		throw FPNN_ERROR_CODE_FMT(FpnnProtoError, FPNN_EC_PROTO_INVALID_PACKAGE, "len is too small:%d", len);
	if(!isSupportPack()) 
		throw FPNN_ERROR_CODE_MSG(FpnnProtoError, FPNN_EC_PROTO_NOT_SUPPORTED, "Create Quest from raw, Not Json OR Msgpack");
	if(!isSupportProto()) 
		throw FPNN_ERROR_CODE_MSG(FpnnProtoError, FPNN_EC_PROTO_NOT_SUPPORTED, "Create Quest from raw, Not TCP OR HTTP");

	if(isTwoWay()){
		uint32_t seq;
		memcpy(&seq, p, sizeof(seq));
		p += sizeof(seq);
		len -= sizeof(seq);
		if(len <= 0) 
			throw FPNN_ERROR_CODE_FMT(FpnnProtoError, FPNN_EC_PROTO_INVALID_PACKAGE, "len is too small:%d", len);
		setSeqNum(seq);
	}
	else if(!isQuest()) 
		throw FPNN_ERROR_CODE_MSG(FpnnProtoError, FPNN_EC_PROTO_NOT_SUPPORTED, "Create Quest from raw, Not Quest package");
	uint8_t method_len = ss();
	if(method_len == 0)
		throw FPNN_ERROR_CODE_FMT(FpnnProtoError, FPNN_EC_PROTO_INVALID_PACKAGE, "Empty method");

	if(len < method_len) 
		throw FPNN_ERROR_CODE_FMT(FpnnProtoError, FPNN_EC_PROTO_INVALID_PACKAGE, "len is too small:%d", len);
	setMethod(std::string(p, method_len));

	p += method_len;
	len -= method_len;
	if(len != payloadSize()) 
		throw FPNN_ERROR_CODE_FMT(FpnnProtoError, FPNN_EC_PROTO_INVALID_PACKAGE, "len is too small:%d", len);
	std::string payload = std::string(p, len);

	if(isMsgPack()){
		setPayload(payload);
	}
	else{
		setPayload(JSONConvert::Json2Msgpack(payload));
	}
	setPayloadSize(this->payload().size());
}

void FPQuest::_create(const std::string& method, const std::string& payload, StringMap& infos, bool post){
	if(post)
		_hdr.magic = _POST_magic;
	else
		_hdr.magic = _GET_magic;

	_hdr.mtype = FP_MT_TWOWAY;

	setMethod(method);
	setFlag(FP_FLAG_JSON);
	//must be JSON
	setPayload(JSONConvert::Json2Msgpack(payload));
	_httpInfos = new StringMap;
	_httpInfos->swap(infos);
}

std::string* FPQuest::raw(){
	msgpack::sbuffer ss(FPNN_MSGPACK_SBUFFER_INIT_SIZE);
	std::string pl = payload();

	size_t oplSize = pl.size();
	if(isJson()) pl = JSONConvert::Msgpack2Json(pl);
	setPayloadSize(pl.size());
	ss.write((const char*)&_hdr, sizeof(_hdr));
	setPayloadSize(oplSize);//for next call of raw
	
	if(isTwoWay()){
		uint32_t seqnum = seqNumLE();
		ss.write((const char*)&seqnum, sizeof(uint32_t));
	}
	else if(!isQuest()) 
		throw FPNN_ERROR_CODE_FMT(FpnnProtoError, FPNN_EC_PROTO_NOT_SUPPORTED, "get RAW data of Quest, but it not a quest package");

	ss.write((const char*)method().data(), method().size());

	ss.write((const char*)pl.data(), pl.size());

	return new std::string(ss.data(),ss.size());
}

std::string FPQuest::info(){
	std::ostringstream os; 
	os << "Quest, seqID(" << seqNum() << "),TCP(";
	os << isTCP()<<"),Method(";
	os << method()<<"),body("<<json()<<")";
	return os.str();
}

void FPAnswer::_create(){
	if(!_quest) 
		throw FPNN_ERROR_CODE_MSG(FpnnProtoError, FPNN_EC_PROTO_UNKNOWN_ERROR, "Create answer, But quest is NULL");
	_hdr = _quest->_hdr;
	if(_quest->isHTTP()) {
		//HTTP DO NOT SUPPORT MSGPACK
		if(_status == FP_ST_OK)
			_status = FP_ST_HTTP_OK;
		const char* desc = httpcode_description(_status);
		if(strlen(desc) == 0) 
			_status = FP_ST_HTTP_ERROR;

		return; //return directly
	}

	if(_status == FP_ST_HTTP_OK)
		_status = FP_ST_OK;
	if(_status != FP_ST_OK)
		_status = FP_ST_ERROR;

	if(!_quest->isTwoWay()) 
		throw FPNN_ERROR_CODE_MSG(FpnnProtoError, FPNN_EC_PROTO_UNKNOWN_ERROR, "Create answer for oneway Message");
	if(!isSupportPack()) 
		throw FPNN_ERROR_CODE_MSG(FpnnProtoError, FPNN_EC_PROTO_NOT_SUPPORTED, "Create answer, Not Json OR Msgpack");
	if(!isSupportProto()) 
		throw FPNN_ERROR_CODE_MSG(FpnnProtoError, FPNN_EC_PROTO_NOT_SUPPORTED, "Create answer, Require TCP");

	setMType(FP_MT_ANSWER);
	setSS(_status);
	setSeqNum(_quest->seqNum());
}

void FPAnswer::_create(const Header& hdr, uint32_t seq, const std::string& payload){
	//if(!_quest) throw FPNN_ERROR_MSG(FpnnProtoError, "Create answer, But quest is NULL");
	//if(!_quest->isTwoWay()) FPNN_ERROR_MSG(FpnnProtoError, "Create answer for oneway Message");
	_hdr = hdr;
	if(!isSupportPack()) 
		throw FPNN_ERROR_CODE_MSG(FpnnProtoError, FPNN_EC_PROTO_NOT_SUPPORTED, "Create answer from raw, But Not Json OR Msgpack");
	if(!isSupportProto()) 
		throw FPNN_ERROR_CODE_MSG(FpnnProtoError, FPNN_EC_PROTO_NOT_SUPPORTED, "Create answer from raw, Not TCP OR HTTP");

	_status = ss();
	setSeqNum(seq);

	if(isMsgPack()){
		setPayload(payload);
	}
	else{
		setPayload(JSONConvert::Json2Msgpack(payload));
	}
	setPayloadSize(this->payload().size());
}

void FPAnswer::_create(const char* data, size_t len){
	//if(!_quest) throw FPNN_ERROR_MSG(FpnnProtoError, "Create answer, But quest is NULL");
	//if(!_quest->isTwoWay()) FPNN_ERROR_MSG(FpnnProtoError, "Create answer for oneway Message");
	if(len < sizeof(_hdr)) 
		throw FPNN_ERROR_CODE_FMT(FpnnProtoError, FPNN_EC_PROTO_INVALID_PACKAGE, "Len is too small:%d", len);
	const char* p = data;
	memcpy(&_hdr, p, sizeof(_hdr));
	p += sizeof(_hdr);
	len -= sizeof(_hdr);
	if(len <= 0) 
		throw FPNN_ERROR_CODE_FMT(FpnnProtoError, FPNN_EC_PROTO_INVALID_PACKAGE, "Len is too small:%d", len);
	_status = ss();
	if(!isSupportPack()) 
		throw FPNN_ERROR_CODE_MSG(FpnnProtoError, FPNN_EC_PROTO_NOT_SUPPORTED, "Create answer from raw, But Not Json OR Msgpack");
	if(!isSupportProto()) 
		throw FPNN_ERROR_CODE_MSG(FpnnProtoError, FPNN_EC_PROTO_NOT_SUPPORTED, "Create answer from raw, Not TCP OR HTTP");

	uint32_t seq;
	memcpy(&seq, p, sizeof(seq));
	p += sizeof(seq);
	len -= sizeof(seq);
	if(len <= 0) 
		throw FPNN_ERROR_CODE_FMT(FpnnProtoError, FPNN_EC_PROTO_INVALID_PACKAGE, "Len is too small:%d", len); 
	setSeqNum(seq);
	if(len != payloadSize()) 
		throw FPNN_ERROR_CODE_FMT(FpnnProtoError, FPNN_EC_PROTO_INVALID_PACKAGE, "Len is too small:%d", len);

	std::string payload = std::string(p, len);

	if(isMsgPack()){
		setPayload(payload);
	}
	else{
		setPayload(JSONConvert::Json2Msgpack(payload));
	}
	setPayloadSize(this->payload().size());
}

std::string* FPAnswer::raw(){
	if(isTCP()) return rawTCP();
	return rawHTTP();
}

std::string* FPAnswer::rawTCP(){
	msgpack::sbuffer ss(FPNN_MSGPACK_SBUFFER_INIT_SIZE);
	std::string pl = payload();

	size_t oplSize = pl.size();
	if(isJson()) pl = JSONConvert::Msgpack2Json(pl);
	setPayloadSize(pl.size());
	ss.write((const char*)&_hdr, sizeof(_hdr));
	setPayloadSize(oplSize);//for next call of raw

	uint32_t seqnum = seqNumLE();
	ss.write((const char*)&seqnum, sizeof(uint32_t));

	ss.write((const char*)pl.data(), pl.size());

	return new std::string(ss.data(),ss.size());
}

std::string* FPAnswer::rawHTTP(){
	msgpack::sbuffer ss(FPNN_MSGPACK_SBUFFER_INIT_SIZE);
	std::string pl = payload();
	pl = JSONConvert::Msgpack2Json(pl);

	size_t len = pl.size();

	std::string websocket = _quest->getWebSocket();
	uint16_t status = _status;	
	if(websocket.size() > 0)
		status = 101;

	std::ostringstream header;
	header << "HTTP/1.1 "<< status <<" " << httpcode_description(status) << _CRLF;
	if(websocket.empty())
	{
		header <<
			"Date: " << TimeUtil::getTimeRFC1123() << _CRLF <<
			"Server: FPNN/1.0" << _CRLF <<
			"Content-Length: " << len << _CRLF <<
			"Connection: Keep-Alive" << _CRLF;
		if(_quest->http_header("Origin").size()){
			header << "Access-Control-Allow-Origin: *" << _CRLF 
				<< "Access-Control-Allow-Credentials: true" << _CRLF
				<< "Access-Control-Allow-Methods: GET, POST" << _CRLF; 
		}
	}
	else {
		header << "Upgrade: websocket" << _CRLF 
			<< "Connection: Upgrade" << _CRLF;
		std::string acceptKey = genWebsocketKey(websocket);
		header << "Sec-WebSocket-Accept: " << acceptKey << _CRLF;
		std::string SecWebSocketProtocol = _quest->http_header("Sec-WebSocket-Protocol");
		if(SecWebSocketProtocol.size())
			header << "Sec-WebSocket-Protocol: " << SecWebSocketProtocol << _CRLF;
	}
	header <<_CRLF;

	ss.write((const char*)header.str().c_str(), static_cast<uint32_t>(header.str().size()));

	if (websocket.size() == 0)
		ss.write((const char*)pl.data(), pl.size());

	return new std::string(ss.data(),ss.size());
}

int64_t FPAnswer::timeCost(){
	if(!_quest) return 0;
	return _ctime - _quest->ctime();
}

std::string FPAnswer::info(){
	std::ostringstream os; 
	os << "Answer, Status("<<_status<<"),seqID(" << (_quest ? _quest->seqNum() : 0) << "),TCP(";
	os << (_quest ? _quest->isTCP() : false) <<"),Method(";
	os << (_quest ? _quest->method() : "null")<<"),body("<<json()<<")";
	return os.str();
}

std::string FPAnswer::genWebsocketKey(const std::string& access){
	std::string strKey = access + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
	unsigned char digest[20];
	sha1_checksum(digest, strKey.data(), strKey.length());

	char buf[128] = {0};
	base64_t b64;
	base64_init(&b64, (const char *)std_base64.alphabet);
	int32_t len = base64_encode(&b64, buf, digest, 20, BASE64_AUTO_NEWLINE);
	return std::string(buf, len);
}
