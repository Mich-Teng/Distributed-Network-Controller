.SUFFIXES: .c .o

COMPILE = $(CC) -O3 -Wall -D_FILE_OFFSET_BITS=64 -D_GNU_SOURCE -DOS_LINUX
INC_PATH = -I../common -I../server -I/usr/local/include
LIB_PATH = -L/usr/local/lib  -lpthread
TARGET_PATH = /usr/local/bin

COMMON_LIB =
SHARED_OBJS = ../common/hash.o  ../common/chain.o ../common/pthread_func.o \
              ../common/shared_func.o ../common/ini_file_reader.o \
              ../common/logger.o ../common/sockopt.o ../common/http_func.o \
              ../common/base64.o ../common/fdht_global.o

ALL_OBJS = $(SHARED_OBJS)

ALL_PRGS = fdht_compress

all: $(ALL_OBJS) $(ALL_PRGS)
.o:
	$(COMPILE) -o $@ $<  $(SHARED_OBJS) $(COMMON_LIB) $(LIB_PATH) $(INC_PATH)
.c:
	$(COMPILE) -o $@ $<  $(ALL_OBJS) $(COMMON_LIB) $(LIB_PATH) $(INC_PATH)
.c.o:
	$(COMPILE) -c -o $@ $<  $(INC_PATH)
install:
	cp -f $(ALL_PRGS) $(TARGET_PATH)
clean:
	rm -f $(ALL_OBJS) $(ALL_PRGS)
