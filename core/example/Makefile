EXES_TEST = complexServer
EXES_TEST2 = simpleServer
EXES_TEST3 = memcacheServer
EXES_CLIENT3 = memcacheClient

CFLAGS +=
CXXFLAGS +=
CPPFLAGS += -g -I.. -I../../base -I../../proto -I../../proto/msgpack -I../../proto/rapidjson
LIBS += -L.. -L../../base -L../../proto  -L../../extends

OBJS_TEST = complexServer.o
OBJS_TEST2 = simpleServer.o
OBJS_TEST3 = memcacheServer.o
OBJS_CLIENT3 = memcacheClient.o

all: $(EXES_TEST) $(EXES_TEST2) $(EXES_TEST3) $(EXES_CLIENT3)

clean:
	$(RM) *.o $(EXES_TEST) $(EXES_TEST2) $(EXES_TEST3) $(EXES_CLIENT3)
	-$(RM) -rf *.dSYM
include ../../def.mk
