#ifndef FPWriter_h_
#define FPWriter_h_
#include <sstream>
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <stdint.h>
#include <msgpack.hpp>
#include <stdarg.h>
#include "FPMessage.h"
#include "JSONConvert.h"
#include "FileSystemUtil.h" 

namespace fpnn{

#define FPNN_MAX_FMT_LEN 2048

class FPWriter{
	public:
		template<typename VALUE>
			void param(const VALUE& v) { _pack.pack(v); }

		void paramFormat(const char *fmt, ...);
		void paramBinary(const void *data, size_t len){
			_pack.pack_bin(len);
			_pack.pack_bin_body((const char*)data,len);
		}
		void paramNull(){
			_pack.pack_nil();
		}
		void paramArray(const char* k, size_t size){
			_pack.pack(k); _pack.pack_array(size);
		}
		void paramArray(const std::string& k, size_t size){
			_pack.pack(k); _pack.pack_array(size);
		}
		void paramArray(size_t size){
			_pack.pack_array(size);
		}
		void paramMap(const char* k, size_t size){
			_pack.pack(k); _pack.pack_map(size);
		} 
		void paramMap(const std::string& k, size_t size){
			_pack.pack(k); _pack.pack_map(size);
		} 
		void paramMap(size_t size){
			_pack.pack_map(size);
		} 

		template<typename VALUE>
			void param(const char *k, const VALUE& v) {
				_pack.pack(k); _pack.pack(v);
			}
		template<typename VALUE>
			void param(const std::string& k, const VALUE& v) { 
				_pack.pack(k); _pack.pack(v);
			}

		void paramFormat(const char *k, const char *fmt, ...);
		void paramFormat(const std::string& k, const char *fmt, ...);

		void paramBinary(const char *k, const void *data, size_t len){
			_pack.pack(k); 
			_pack.pack_bin(len);
			_pack.pack_bin_body((const char*)data,len);
		}
		void paramBinary(const std::string& k, const void *data, size_t len){
			paramBinary(k.c_str(), data, len);
		}

		void paramNull(const char *k){
			_pack.pack(k); _pack.pack_nil();
		}
		void paramNull(const std::string& k){
			_pack.pack(k); _pack.pack_nil();
		}

		void paramFile(const std::string& k, const std::string& file){
			paramFile(k.c_str(), file.c_str());
		}

		void paramFile(const char *k, const char *file);

		virtual ~FPWriter() {}

		FPWriter(uint32_t size):_sbuf(FPNN_MSGPACK_SBUFFER_INIT_SIZE), _pack(&_sbuf){
			_pack.pack_map(size);
		}
		//only support pack a map
		FPWriter():_sbuf(FPNN_MSGPACK_SBUFFER_INIT_SIZE), _pack(&_sbuf){
		}

		//only support pack raw JSON
		FPWriter(const std::string& json):_sbuf(FPNN_MSGPACK_SBUFFER_INIT_SIZE), _pack(&_sbuf){
			std::string msgpack = JSONConvert::Json2Msgpack(json);
			_sbuf.write((const char*)msgpack.data(), msgpack.size());
		}
		FPWriter(const char* json):_sbuf(FPNN_MSGPACK_SBUFFER_INIT_SIZE), _pack(&_sbuf){
			std::string msgpack = JSONConvert::Json2Msgpack(json);
			_sbuf.write((const char*)msgpack.data(), msgpack.size());
		}
	public:
		std::string raw(){
			return std::string(_sbuf.data(), _sbuf.size());
		}
		std::string json();
	private:
		std::string fmtString(const char *fmt, va_list ap){
			char v[FPNN_MAX_FMT_LEN+1] = {0};
			vsnprintf(v, FPNN_MAX_FMT_LEN, fmt, ap);
			return std::string(v);
		}

	private:
		msgpack::sbuffer _sbuf;
		msgpack::packer<msgpack::sbuffer> _pack;
};


class FPQWriter : public FPWriter{
	public:
		static FPQuestPtr CloneQuest(const char* method, const FPQuestPtr quest);
		static FPQuestPtr CloneQuest(const std::string& method, const FPQuestPtr quest);

	public:
		FPQWriter(size_t size, const char *method, bool oneway=false, FPMessage::FP_Pack_Type ptype=FPMessage::FP_PACK_MSGPACK)
			: FPWriter(size), _quest(new FPQuest(method, oneway, ptype)){
		}

