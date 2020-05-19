CC = gcc
LOG_FLAGS = -DDEBUG -DLOG_LEVEL=LOG_DEBUG
CFLAGS = -Wall -g -Wno-unused -fPIC $(LOG_FLAGS)

all: build example

build: vmsim.o common_lin.o
	$(CC) $(CFLAGS) -shared vmsim.o common_lin.o -o libvmsim.so

vmsim.o: vmsim.c debug.h common.h

common_lin.o: common_lin.c common.h

.PHONY: clean
clean:
	-rm -f *~ *.o libvmsim.so example

example: example.c
	$(CC) $(CFLAGS) example.c -L./ -lvmsim -o example
	# $(CC) $(CFLAGS) example.c vmsim.o common_lin.o -o example

# export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:./