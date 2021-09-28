CORE_DIR = ../../../..
FPNN_DIR = $(CORE_DIR)/..

CFLAGS +=
CXXFLAGS +=
CPPFLAGS += -std=c++11 -I$(FPNN_DIR)/base -I$(FPNN_DIR)/proto -I$(CORE_DIR) -I$(FPNN_DIR)/proto/msgpack -I$(FPNN_DIR)/proto/rapidjson
LIBS += -L$(FPNN_DIR)/extends -L$(CORE_DIR) -L$(FPNN_DIR)/proto -L$(FPNN_DIR)/base -lfpnn -lssl -lcrypto

EXES_SERVER = UDPMassConnController
EXES_CLIENT = UDPMassConnLoadWaiter

OBJS_SERVER = MassConnController.o CtrlQuestProcessor.o MassConnController.actor.o StressRecorder.o
OBJS_CLIENT = MassConnLoadWaiter.o


all: $(EXES_SERVER) $(EXES_CLIENT)

clean:
	$(RM) $(EXES_SERVER) $(EXES_CLIENT) *.o
	-$(RM) -rf *.dSYM

include $(FPNN_DIR)/def.mk