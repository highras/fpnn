OBJS_C = 

OBJS_CXX = FpnnClientCenter.o FpnnTCPClient.o FpnnTCPCodec.o  FpnnUDPClient.o FpnnUDPCodec.o

# Static 
LIBFPNN_A = libFpnnProtoClients.a

# For C
LIBFPNN_S_SO = libFpnnProtoClients_s.so

# For C++
LIBFPNN_X_SO = libFpnnProtoClients_x.so

CFLAGS +=
CXXFLAGS +=
CPPFLAGS += -I../../../../base -I../../../../proto -I../../../../proto/msgpack -I../../../../proto/rapidjson

# For MacOS
UNAME := $(shell uname -s)
ifeq ($(UNAME), Darwin)
	LIBFPNN_S_SO = 
	LIBFPNN_X_SO = 
endif

all: $(LIBFPNN_A) $(LIBFPNN_S_SO) $(LIBFPNN_X_SO)

clean:
	find . -name \*.lo -o -name \*.o | xargs rm -f
	find . -name \*.la -o -name \*.a | xargs rm -f 
	find . -name \*.so | xargs rm -f
	find . -name .libs -a -type d | xargs rm -rf
	rm -rf $(LIBFPNN_X_SO) $(LIBFPNN_S_SO) $(LIBFPNN_A) 

include ../../../../def.mk