		FPQWriter(size_t size, const std::string& method, bool oneway=false, FPMessage::FP_Pack_Type ptype=FPMessage::FP_PACK_MSGPACK)
			: FPWriter(size), _quest(new FPQuest(method, oneway, ptype)){
		}

		//only support pack a map struct
		FPQWriter(const char *method, bool oneway=false, FPMessage::FP_Pack_Type ptype=FPMessage::FP_PACK_MSGPACK)
			: FPWriter(), _quest(new FPQuest(method, oneway, ptype)){
		}

		FPQWriter(const std::string& method, bool oneway=false, FPMessage::FP_Pack_Type ptype=FPMessage::FP_PACK_MSGPACK)
			: FPWriter(), _quest(new FPQuest(method, oneway, ptype)){
		}

		//only support pack raw JSON
		FPQWriter(const std::string& method, const std::string& jsonBody, bool oneway=false, FPMessage::FP_Pack_Type ptype=FPMessage::FP_PACK_MSGPACK)
			: FPWriter(jsonBody), _quest(new FPQuest(method, oneway, ptype)){
		}
		FPQWriter(const char* method, const char* jsonBody, bool oneway=false, FPMessage::FP_Pack_Type ptype=FPMessage::FP_PACK_MSGPACK)
			: FPWriter(jsonBody), _quest(new FPQuest(method, oneway, ptype)){
		}

		~FPQWriter() { }

		operator FPQuestPtr(){
			return this->take();
		}

		template<typename VALUE>
			FPQWriter& operator()(const char *key, const VALUE& v){
				this->param(key, v);
				return *this;
			}

		template<typename VALUE>
			FPQWriter& operator()(const std::string& key, const VALUE& v){
				this->param(key, v);
				return *this;
			}

		template<typename VALUE>
			FPQWriter& operator()(const VALUE& v){
				this->param(v);
				return *this;
			}

		FPQuestPtr take();
	public:
		static FPQuestPtr emptyQuest(const char *method, bool oneway=false, FPMessage::FP_Pack_Type ptype=FPMessage::FP_PACK_MSGPACK);
		static FPQuestPtr emptyQuest(const std::string& method, bool oneway=false, FPMessage::FP_Pack_Type ptype=FPMessage::FP_PACK_MSGPACK);
	private:
		FPQuestPtr _quest;
};

class FPAWriter : public FPWriter{
	public:
		static FPAnswerPtr CloneAnswer(const FPAnswerPtr answer, const FPQuestPtr quest);

	public:
		FPAWriter(size_t size, const FPQuestPtr quest)
			: FPWriter(size), _answer(new FPAnswer(quest)){
		}
		//only support pack a map
		FPAWriter(const FPQuestPtr quest)
			: FPWriter(), _answer(new FPAnswer(quest)){
		}
		//only support pack raw JSON
		FPAWriter(const char* jsonBody, const FPQuestPtr quest)
			: FPWriter(jsonBody), _answer(new FPAnswer(quest)){
		}
		FPAWriter(const std::string& jsonBody, const FPQuestPtr quest)
			: FPWriter(jsonBody), _answer(new FPAnswer(quest)){
		}

		~FPAWriter() { }

		operator FPAnswerPtr(){
			return this->take();
		}

		template<typename Value>
			FPAWriter& operator()(const char *key, const Value& v){
				this->param(key, v); 
				return *this;
			}
		template<typename Value>
			FPAWriter& operator()(const std::string& key, const Value& v){
				this->param(key, v); 
				return *this;
			}
		template<typename Value>
			FPAWriter& operator()(const Value& v){
				this->param(v); 
				return *this;
			}

		FPAnswerPtr take();
	public:
		static FPAnswerPtr errorAnswer(const FPQuestPtr quest, int code = 0, const std::string& ex = "", const std::string& raiser = "");
		static FPAnswerPtr errorAnswer(const FPQuestPtr quest, int code = 0, const char* ex = "", const char* raiser = "");
		static FPAnswerPtr emptyAnswer(const FPQuestPtr quest);
	private:
		FPAWriter(size_t size, uint16_t status, const FPQuestPtr quest)
			: FPWriter(size), _answer(new FPAnswer(status, quest)){
		}
	private:
		FPAnswerPtr _answer;
};

}

#endif
