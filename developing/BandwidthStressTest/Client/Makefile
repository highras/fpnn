EXES_UP_TEST = upTest
EXES_DL_TEST = dlTest
EXES_LS_TEST = lsTest
EXES_STREAM_TEST = streamTest

CFLAGS +=
CXXFLAGS +=
CPPFLAGS += -g -I../../../base -I../../../core -I../../../proto -I../../../proto/msgpack -I../../../proto/rapidjson
LIBS += -L../../../base -L../../../proto -L../../../core -L../../../extends


all: $(EXES_UP_TEST) $(EXES_DL_TEST) $(EXES_LS_TEST) $(EXES_STREAM_TEST)

clean:
	$(RM) *.o $(EXES_UP_TEST) $(EXES_DL_TEST) $(EXES_LS_TEST) $(EXES_STREAM_TEST)
	-$(RM) -rf *.dSYM


include ../../../def.mk