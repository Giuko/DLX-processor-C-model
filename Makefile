.PHONY: all run datapath beqz clean compile test build_init

#####################
# Compile options
#####################
CC = gcc
CFLAGS = -Wall -Wextra

FILENAME ?= "Datapath_Test.asm"
TESTFILE1 ?= "testprogram.asm"
ROWS ?= -1

to_debug ?= no
relative_jump ?= yes
delayslot ?= 1
using_uart1 ?= no
forwarding ?= yes
avoid_print ?= no

CFLAGS += -DDELAYSLOT$(delayslot)
ifeq ($(to_debug),yes)
    CFLAGS += -DDEBUG
endif
ifeq ($(relative_jump),yes)
    CFLAGS += -DRELATIVE_JUMP
endif
ifeq ($(using_uart1),yes)
    CFLAGS += -DUSING_UART1
endif
ifeq ($(forwarding),yes)
    CFLAGS += -DFORWARDING
endif
ifeq ($(avoid_print),yes)
    CFLAGS += -DAVOID_PRINT
endif

#####################
# Folders 
#####################
# Main folders
SRC		= src
INC		= inc
TEST 	= test
BUILD	= build
TESTPROGRAM = programs

CFLAGS += -I$(INC)

# Subfolders
CPUMODEL 	= cpu_model
COMPILER 	= compiler
EXTRA 		= extra

#####################
# Folders Peripherals 
#####################
PERIPHERAL = peripherals

#
# UART
#
BUS = $(CPUMODEL)/$(PERIPHERAL)/bus

#
# Memory
#
MEMORY = $(CPUMODEL)/$(PERIPHERAL)/memory

#
# UART
#
UART = $(CPUMODEL)/$(PERIPHERAL)/uart

#####################
# Variables
#####################
#
# Peripherals
#
UART_OBJS = $(BUILD)/$(UART)/uart.o															# UART objs
BUS_OBJS = $(BUILD)/$(BUS)/bus.o															# Bus objs
MEM_OBJS = $(BUILD)/$(MEMORY)/memory.o														# Memory objs

PER_OBJS = $(MEM_OBJS) $(BUS_OBJS) $(UART_OBJS)												# All of the peripherals
CPU_OBJS = $(BUILD)/$(CPUMODEL)/cpu_model.o $(BUILD)/$(CPUMODEL)/cpu_utils.o $(PER_OBJS)	# Everything needed to compile CPU
APP_OBJS = $(CPU_OBJS) $(BUILD)/$(EXTRA)/utils.o											# Minimal objectes for any app 

#####################
# Execution options
#####################
all: clean build_init $(BUILD)/a.out $(BUILD)/$(COMPILER)/compiler.out $(BUILD)/$(TEST)/test.out compile

run: all
	./$(BUILD)/a.out $(TESTPROGRAM)/$(FILENAME).mem $(ROWS)

datapath: all
	./$(BUILD)/a.out $(TESTPROGRAM)/Datapath_Test.asm.mem $(ROWS)

beqz: all
	./$(BUILD)/a.out $(TESTPROGRAM)/Branch_Test_beqz.asm.mem $(ROWS)

compile: all
	$(BUILD)/$(COMPILER)/compiler.out $(TESTPROGRAM)/$(FILENAME)

test: all compile_test
	./$(BUILD)/$(TEST)/test.out

compile_test: all
	$(BUILD)/$(COMPILER)/compiler.out $(TESTPROGRAM)/$(TESTFILE1)

