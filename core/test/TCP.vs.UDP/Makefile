EXES_SERVER = UDPBaseServer

CORE_DIR = ../..
FPNN_DIR = $(CORE_DIR)/..
CFLAGS +=
CXXFLAGS +=
CPPFLAGS += -I$(CORE_DIR) -I$(FPNN_DIR)/proto -I$(FPNN_DIR)/base -I$(FPNN_DIR)/proto/msgpack -I$(FPNN_DIR)/proto/rapidjson -I$(FPNN_DIR)/extends
LIBS += -L$(CORE_DIR) -L$(FPNN_DIR)/proto -L$(FPNN_DIR)/base -lfpnn  -L$(FPNN_DIR)/extends -lextends

OBJS_SERVER = UDPBaseServer.o UDPTestProcessor.o

all: $(EXES_SERVER)

clean:
	$(RM) *.o $(EXES_SERVER)
	-$(RM) -rf *.dSYM
include $(FPNN_DIR)/def.mk