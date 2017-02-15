CC = gcc
LOG_FLAGS = -DDEBUG -DLOG_LEVEL=LOG_DEBUG
CFLAGS = -Wall -g -Wno-unused -fPIC $(LOG_FLAGS)

all: build

build: vmsim.o common_lin.o
	$(CC) $(CFLAGS) -shared vmsim.o common_lin.o -o libvmsim.so

vmsim.o: vmsim.c debug.h common.h

common_lin.o: common_lin.c common.h

.PHONY: clean
clean:
	-rm -f *~ *.o libvmsim.so