clean:
	rm -rf $(BUILD)
	rm -rf $(TESTPROGRAM)/*.mem

build_init:
	mkdir -p $(BUILD)/$(CPUMODEL)
	mkdir -p $(BUILD)/$(COMPILER)
	mkdir -p $(BUILD)/$(EXTRA)
	mkdir -p $(BUILD)/$(TEST)
	mkdir -p $(BUILD)/$(MEMORY)
	mkdir -p $(BUILD)/$(UART)
	mkdir -p $(BUILD)/$(BUS)

#####################
# Compiling Files
#####################
#
# main
#
$(BUILD)/a.out: $(BUILD)/main.o $(APP_OBJS)
	$(CC) $(BUILD)/main.o $(APP_OBJS) -o $(BUILD)/a.out

$(BUILD)/main.o: $(SRC)/main.c $(INC)/$(CPUMODEL)/cpu_model.h
	$(CC) $(CFLAGS) -c $(SRC)/main.c -o $(BUILD)/main.o

#
# CPU model
#
$(BUILD)/$(CPUMODEL)/cpu_model.o: $(SRC)/$(CPUMODEL)/cpu_model.c  $(INC)/$(CPUMODEL)/cpu_model.h
	$(CC) $(CFLAGS) -c $(SRC)/$(CPUMODEL)/cpu_model.c -o $(BUILD)/$(CPUMODEL)/cpu_model.o

$(BUILD)/$(CPUMODEL)/cpu_utils.o: $(SRC)/$(CPUMODEL)/cpu_utils.c $(INC)/$(CPUMODEL)/cpu_model.h
	$(CC) $(CFLAGS) -c $(SRC)/$(CPUMODEL)/cpu_utils.c  -o $(BUILD)/$(CPUMODEL)/cpu_utils.o

#
# Compiler
#
$(BUILD)/$(COMPILER)/compiler.out: $(BUILD)/$(COMPILER)/compiler.o $(INC)/$(CPUMODEL)/cpu_model.h
	$(CC) $(CFLAGS) $(BUILD)/$(COMPILER)/compiler.o -o $(BUILD)/$(COMPILER)/compiler.out

$(BUILD)/$(COMPILER)/compiler.o: $(SRC)/$(COMPILER)/compiler.c
	$(CC) $(CFLAGS) -c $(SRC)/$(COMPILER)/compiler.c -o $(BUILD)/$(COMPILER)/compiler.o

#
# Test
#
$(BUILD)/$(TEST)/test.out: $(BUILD)/$(TEST)/test.o $(APP_OBJS)
	$(CC) $(APP_OBJS) $(BUILD)/$(TEST)/test.o -o $(BUILD)/$(TEST)/test.out

$(BUILD)/$(TEST)/test.o: $(TEST)/test.c $(INC)/$(TEST)/test.h
	$(CC) $(CFLAGS) -c $(TEST)/test.c -o $(BUILD)/$(TEST)/test.o

#####################
# Extra 
#####################
$(BUILD)/$(EXTRA)/utils.o: $(SRC)/$(EXTRA)/utils.c $(INC)/$(EXTRA)/utils.h
	$(CC) $(CFLAGS) -c $(SRC)/$(EXTRA)/utils.c -o $(BUILD)/$(EXTRA)/utils.o


#####################
# Peripherals
#####################

#
# Bus
#
$(BUILD)/$(BUS)/bus.o: $(SRC)/$(BUS)/bus.c $(INC)/$(BUS)/bus.h
	$(CC) $(CFLAGS) -c $(SRC)/$(BUS)/bus.c -o $(BUILD)/$(BUS)/bus.o

#
# Memory
#
$(BUILD)/$(MEMORY)/memory.o: $(SRC)/$(MEMORY)/memory.c $(INC)/$(MEMORY)/memory.h
	$(CC) $(CFLAGS) -c $(SRC)/$(MEMORY)/memory.c -o $(BUILD)/$(MEMORY)/memory.o

#
# UART
#
$(BUILD)/$(UART)/uart.o: $(SRC)/$(UART)/uart.c $(INC)/$(UART)/uart.h
	$(CC) $(CFLAGS) -c $(SRC)/$(UART)/uart.c -o $(BUILD)/$(UART)/uart.o

$(BUILD)/$(UART)/main.o: $(SRC)/$(UART)/main.c $(INC)/$(UART)/uart.h
	$(CC) $(CFLAGS) -c $(SRC)/$(UART)/main.c -o $(BUILD)/$(UART)/main.o

