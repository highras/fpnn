EXES_CLIENT = TCPAsyncStressClient
EXES_CLIENT2 = UDPAsyncStressClient
EXES_TCP_VT = TCPSingleClientConcurrentTest
EXES_UDP_VT = UDPSingleClientConcurrentTest

CFLAGS +=
CXXFLAGS +=
CPPFLAGS += -g -I../../../../base -I../../../../proto -I../../../../proto/msgpack -I../../../../proto/rapidjson
LIBS += -L../../.. -L../ProtocolClient -L../../../../base -L../../../../proto  -L../../../../extends -lFpnnProtoClients

OBJS_CLIENT = TCPAsyncStressClient.o
OBJS_CLIENT2 = UDPAsyncStressClient.o

all: $(EXES_CLIENT) $(EXES_CLIENT2) $(EXES_TCP_VT) $(EXES_UDP_VT)

clean:
	$(RM) *.o $(EXES_CLIENT) $(EXES_CLIENT2) $(EXES_TCP_VT) $(EXES_UDP_VT)
	-$(RM) -rf *.dSYM

include ../../../../def.mk
