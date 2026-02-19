.PHONY: all run

CC = gcc
CFLAGS = -Wall -Wextra
FILENAME ?= "Datapath_Test.bin.mem"

all: a.out
	rm -f *.o

run: all
	./a.out $(FILENAME)

clean:
	rm -f *.o a.out

a.out: main.o cpu_model.o
	$(CC) main.o cpu_model.o -o a.out

main.o: main.c cpu_model.h
	$(CC) $(CFLAGS) -c main.c

cpu_model.o: cpu_model.c cpu_model.h
	$(CC) $(CFLAGS) -c cpu_model.c
