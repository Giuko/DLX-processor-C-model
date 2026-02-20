.PHONY: all run datapath beqz clean compile

CC = gcc
CFLAGS = -Wall -Wextra

CPUMODEL = cpu_model
TESTPROGRAM = test_propram
COMPILER = compiler

# The possible ways to run the program is:
# 	make run [FILENAME=<filename>] [ROWS=<rows>]
# 	make datapath [ROWS=<rows>]
# 	make beqz [ROWS=<rows>]
FILENAME ?= "Datapath_Test.bin.mem"
FILENAMECOMPILE ?= "Datapath_Test.asm"
ROWS ?= -1

# To add debug print info:
# 	make [options] [OPTIONS] [to_debug=yes]
to_debug ?= no

ifeq ($(to_debug),yes)
    CFLAGS += -DDEBUG
endif

all: a.out basicallyGDB.out $(COMPILER)/compiler.out
	rm -f *.o

run: all
	./a.out $(TESTPROGRAM)/$(FILENAME) $(ROWS)

gdb: all
	./basicallyGDB.out $(TESTPROGRAM)/$(FILENAME) $(ROWS)

datapath: all
	./a.out $(TESTPROGRAM)/Datapath_Test.asm.mem $(ROWS)

beqz: all
	./a.out $(TESTPROGRAM)/Branch_Test_beqz.asm.mem $(ROWS)

compile: all
	$(COMPILER)/compiler.out $(TESTPROGRAM)/$(FILENAMECOMPILE)

clean:
	rm -f *.o a.out

basicallyGDB.out: basicallyGDB.o cpu_model.o
	$(CC) basicallyGDB.o cpu_model.o -o basicallyGDB.out

basicallyGDB.o: basicallyGDB.c $(CPUMODEL)/cpu_model.h
	$(CC) $(CFLAGS) -c basicallyGDB.c 

a.out: main.o cpu_model.o
	$(CC) main.o cpu_model.o -o a.out

main.o: main.c $(CPUMODEL)/cpu_model.h
	$(CC) $(CFLAGS) -c main.c 

cpu_model.o: $(CPUMODEL)/cpu_model.c $(CPUMODEL)/cpu_model.h
	$(CC) $(CFLAGS) -c $(CPUMODEL)/cpu_model.c

$(COMPILER)/compiler.out: $(COMPILER)/compiler.o $(CPUMODEL)/cpu_model.h
	$(CC) $(CFLAGS) compiler.o -o $(COMPILER)/compiler.out

$(COMPILER)/compiler.o: $(COMPILER)/compiler.c
	$(CC) $(CFLAGS) -c $(COMPILER)/compiler.c
