# OBJS_CXX = FPJson.o FPJsonParser.o
OBJS_CXX = FPZKClient.o

# Static 
LIBFPNN_A = libdeveloping.a

CFLAGS +=
CXXFLAGS += -I../base -I../core -I../proto -I../proto/msgpack -I../proto/rapidjson
CPPFLAGS +=

all: $(LIBFPNN_A)

clean:
	find . -name \*.lo -o -name \*.o | xargs rm -f
	find . -name \*.la -o -name \*.a | xargs rm -f 
	find . -name \*.so | xargs rm -f
	find . -name .libs -a -type d | xargs rm -rf
	rm -f $(LIBFPNN_A)
	#make clean -C test
include ../def.mk
