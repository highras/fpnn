OBJS_C = 

OBJS_CXX = ClientEngine.o ConnectionReclaimer.o ClientIOWorker.o GlobalIOPool.o IOBuffer.o PartitionedConnectionMap.o \
			TCPServerMasterProcessor.o ServerIOWorker.o TCPClient.o TCPEpollServer.o HttpParser.o Config.o Statistics.o \
			Encryptor.o StandardReceiver.o EncryptedStreamReceiver.o EncryptedPackageReceiver.o WebSocketReceiver.o \
			micro-ecc/uECC.o KeyExchange.o PEM_DER_SAX.o IQuestProcessor.o UDPEpollServer.o UDPServerMasterProcessor.o \
			ServerController.o UDPClient.o UDPIOBuffer.o UDPClientIOBuffer.o UDPSessions.o UDPClientIOWorker.o ClientInterface.o \
			OpenSSLModule.o

# Static 
LIBFPNN_A = libfpnn.a

# For C
LIBFPNN_S_SO = libfpnn_s.so

# For C++
LIBFPNN_X_SO = libfpnn_x.so

CFLAGS +=
CXXFLAGS +=
CPPFLAGS += -I../base -I../proto -I../proto/msgpack -I../proto/rapidjson

all: $(LIBFPNN_A) $(LIBFPNN_S_SO) $(LIBFPNN_X_SO)

test:
	make -C test
	make -C example 

clean:
	find . -name \*.lo -o -name \*.o | xargs rm -f
	find . -name \*.la -o -name \*.a | xargs rm -f 
	find . -name \*.so | xargs rm -f
	find . -name .libs -a -type d | xargs rm -rf
	rm -rf $(LIBFPNN_X_SO) $(LIBFPNN_S_SO) $(LIBFPNN_A) 
	make clean -C test
	make clean -C example
	make clean -C tools
include ../def.mk
