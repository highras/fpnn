#ifndef FPNN_Decoder_H
#define FPNN_Decoder_H

#include <string.h>
#include <string>
#include <algorithm>
#include <exception>
#include "ChainBuffer.h"
#include "FPMessage.h"
#include "HttpParser.h"

namespace fpnn
{
	class Decoder
	{
	public:
		static FPQuestPtr decodeQuest(const char* buf, int len)
		{
			size_t total_len = sizeof(FPMessage::Header) + FPMessage::BodyLen(buf);

			if(len != (int)total_len) 
				throw FPNN_ERROR_CODE_FMT(FpnnCoreError, FPNN_EC_CORE_INVALID_PACKAGE, "Expect Len:%d, readed Len:%d", total_len, len);

			FPQuestPtr quest = NULL;
			try{
				quest.reset(new FPQuest(buf,total_len));
			}
			catch(const FpnnError& ex){
				LOG_ERROR("Can not create Quest from Raw:(%d) %s", ex.code(), ex.what());
			}
			catch(...){
				LOG_ERROR("Unknow Error, Can not create Quest from Raw Data.");
			}
			return quest;
		}

		static FPQuestPtr decodeQuest(ChainBuffer* cb)
		{
			/**
				IChainBuffer->header(int require_length) maybe return NULL.
				But TWO things ensure in this case, the function alway returns available pointer, NEVER returns NULL.
				1. RecvBuffer chunk size is larger than FPMessage::HeaderLength.
					(Set By TCPServerConnection in IOWorker.h or TCPClientConnection in ClientIOWorker.h.)
				2. By framework logic, all caller will call this function after received at least FPMessage::HeaderLength bytes data.
			*/
			const char* header = (const char *)cb->header(FPMessage::_HeaderLength);
			size_t total_len = sizeof(FPMessage::Header) + FPMessage::BodyLen(header);

			if(cb->length() != (int)total_len) 
				throw FPNN_ERROR_CODE_FMT(FpnnCoreError, FPNN_EC_CORE_INVALID_PACKAGE, "Expect Len:%d, readed Len:%d", total_len, cb->length());

			char* buf = (char*)malloc(total_len);
			cb->writeTo(buf, total_len, 0);


			FPQuestPtr quest = NULL;
			try{
				quest.reset(new FPQuest(buf,total_len));
			}
			catch(const FpnnError& ex){
				LOG_ERROR("Can not create Quest from Raw:(%d) %s", ex.code(), ex.what());
			}
			catch(...){
				LOG_ERROR("Unknow Error, Can not create Quest from Raw Data.");
			}
			free(buf);
			return quest;
		}

		static FPQuestPtr decodeHttpQuest(ChainBuffer* cb, HttpParser& httpParser)
		{
			if(httpParser._post){
				httpParser.parseBody(cb);
			}
			else{
				auto it = httpParser._httpInfos.find("u_fpnn");
				if(it == httpParser._httpInfos.end())	
					httpParser._httpInfos["u_fpnn"] = "{}";
				httpParser._content = httpParser._httpInfos["u_fpnn"];
			}
			return std::make_shared<FPQuest>(httpParser._method, httpParser._content, httpParser._httpInfos, httpParser._post);
		}

		static FPAnswerPtr decodeAnswer(const char* buf, int len)
		{
			size_t total_len = sizeof(FPMessage::Header) + FPMessage::BodyLen(buf);

			if(len != (int)total_len)
				throw FPNN_ERROR_CODE_FMT(FpnnCoreError, FPNN_EC_CORE_INVALID_PACKAGE, "Expect Len:%d, readed Len:%d", total_len, len);

			FPAnswerPtr answer = NULL;
			try{
				answer.reset(new FPAnswer(buf,total_len));
			}
			catch(const FpnnError& ex){
				LOG_ERROR("Can not create Answer from Raw:(%d) %s", ex.code(), ex.what());
			}
			catch(...){
				LOG_ERROR("Unknown Error, Can not create Answer from Raw Data");
			}
			return answer;
		}

		static FPAnswerPtr decodeAnswer(ChainBuffer* cb)
		{
			/**
				IChainBuffer->header(int require_length) maybe return NULL.
				But TWO things ensure in this case, the function alway returns available pointer, NEVER returns NULL.
				1. RecvBuffer chunk size is larger than FPMessage::HeaderLength.
					(Set By TCPServerConnection in IOWorker.h or TCPClientConnection in ClientIOWorker.h.)
				2. By framework logic, all caller will call this function after received at least FPMessage::HeaderLength bytes data.
			*/
			const char* header = (const char *)cb->header(FPMessage::_HeaderLength);
			size_t total_len = sizeof(FPMessage::Header) + FPMessage::BodyLen(header);

			if(cb->length() != (int)total_len)
				throw FPNN_ERROR_CODE_FMT(FpnnCoreError, FPNN_EC_CORE_INVALID_PACKAGE, "Expect Len:%d, readed Len:%d", total_len, cb->length());

			char* buf = (char*)malloc(total_len);
			cb->writeTo(buf, total_len, 0);


			FPAnswerPtr answer = NULL;
			try{
				answer.reset(new FPAnswer(buf,total_len));
			}
			catch(const FpnnError& ex){
				LOG_ERROR("Can not create Answer from Raw:(%d) %s", ex.code(), ex.what());
			}
			catch(...){
				LOG_ERROR("Unknown Error, Can not create Answer from Raw Data");
			}
			free(buf);
			return answer;
		}

		static bool isQuest(ChainBuffer* cb)
		{
			/**
				IChainBuffer->header(int require_length) maybe return NULL.
				But TWO things ensure in this case, the function alway returns available pointer, NEVER returns NULL.
				1. RecvBuffer chunk size is larger than FPMessage::HeaderLength.
					(Set By TCPServerConnection in IOWorker.h or TCPClientConnection in ClientIOWorker.h.)
				2. By framework logic, all caller will call this function after received at least FPMessage::HeaderLength bytes data.
			*/
			return FPMessage::isQuest((const char *)cb->header(FPMessage::_HeaderLength));
		}
	};
}

#endif
