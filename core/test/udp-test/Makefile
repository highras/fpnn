EXES_TEST2 = cmd
EXES_SERVER = serverTest
EXES_CLIENT2 = clientAsyncOnewayTest
EXES_DUPLEX_CLIENT = duplexClientTest
EXES_STRESS = asyncStressClient
EXES_PERIOD_TEST = periodClientTest
EXES_TIMEOUT_TEST = timeoutTest
EXES_SINGLE_CLIENT_CONCURRENT_TEST = singleClientConcurrentTest

CFLAGS +=
CXXFLAGS +=
CPPFLAGS += -g -I../.. -I../../../base -I../../../proto -I../../../proto/msgpack -I../../../proto/rapidjson
LIBS += -L../.. -L../../../base -L../../../proto -L../../../extends

OBJS_TEST2 = cmd.o
OBJS_SERVER = serverTest.o
OBJS_CLIENT2 = clientAsyncOnewayTest.o
OBJS_DUPLEX_CLIENT = duplexClientTest.o
OBJS_STRESS = asyncStressClient.o
OBJS_PERIOD_TEST = periodClientTest.o
OBJS_TIMEOUT_TEST = timeoutTest.o
OBJS_SINGLE_CLIENT_CONCURRENT_TEST = singleClientConcurrentTest.o

all: $(EXES_SERVER) $(EXES_CLIENT2) $(EXES_TEST2) $(EXES_DUPLEX_CLIENT) $(EXES_STRESS) $(EXES_PERIOD_TEST) $(EXES_TIMEOUT_TEST)  $(EXES_SINGLE_CLIENT_CONCURRENT_TEST)

clean:
	$(RM) *.o $(EXES_SERVER) $(EXES_CLIENT2) $(EXES_TEST2) $(EXES_DUPLEX_CLIENT) $(EXES_STRESS) $(EXES_PERIOD_TEST) $(EXES_TIMEOUT_TEST)  $(EXES_SINGLE_CLIENT_CONCURRENT_TEST)

include ../../../def.mk
