The possible ways to run the program is:
    - make run [FILENAME=<filename>] [ROWS=<rows>]
    - make datapath [ROWS=<rows>]
    - make beqz [ROWS=<rows>]

To add debug print info:
 	- make [options] [OPTIONS] [to\_debug=yes]

At the moment the best way to run the program is:
    make gdb [options]

Where the options are:
    - **FILENAME=<filename>**: The filename should be in *test\_program* folder
    - **ROWS=<rows>**: You can decide how many row to execute
    - **to_debug=<yes or no>**: To print extra debug info (insert *yes* to do it)
    - **delayslot=<num>**: It can be 1, 2, or 3. You can decide which model of CPU you're going to simulate
    - **relative_jump=<yes or no>**: You can decide if the address you're going to jump should be computed as addr+imm or as imm, both in compiler and within the CPU hardware model

The default values are:
    - **FILENAME**=Datapath\_Test.asm
    - **ROWS**=-1 (It means run all of the program)
    - **to_debug**=no
    - **delayslot**=3
    - **relative_jump**=no
