OBJS_C = binary_abbr.o md5.o bit.o hashint.o jenkins.o obpool.o cpu.o \
		 hex.o obstack.o rdtsc.o rijndael.o sha1.o sha256.o uuid.o \
		 crc.o crc64.o carp.o unix_user.o unixfs.o msec.o httpcode.o \
		 strhash.o hseq.o net.o base64.o

OBJS_CXX = FpnnError.o FunCarpSequence.o ParamThreadPool.o TaskThreadPool.o \
		   ChainBuffer.o CachedChainBuffer.o StringUtil.o TimeUtil.o  \
		   HSequence.o Setting.o ServerInfo.o FPLog.o \
		   TimeAnalyst.o HostLookup.o Endian.o escapeString.o \
		   PrintMemory.o NetworkUtility.o MemoryPool.o HttpClient.o UnlockedMemoryPool.o \
		   TableRow.o MidGenerator.o FPJson.o FPJsonParser.o FileSystemUtil.o gzpipe.o \
		   FormattedPrint.o ResourceCenter.o FPTimer.o CommandLineUtil.o MachineStatus.o

# Static 
LIBFPNN_A = libfpbase.a

# For C
LIBFPNN_S_SO = libfpbase_s.so

# For C++
LIBFPNN_X_SO = libfpbase_x.so

CFLAGS +=
CXXFLAGS += 
CPPFLAGS +=

all: $(LIBFPNN_A) $(LIBFPNN_S_SO) $(LIBFPNN_X_SO)

clean:
	find . -name \*.lo -o -name \*.o | xargs rm -f
	find . -name \*.la -o -name \*.a | xargs rm -f 
	find . -name \*.so | xargs rm -f
	find . -name .libs -a -type d | xargs rm -rf
	rm -f $(LIBFPNN_X_SO) $(LIBFPNN_S_SO) $(LIBFPNN_A)

	make clean -C test
include ../def.mk
