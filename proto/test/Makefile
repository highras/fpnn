EXES_TEST=example
EXES_TEST2=array_example
EXES_TEST3=custom
EXES_TEST4=bin_example
EXES_TEST5=file_example

CFLAGS +=
CXXFLAGS +=
CPPFLAGS += -I../../proto -I../../base -I../../proto/msgpack -I../../proto/rapidjson
LIBS += -L.. -L../../base -L../../core -L../../extends

OBJS_TEST = example.o 
OBJS_TEST2 = array_example.o 
OBJS_TEST3 = custom.o 
OBJS_TEST4 = bin_example.o 
OBJS_TEST5 = file_example.o 

all: $(EXES_TEST) $(EXES_TEST2) $(EXES_TEST3) $(EXES_TEST4) $(EXES_TEST5)

clean:
	$(RM) *.o $(EXES_TEST) $(EXES_TEST2) $(EXES_TEST3) $(EXES_TEST4) $(EXES_TEST5)
include ../../def.mk
