EXES_SERVER = HttpsSecondaryProxyServer

CFLAGS +=
CXXFLAGS +=
CPPFLAGS += -g -I../../../base -I../../../core -I../../../extends -I../../../proto -I../../../proto/msgpack -I../../../proto/rapidjson
LIBS += -L../../../base -L../../../proto -L../../../core -L../../../extends

OBJS_SERVER = ProxyQuestProcessor.o HttpsSecondaryProxyServer.o

all: $(EXES_SERVER)

clean:
	$(RM) *.o $(EXES_SERVER)
	-$(RM) -rf *.dSYM


include ../../../def.mk