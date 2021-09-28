EXES_CLIENT = clientTest
EXES_TEST = clientAsyncTest
EXES_TEST3 = netAsyncTest
EXES_TEST4 = clientNetTest 
EXES_SHORT_CONCURRENT = shortConnectionTesting
EXES_STRESS = asyncStressClient
EXES_MASSIVE_TEST = massiveClientTest
EXES_MASSIVE_ASYNC_TEST = massiveAsyncClientTest
EXES_TEST_CLONE_QUEST = testCloneQuest
EXES_TEST_CLONE_ANSWER = testCloneAnswer
EXES_ACCASVT = asyncCloseConnectionAfterSentViolentTest

CFLAGS +=
CXXFLAGS +=
CPPFLAGS += -g -I../.. -I../../../base -I../../../proto -I../../../proto/msgpack -I../../../proto/rapidjson
LIBS += -L../.. -L../../../base -L../../../proto -L../../../extends  -lssl -lcrypto

OBJS_CLIENT = clientTest.o
OBJS_TEST = clientAsyncTest.o
OBJS_TEST3 = netAsyncTest.o
OBJS_TEST4 = clientNetTest.o

all: $(EXES_CLIENT) $(EXES_TEST) $(EXES_TEST3) $(EXES_TEST4) $(EXES_SHORT_CONCURRENT) $(EXES_STRESS) $(EXES_MASSIVE_TEST) $(EXES_MASSIVE_ASYNC_TEST) $(EXES_TEST_CLONE_ANSWER) $(EXES_TEST_CLONE_QUEST) $(EXES_ACCASVT)

clean:
	$(RM) *.o $(EXES_CLIENT) $(EXES_TEST) $(EXES_TEST3) $(EXES_TEST4) $(EXES_SHORT_CONCURRENT) $(EXES_STRESS) $(EXES_MASSIVE_TEST) $(EXES_MASSIVE_ASYNC_TEST) $(EXES_TEST_CLONE_ANSWER) $(EXES_TEST_CLONE_QUEST) $(EXES_ACCASVT)
	-$(RM) -rf *.dSYM
	make clean -C duplexTest

include ../../../def.mk
