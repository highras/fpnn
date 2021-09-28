#EXES_CLIENT = clientTest
#EXES_TEST = clientAsyncTest
#EXES_TEST3 = netAsyncTest
#EXES_TEST4 = clientNetTest 
EXES_SERVER = serverTest
EXES_ASYNC_ONEWAY_TEST = clientAsyncOnewayTest
EXES_DUPLEX_CLIENT = duplexClientTest
#EXES_SHORT_CONCURRENT = shortConnectionTesting
#EXES_STRESS = asyncStressClient
EXES_PERIOD_TEST = periodClientTest
EXES_TIMEOUT_TEST = timeoutTest
#EXES_MASSIVE_TEST = massiveClientTest
#EXES_MASSIVE_ASYNC_TEST = massiveAsyncClientTest
#EXES_TEST_CLONE_QUEST = testCloneQuest
#EXES_TEST_CLONE_ANSWER = testCloneAnswer
EXES_SINGLE_CLIENT_CONCURRENT_TEST = singleClientConcurrentTest
#EXES_ACCASVT = asyncCloseConnectionAfterSentViolentTest

OBJS_SERVER = serverTest.o

CFLAGS +=
CXXFLAGS +=
CPPFLAGS += -g -I../.. -I../../../base -I../../../proto -I../../../proto/msgpack -I../../../proto/rapidjson
LIBS += -L../.. -L../../../base -L../../../proto -L../../../extends  -lssl -lcrypto

all: $(EXES_SERVER) $(EXES_ASYNC_ONEWAY_TEST) $(EXES_DUPLEX_CLIENT) $(EXES_PERIOD_TEST) $(EXES_TIMEOUT_TEST) $(EXES_SINGLE_CLIENT_CONCURRENT_TEST)

clean:
	$(RM) *.o $(EXES_SERVER) $(EXES_ASYNC_ONEWAY_TEST) $(EXES_DUPLEX_CLIENT) $(EXES_PERIOD_TEST) $(EXES_TIMEOUT_TEST) $(EXES_SINGLE_CLIENT_CONCURRENT_TEST)
	-$(RM) -rf *.dSYM
	
include ../../../def.mk
