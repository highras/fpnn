CORE_DIR = ../../../..
FPNN_DIR = $(CORE_DIR)/..

CFLAGS +=
CXXFLAGS +=
CPPFLAGS += -std=c++11 -I$(FPNN_DIR)/base -I$(FPNN_DIR)/proto -I$(CORE_DIR) -I$(FPNN_DIR)/proto/msgpack -I$(FPNN_DIR)/proto/rapidjson
LIBS += -L$(FPNN_DIR)/extends -L$(CORE_DIR) -L$(FPNN_DIR)/proto -L$(FPNN_DIR)/base -lfpnn -lssl -lcrypto

EXES_SERVER = UDPStressController

OBJS_SERVER = StressController.o CtrlQuestProcessor.o StressController.actor.o StressRecorder.o


all: $(EXES_SERVER)

clean:
	$(RM) $(EXES_SERVER) *.o
	-$(RM) -rf *.dSYM

include $(FPNN_DIR)/def.mk