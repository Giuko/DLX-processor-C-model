.PHONY: all run

CC = gcc
CFLAGS = -Wall -Wextra

# The possible ways to run the program is:
# 	make run [FILENAME=<filename>] [ROWS=<rows>]
# 	make datapath [ROWS=<rows>]
# 	make beqz [ROWS=<rows>]
FILENAME ?= "Datapath_Test.bin.mem"
ROWS ?= -1

# To add debug print info:
# 	make [options] [OPTIONS] [to_debug=yes]
to_debug ?= no

ifeq ($(to_debug),yes)
    CFLAGS += -DDEBUG
endif

all: a.out
	rm -f *.o

run: all
	./a.out $(FILENAME) $(ROWS)
datapath: all
	./a.out Datapath_Test.bin.mem $(ROWS)

beqz: all
	./a.out Branch_Test_beqz.bin.mem $(ROWS)

clean:
	rm -f *.o a.out

a.out: main.o cpu_model.o
	$(CC) main.o cpu_model.o -o a.out

main.o: main.c cpu_model.h
	$(CC) $(CFLAGS) -c main.c 

cpu_model.o: cpu_model.c cpu_model.h
	$(CC) $(CFLAGS) -c cpu_model.c
