.PHONY: all run datapath beqz clean compile gdb test

CC = gcc
CFLAGS = -Wall -Wextra

CPUMODEL = cpu_model
TESTPROGRAM = programs
COMPILER = compiler
INC = inc
TEST = test

# The possible ways to run the program is:
# 	make run [FILENAME=<filename>] [ROWS=<rows>]
# 	make datapath [ROWS=<rows>]
# 	make beqz [ROWS=<rows>]
FILENAME ?= "Datapath_Test.asm"
TESTFILE1 ?= "testprogram.asm"
ROWS ?= -1

# To add debug print info:
# 	make [options] [OPTIONS] [to_debug=yes]
to_debug ?= no
relative_jump ?= no
delayslot ?= 3

CFLAGS += -DDELAYSLOT$(delayslot)
ifeq ($(to_debug),yes)
    CFLAGS += -DDEBUG
endif
ifeq ($(relative_jump),yes)
    CFLAGS += -DRELATIVE_JUMP
endif

all: a.out $(COMPILER)/compiler.out $(TEST)/test.out
	find . -type f \( -name "*.o" \) -delete

run: all compile
	./a.out $(TESTPROGRAM)/$(FILENAME).mem $(ROWS)

datapath: all
	./a.out $(TESTPROGRAM)/Datapath_Test.asm.mem $(ROWS)

beqz: all
	./a.out $(TESTPROGRAM)/Branch_Test_beqz.asm.mem $(ROWS)

compile: all
	$(COMPILER)/compiler.out $(TESTPROGRAM)/$(FILENAME)

test: all compile_test
	./$(TEST)/test.out

compile_test: all
	$(COMPILER)/compiler.out $(TESTPROGRAM)/$(TESTFILE1)

clean:
	find . -type f \( -name "*.o" -o -name "*.out" \) -delete
	rm -r programs/*.mem

#
# main
#
a.out: main.o $(CPUMODEL)/cpu_model.o $(CPUMODEL)/cpu_utils.o $(INC)/utils.o
	$(CC) main.o $(CPUMODEL)/cpu_model.o $(CPUMODEL)/cpu_utils.o $(INC)/utils.o -o a.out

main.o: main.c $(CPUMODEL)/cpu_model.h
	$(CC) $(CFLAGS) -c main.c

$(INC)/utils.o: $(INC)/utils.c $(INC)/utils.h
	$(CC) $(CFLAGS) -c $(INC)/utils.c -o $(INC)/utils.o

#
# CPU model
#
$(CPUMODEL)/cpu_model.o: $(CPUMODEL)/cpu_model.c  $(CPUMODEL)/cpu_model.h
	$(CC) $(CFLAGS) -c $(CPUMODEL)/cpu_model.c -o $(CPUMODEL)/cpu_model.o

$(CPUMODEL)/cpu_utils.o: $(CPUMODEL)/cpu_utils.c $(CPUMODEL)/cpu_model.h
	$(CC) $(CFLAGS) -c $(CPUMODEL)/cpu_utils.c  -o $(CPUMODEL)/cpu_utils.o

#
# Compiler
#
$(COMPILER)/compiler.out: $(COMPILER)/compiler.o $(CPUMODEL)/cpu_model.h
	$(CC) $(CFLAGS) $(COMPILER)/compiler.o -o $(COMPILER)/compiler.out

$(COMPILER)/compiler.o: $(COMPILER)/compiler.c
	$(CC) $(CFLAGS) -c $(COMPILER)/compiler.c -o $(COMPILER)/compiler.o

#
# Test
#
$(TEST)/test.out: $(TEST)/test.o $(CPUMODEL)/cpu_model.o $(CPUMODEL)/cpu_utils.o $(INC)/utils.o
	$(CC) $(CPUMODEL)/cpu_model.o $(CPUMODEL)/cpu_utils.o $(TEST)/test.o $(INC)/utils.o -o $(TEST)/test.out

$(TEST)/test.o: $(TEST)/test.c $(TEST)/test.h
	$(CC) $(CFLAGS) -c $(TEST)/test.c -o $(TEST)/test.o


