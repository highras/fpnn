OBJS_C = 

OBJS_CXX = FPMessage.o FPReader.o FPWriter.o JSONConvert.o

# Static 
LIBFPNN_A = libfpproto.a

# For C
LIBFPNN_S_SO = libfpproto_s.so

# For C++
LIBFPNN_X_SO = libfpproto_x.so

CFLAGS +=
CXXFLAGS +=
CPPFLAGS += -I../base -I./msgpack -I./rapidjson

# For MacOS
UNAME := $(shell uname -s)
ifeq ($(UNAME), Darwin)
	LIBFPNN_S_SO = 
	LIBFPNN_X_SO = 
endif

all: $(LIBFPNN_A) $(LIBFPNN_S_SO) $(LIBFPNN_X_SO)

test:
	make -C test

clean:
	find . -name \*.lo -o -name \*.o | xargs rm -f
	find . -name \*.la -o -name \*.a | xargs rm -f 
	find . -name \*.so | xargs rm -f
	find . -name .libs -a -type d | xargs rm -rf
	rm -rf $(LIBFPNN_X_SO) $(LIBFPNN_S_SO) $(LIBFPNN_A) 
	make clean -C test
include ../def.mk
