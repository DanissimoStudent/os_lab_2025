CC=gcc
CFLAGS=-I.

all: parallel_min_max zombie_example process_memory

parallel_min_max : utils.o find_min_max.o
	$(CC) -o parallel_min_max utils.o find_min_max.o parallel_min_max.c $(CFLAGS)

zombie_example : 
	$(CC) -o zombie_example zombie_example.c $(CFLAGS)

process_memory : 
	$(CC) -o process_memory process_memory.c $(CFLAGS)

utils.o : utils.h
	$(CC) -o utils.o -c utils.c $(CFLAGS)

find_min_max.o : utils.h find_min_max.h
	$(CC) -o find_min_max.o -c find_min_max.c $(CFLAGS)
