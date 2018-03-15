EXES_CMD = cmd
EXES_CLIENT = fss
EXES_CLIENT2 = eccKeyMaker

CFLAGS +=
CXXFLAGS +=
CPPFLAGS += -g -I.. -I../../base -I../../proto -I../../proto/msgpack -I../../proto/rapidjson
LIBS += -L.. -L../../base -L../../proto

OBJS_CMD = cmd.o
OBJS_CLIENT = fss.o linenoise.o
OBJS_CLIENT2 = eccKeyMaker.o PemGenerator.o

all: $(EXES_CLIENT2) $(EXES_CMD) $(EXES_CLIENT)

clean:
	$(RM) *.o $(EXES_CLIENT2) $(EXES_CMD) $(EXES_CLIENT)

cleankey:
	$(RM) *.key *.pem *.der

cleanall: clean cleankey

include ../../def.mk
