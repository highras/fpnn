EXES_CLIENT = tcpfss
EXES_CLIENT2 = udpfss

CFLAGS +=
CXXFLAGS +=
CPPFLAGS += -g -I../../../../base -I../../../../proto -I../../../../proto/msgpack -I../../../../proto/rapidjson
LIBS += -L../../.. -L../ProtocolClient -L../../../../base -L../../../../proto  -L../../../../extends -lFpnnProtoClients

OBJS_CLIENT = tcpfss.o linenoise.o
OBJS_CLIENT2 = udpfss.o linenoise.o

all: $(EXES_CLIENT2) $(EXES_CLIENT)

clean:
	$(RM) *.o $(EXES_CLIENT2) $(EXES_CLIENT)
	-$(RM) -rf *.dSYM

include ../../../../def.mk
