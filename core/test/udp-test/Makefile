
EXES_STRESS = asyncStressClient

CFLAGS +=
CXXFLAGS +=
CPPFLAGS += -g -I../.. -I../../../base -I../../../proto -I../../../proto/msgpack -I../../../proto/rapidjson
LIBS += -L../.. -L../../../base -L../../../proto -L../../../extends -lssl -lcrypto


all: $(EXES_STRESS)

clean:
	$(RM) *.o $(EXES_STRESS)

include ../../../def.mk
