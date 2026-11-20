CC=gcc
CFLAGS=-I.
POSIX=-pthread
LIB=-L. -Wl,-rpath,'$$ORIGIN'

all: client server libutils.so

client : client.c libutils.so utils.h utils.c
	$(CC) -o client client.c $(LIB) -lutils $(POSIX) $(CFLAGS)

server : server.c libutils.so utils.h utils.c
	$(CC) -o server server.c $(LIB) -lutils $(POSIX) $(CFLAGS)

libutils.so: utils.h utils.c
	$(CC) $(CFLAGS) -fPIC -shared -o libutils.so utils.c