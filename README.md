# Program Execution Guide

## Detail
This software aim to model a DLX 32-bit processor to use it as simulation and verification of a HDL model.
The basic behavior of a DLX (very simple one) is already reached. The actual task is to create a model in C, with some
compile flags able to customize it. For example with or without FP, with or without branch prediction and so on.

To debug it, some program needs to be tested. In this repo you can find a compiler, not every instruction is available.
It expects: 
- **RTYPE**: *Instruction RD, RS1, RS2*
- **ITYPE**: *Instruction RD, RS, #imm*
- **JTYPE**: *Instruction label*


Some particular attention on SW, LW:
- *SW RS, R\_off, imm*
- *LW RD, R\_off, imm*

It means that the address of the memory is computed as R\_off+imm.

## Available Commands

You can run the program using the following `make` targets:

```bash
make run [FILENAME=<filename>] [ROWS=<rows>]
make datapath [ROWS=<rows>]
make beqz [ROWS=<rows>]
```

To enable debug print information:

```bash
make <target> [options] to_debug=yes
```

---

## Available Options

|          Option           |                      Description                      |        Default values        |
|---------------------------|-------------------------------------------------------|------------------------------|
| `FILENAME=<filename>`     | Assembly file located in the `test_program` folder    | `Datapath_Test.asm` |
| `ROWS=<rows>`             | Number of instructions (rows) to execute. Use `-1` to run the entire program | `-1` |
| `to_debug=<yes/no>`       | Enable extra debug output                             | `no` |
| `delayslot=<1\|2\|3>`     | Select CPU delay slot model to simulate               | `1` |
| `relative_jump=<yes/no>`  | Controls jump address calculation:<br>• `yes` → compute as `addr + imm`<br>• `no` → compute as `imm` | `yes` |
| `using_uart1=<yes/no>`    | Enable the usage of a UART output to another terminal, use `nc localhost 5555`  | `no` |
| `forwarding=<yes/no>`     | Enable the forwaring to the decode stage  | `yes` |

---

## Examples

Run the full default program:

```bash
make run 
```

Run a specific test file with debugging enabled:

```bash
make run FILENAME=example.asm to_debug=yes
```

Run only the first 20 rows with delay slot model 1:

```bash
make run ROWS=20 delayslot=1
```

Use relative jump addressing:

```bash
make run relative_jump=yes
```

---

## Notes

- All test files must be placed inside the `programs` directory.
- `ROWS=-1` ensures the entire program executes.
- Debug mode (`to_debug=yes`) provides additional internal execution details.
- The `delayslot` parameter allows you to simulate different CPU architectural behaviors.
- The `relative_jump` option affects both the compiler and the CPU hardware model.

## Programming notes
- DRAM base address : 0x0000 0000
- UART1 base address: 0x1000 0000
