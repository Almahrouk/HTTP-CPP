CC=clang
CFLAGS=-g
COMMON_OBJS = common.o

all: $(COMMON_OBJS) tcpc tcps

common.o: common.h common.c
	$(CC) $(CFLAGS) -c common.c

tcpc: tcpclient.c $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o tcpc tcpclient.c $(COMMON_OBJS)

tcps: tcpserver.c $(COMMON_OBJS)
	$(CC) $(CFLAGS) -o tcps tcpserver.c $(COMMON_OBJS)

clean:
	rm -rf *.o *.dSYM tcpc tcps