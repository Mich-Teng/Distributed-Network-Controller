FDHT_LIBS=-L../FastDHT/client -lfastcommon -lfdhtclient
FDHT_INCLUDE_PATH=../FastDHT/common
CFLAGS=-O2 -Wall

all:  InterDomainTopo mytest 

mytest.o: mytest.c
	gcc mytest.c ${CFLAGS} -I${FDHT_INCLUDE_PATH} -c -o mytest.o

LinkEventHandler.o : LinkEventHandler.cpp
	g++ LinkEventHandler.cpp -c -o LinkEventHandler.o
LinkInfo.o: LinkInfo.cpp
	g++ LinkInfo.cpp -c -o LinkInfo.o

InterDomainTopo.o: InterDomainTopo.cpp
	g++ InterDomainTopo.cpp ${CFLAGS} -I${FDHT_INCLUDE_PATH} -c -o InterDomainTopo.o

test: InterDomainTopo.o LinkInfo.o
	g++ InterDomainTopo.o LinkInfo.o LinkEventHandler.o -L. ${FDHT_LIBS}  -o test

mytest: mytest.o
	gcc mytest.o -L. ${FDHT_LIBS} -o mytest

clean:
	rm -f *.o
	rm -f *.so
	rm -f InterDomainTopo
