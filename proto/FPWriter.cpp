#include "FPWriter.h"
#include "FPLog.h"
#include "gzpipe.h"

using namespace fpnn;

void FPWriter::paramFormat(const char *fmt, ...){
	va_list ap; 
	va_start(ap, fmt);
	std::string v = fmtString(fmt, ap);
	va_end(ap);
	_pack.pack(v);
}

void FPWriter::paramFormat(const char *k, const char *fmt, ...){
	va_list ap; 
	va_start(ap, fmt);
	std::string v = fmtString(fmt, ap);
	va_end(ap);
	_pack.pack(k); 
	_pack.pack(v);
}
void FPWriter::paramFormat(const std::string& k, const char *fmt, ...){
	va_list ap; 
	va_start(ap, fmt);
	std::string v = fmtString(fmt, ap);
	va_end(ap);
	_pack.pack(k); 
	_pack.pack(v);
}

void FPWriter::paramFile(const char *k, const char *file){
	if(!k || !file){
		throw FPNN_ERROR_CODE_FMT(FpnnProtoError, FPNN_EC_PROTO_UNKNOWN_ERROR, "NULL k or filename");
	}
	FileSystemUtil::FileAttrs attrs;
	if(!FileSystemUtil::readFileAndAttrs(file, attrs)){
		throw FPNN_ERROR_CODE_FMT(FpnnProtoError, FPNN_EC_PROTO_FILE_NOT_EXIST, "Can not get file attrs, name: %s", file);
	}

	paramMap(k, 8);
	param("name", attrs.name);
	param("content", attrs.content);
	param("sign", attrs.sign);
	param("ext", attrs.ext);
	param("size", attrs.size);
	param("atime", attrs.atime);
	param("mtime", attrs.mtime);
	param("ctime", attrs.ctime);
	
	std::cout<<"pack len:"<<attrs.content.size() << " size:"<<attrs.size<<std::endl;
	return;
}

std::string FPWriter::json(){
	try{
		return JSONConvert::Msgpack2Json(_sbuf.data(), _sbuf.size());
	}
	catch(const std::exception& ex){
		LOG_ERROR("EXCEPTION:%s", ex.what());
	}
	catch(...){
		LOG_ERROR("Unknow EXCEPTION");
	}
	return "";
}

FPQuestPtr FPQWriter::CloneQuest(const char* method, const FPQuestPtr quest){
	FPQuestPtr q(new FPQuest(method, quest->isOneWay(), quest->isMsgPack() ? FPMessage::FP_PACK_MSGPACK : FPMessage::FP_PACK_JSON));
	std::string payload = quest->payload();
	q->setPayload(payload);
	q->setPayloadSize(payload.size());
	q->setCTime(slack_real_msec());
	return q;
}

FPQuestPtr FPQWriter::CloneQuest(const std::string& method, const FPQuestPtr quest){
	return CloneQuest(method.c_str(), quest);
}

FPQuestPtr FPQWriter::take(){
	std::string payload = raw();
	_quest->setPayload(payload);
	_quest->setPayloadSize(payload.size());
	_quest->setCTime(slack_real_msec());

	FPQuestPtr q;
	_quest.swap(q);
	return q;
}

FPQuestPtr FPQWriter::emptyQuest(const char *method, bool oneway, FPMessage::FP_Pack_Type ptype){
	FPQWriter qw((size_t)0, method, oneway, ptype);
	return qw.take();
}

FPQuestPtr FPQWriter::emptyQuest(const std::string& method, bool oneway, FPMessage::FP_Pack_Type ptype){
	FPQWriter qw((size_t)0, method, oneway, ptype);
	return qw.take();
}

FPAnswerPtr FPAWriter::CloneAnswer(const FPAnswerPtr answer, const FPQuestPtr quest){
	FPAnswerPtr an(new FPAnswer(quest));
	an->setSS(answer->status());
	std::string payload = answer->payload();
	an->setPayload(payload);
	an->setPayloadSize(payload.size());
	an->setCTime(slack_real_msec());
	return an;
}

FPAnswerPtr FPAWriter::take(){
	std::string payload = raw();
	_answer->setPayload(payload);
	_answer->setPayloadSize(payload.size());
	_answer->setCTime(slack_real_msec());

	FPAnswerPtr a;
	_answer.swap(a);
	return a;
}

FPAnswerPtr FPAWriter::errorAnswer(const FPQuestPtr quest, int code, const std::string& ex, const std::string& raiser){
	return errorAnswer(quest, code, ex.c_str(), raiser.c_str());
}

FPAnswerPtr FPAWriter::errorAnswer(const FPQuestPtr quest, int code, const char* ex, const char* raiser){
	FPAWriter aw(3, FPAnswer::FP_ST_ERROR, quest);
	aw.param("code", code);
	aw.param("ex", ex);
	aw.param("raiser", raiser);
	LOG_ERROR("ERROR ANSWER: code(%d), exception(%s), raiser(%s), QUEST(%s)", code, ex, raiser, quest->info().c_str());
	return aw.take();
}

FPAnswerPtr FPAWriter::emptyAnswer(const FPQuestPtr quest){
	FPAWriter aw((size_t)0, quest);
	return aw.take();
}
