EXE_MemoryPoolTest = MemoryPoolTest
EXE_ObjectPoolTest = ObjectPoolTest
EXE_ParamThreadPoolTest = ParamThreadPoolTest
EXE_ParamTemplateThreadPoolTest = ParamTemplateThreadPoolTest
EXE_TaskThreadPoolTest = TaskThreadPoolTest
EXE_LambdaTaskThreadPoolTest = LambdaTaskThreadPoolTest
EXE_HashSpeedTest = hashSpeedTest
EXE_TwoTimeHashTest = twoTimeHashTest 
EXE_RijndaelDemo = rijndaelDemo

EXES_JSONTEST = FPJsonParseTest
EXES_JSONFUNCTIONALTEST = FPJsonFunctionalTest
EXES_JSONADVANCEDTEST_Modify = FPJsonAdvancedTest_modify
EXES_JSONADVANCEDTEST_Fetch_WantMethods = FPJsonAdvancedTest_fetch_wantMethods
EXES_JSONADVANCEDTEST_Fetch_WantWithPathMethods = FPJsonAdvancedTest_fetch_wantWithPathMethods
EXES_JSONADVANCEDTEST_Fetch_GetWithPathMethods = FPJsonAdvancedTest_fetch_getWithPathMethods

EXES_ResourceCenterTest = ResourceCenterTest
EXES_FPTimerTest = FPTimerTest

EXE_LIBS = -L.. -lfpbase -lcurl -lpthread

JSON_Module = $(EXES_JSONTEST) $(EXES_JSONFUNCTIONALTEST) $(EXES_JSONADVANCEDTEST_Modify) \
	$(EXES_JSONADVANCEDTEST_Fetch_WantMethods) $(EXES_JSONADVANCEDTEST_Fetch_WantWithPathMethods) $(EXES_JSONADVANCEDTEST_Fetch_GetWithPathMethods)
EXES = $(EXE_MemoryPoolTest) $(EXE_ObjectPoolTest) $(EXE_ParamThreadPoolPoolTest) $(EXE_ParamTemplateThreadPoolTest) \
	$(EXE_TaskThreadPoolTest) $(EXE_LambdaTaskThreadPoolTest) $(EXE_ParamThreadPoolTest) $(EXE_HashSpeedTest) $(EXE_TwoTimeHashTest) $(EXE_RijndaelDemo) \
	$(EXES_ResourceCenterTest) $(EXES_FPTimerTest)

all:
	g++ --std=c++11 -I.. -o $(EXE_MemoryPoolTest) MemoryPoolTest.cpp $(EXE_LIBS)
	g++ --std=c++11 -I.. -o $(EXE_ObjectPoolTest) ObjectPoolTest.cpp $(EXE_LIBS)
	g++ --std=c++11 -I.. -o $(EXE_ParamThreadPoolTest) ParamThreadPoolTest.cpp $(EXE_LIBS)
	g++ --std=c++11 -I.. -o $(EXE_ParamTemplateThreadPoolTest) ParamTemplateThreadPoolTest.cpp $(EXE_LIBS)
	g++ --std=c++11 -I.. -o $(EXE_TaskThreadPoolTest) TaskThreadPoolTest.cpp $(EXE_LIBS)
	g++ --std=c++11 -I.. -o $(EXE_LambdaTaskThreadPoolTest) LambdaTaskThreadPoolTest.cpp $(EXE_LIBS)
	g++ --std=c++11 -I.. -o $(EXE_HashSpeedTest) hashSpeedTest.cpp $(EXE_LIBS)
	g++ --std=c++11 -I.. -o $(EXE_TwoTimeHashTest) twoTimeHashTest.cpp $(EXE_LIBS)
	g++ --std=c++11 -I.. -o $(EXE_RijndaelDemo) rijndaelDemo.cpp $(EXE_LIBS)
	
	g++ -O2 -o queueListPerformanceTest_O2 queueListPerformanceTest.cpp $(EXE_LIBS)
ifdef $(LINKARGS)
	g++ -O2 -ltcmalloc -o queueListPerformanceTest_O2_tcmalloc queueListPerformanceTest.cpp $(EXE_LIBS)
endif

	g++ --std=c++11 -O2 -I.. -o ThreadPoolPerformanceTest_O2 ThreadPoolPerformanceTest.cpp $(EXE_LIBS)
ifdef $(LINKARGS)
	g++ --std=c++11 -O2 -ltcmalloc -I.. -o ThreadPoolPerformanceTest_O2_tcmalloc ThreadPoolPerformanceTest.cpp $(EXE_LIBS)
endif

	g++ --std=c++11 -O2 -o heapPerformanceTest_O2 heapPerformanceTest.cpp
ifdef $(LINKARGS)
	g++ --std=c++11 -O2 -ltcmalloc -o heapPerformanceTest_O2_tcmalloc heapPerformanceTest.cpp
endif

	g++ --std=c++11 -I.. -o $(EXES_JSONTEST) FPJsonParseTest.cpp -L.. -lfpbase
	g++ --std=c++11 -I.. -o $(EXES_JSONFUNCTIONALTEST) FPJsonFunctionalTest.cpp -L.. -lfpbase
	g++ --std=c++11 -I.. -o $(EXES_JSONADVANCEDTEST_Modify) FPJsonAdvancedTest_modify.cpp -L.. -lfpbase
	g++ --std=c++11 -I.. -o $(EXES_JSONADVANCEDTEST_Fetch_WantMethods) FPJsonAdvancedTest_fetch_wantMethods.cpp -L.. -lfpbase
	g++ --std=c++11 -I.. -o $(EXES_JSONADVANCEDTEST_Fetch_WantWithPathMethods) FPJsonAdvancedTest_fetch_wantWithPathMethods.cpp -L.. -lfpbase
	g++ --std=c++11 -I.. -o $(EXES_JSONADVANCEDTEST_Fetch_GetWithPathMethods) FPJsonAdvancedTest_fetch_getWithPathMethods.cpp -L.. -lfpbase

	g++ --std=c++11 -I.. -o $(EXES_ResourceCenterTest) ResourceCenterTest.cpp $(EXE_LIBS)
	g++ --std=c++11 -I.. -o $(EXES_FPTimerTest) FPTimerTest.cpp $(EXE_LIBS)

	g++ --std=c++11 -O2 -o rijndaelPerformance_O2 rijndaelPerformance.cpp ../rijndael.c -I..

clean:
	$(RM) -f *.o $(EXES) $(JSON_Module) queueListPerformanceTest_O2 queueListPerformanceTest_O2_tcmalloc \
		ThreadPoolPerformanceTest_O2 ThreadPoolPerformanceTest_O2_tcmalloc heapPerformanceTest_O2 heapPerformanceTest_O2_tcmalloc rijndaelPerformance_O2
