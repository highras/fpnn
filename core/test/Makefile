EXES_CLIENT = clientTest
EXES_TEST = clientAsyncTest
EXES_TEST2 = cmd 
EXES_TEST3 = netAsyncTest
EXES_SERVER = serverTest
EXES_CLIENT2 = clientAsyncOnewayTest
EXES_DUPLEX_CLIENT = duplexClientTest
EXES_SHORT_CONCURRENT = shortConnectionTesting
EXES_STRESS = asyncStressClient
EXES_PERIOD_TEST = periodClientTest
EXES_TIMEOUT_TEST = timeoutTest
EXES_MASSIVE_TEST = massiveClientTest
EXES_MASSIVE_ASYNC_TEST = massiveAsyncClientTest
EXES_TEST_CLONE_QUEST = testCloneQuest
EXES_TEST_CLONE_ANSWER = testCloneAnswer
EXES_SINGLE_CLIENT_CONCURRENT_TEST = singleClientConcurrentTest
EXES_ACCASVT = asyncCloseConnectionAfterSentViolentTest

CFLAGS +=
CXXFLAGS +=
CPPFLAGS += -g -I.. -I../../base -I../../proto -I../../proto/msgpack -I../../proto/rapidjson
LIBS += -L.. -L../../base -L../../proto -L../../extends  -lssl -lcrypto

OBJS_CLIENT = clientTest.o
OBJS_TEST = clientAsyncTest.o
OBJS_TEST2 = cmd.o
OBJS_TEST3 = netAsyncTest.o
OBJS_SERVER = serverTest.o
OBJS_CLIENT2 = clientAsyncOnewayTest.o
OBJS_DUPLEX_CLIENT = duplexClientTest.o
OBJS_SHORT_CONCURRENT = shortConnectionTesting.o
OBJS_STRESS = asyncStressClient.o
OBJS_PERIOD_TEST = periodClientTest.o
OBJS_TIMEOUT_TEST = timeoutTest.o
OBJS_MASSIVE_TEST = massiveClientTest.o
OBJS_MASSIVE_ASYNC_TEST = massiveAsyncClientTest.o
OBJS_TEST_CLONE_QUEST = testCloneQuest.o
OBJS_TEST_CLONE_ANSWER = testCloneAnswer.o
OBJS_SINGLE_CLIENT_CONCURRENT_TEST = singleClientConcurrentTest.o
OBJS_ACCASVT = asyncCloseConnectionAfterSentViolentTest.o

all: $(EXES_CLIENT) $(EXES_TEST) $(EXES_SERVER) $(EXES_TEST2) $(EXES_TEST3) $(EXES_CLIENT2) $(EXES_DUPLEX_CLIENT) $(EXES_SHORT_CONCURRENT) $(EXES_STRESS) $(EXES_PERIOD_TEST) $(EXES_TIMEOUT_TEST) $(EXES_MASSIVE_TEST) $(EXES_MASSIVE_ASYNC_TEST) $(EXES_TEST_CLONE_ANSWER) $(EXES_TEST_CLONE_QUEST) $(EXES_SINGLE_CLIENT_CONCURRENT_TEST) $(EXES_ACCASVT)

clean:
	$(RM) *.o $(EXES_CLIENT) $(EXES_TEST) $(EXES_SERVER) $(EXES_TEST2) $(EXES_TEST3) $(EXES_CLIENT2) $(EXES_DUPLEX_CLIENT) $(EXES_SHORT_CONCURRENT) $(EXES_STRESS) $(EXES_PERIOD_TEST) $(EXES_TIMEOUT_TEST) $(EXES_MASSIVE_TEST) $(EXES_MASSIVE_ASYNC_TEST) $(EXES_TEST_CLONE_ANSWER) $(EXES_TEST_CLONE_QUEST) $(EXES_SINGLE_CLIENT_CONCURRENT_TEST) $(EXES_ACCASVT)
	make clean -C AutoDistributedTest
	make clean -C udp-test

include ../../def.mk
