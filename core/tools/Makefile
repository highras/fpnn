EXES_CMD = cmd
EXES_CMD_INFOS = cmdinfos
EXES_CMD_STATUS = cmdstatus
EXES_CMD_TUNE = cmdtune
EXES_CLIENT = fss
EXES_CLIENT2 = eccKeyMaker

CFLAGS +=
CXXFLAGS +=
CPPFLAGS += -g -I.. -I../../base -I../../proto -I../../proto/msgpack -I../../proto/rapidjson
LIBS += -L.. -L../../base -L../../proto  -L../../extends -lssl -lcrypto

OBJS_CMD = cmd.o
OBJS_CMD_INFOS = cmdinfos.o
OBJS_CMD_STATUS = cmdstatus.o
OBJS_CMD_TUNE = cmdtune.o
OBJS_CLIENT = fss.o linenoise.o
OBJS_CLIENT2 = eccKeyMaker.o PemGenerator.o

all: $(EXES_CLIENT2) $(EXES_CMD) $(EXES_CLIENT) $(EXES_CMD_INFOS) $(EXES_CMD_STATUS) $(EXES_CMD_TUNE)

clean:
	$(RM) *.o $(EXES_CLIENT2) $(EXES_CMD) $(EXES_CLIENT) $(EXES_CMD_INFOS) $(EXES_CMD_STATUS) $(EXES_CMD_TUNE)
	-$(RM) -rf *.dSYM

cleankey:
	$(RM) *.key *.pem *.der

cleanall: clean cleankey

include ../../def.mk
