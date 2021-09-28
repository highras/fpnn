EXES_SERVER = duplexServerTest
EXES_STRESS = asyncDuplexStressClient
EXES_SINGLE_CLIENT_CONCURRENT_TEST = singleClientConcurrentDuplexTest

CFLAGS +=
CXXFLAGS +=
CPPFLAGS += -g -I../../.. -I../../../../base -I../../../../proto -I../../../../proto/msgpack -I../../../../proto/rapidjson
LIBS += -L../../.. -L../../../../base -L../../../../proto -L../../../../extends  -lssl -lcrypto

OBJS_SERVER = duplexServerTest.o
OBJS_STRESS = asyncDuplexStressClient.o
OBJS_SINGLE_CLIENT_CONCURRENT_TEST = singleClientConcurrentDuplexTest.o

all: $(EXES_SERVER) $(EXES_STRESS) $(EXES_SINGLE_CLIENT_CONCURRENT_TEST)

clean:
	$(RM) *.o $(EXES_SERVER) $(EXES_STRESS) $(EXES_SINGLE_CLIENT_CONCURRENT_TEST)
	-$(RM) -rf *.dSYM

include ../../../../def.mk
