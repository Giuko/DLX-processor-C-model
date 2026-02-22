addi r1, r0, #2
nop
nop
nop
nop
nop		; r1 written back
L1: 
subi r1, r1, #1
nop
nop
nop
nop
nop		; r1 written back
bnez r1, L1	
ori r2, r1, #4
andi r3, r1, #4
xori r4, r1, #4
ori r5, r1, #4
andi r6, r1, #4
xori r7, r1, #4
