addi r1, r0, #1
subi r2, r0, #1
sub r3, r1, r2
nop
nop
sw r1, r3, #2					; Store R1 (1) in [R3] + #2 (2+2 = 4) 
addi r4, r0, #1671668
addi r5, r0, #0
L1:
addi r6, r0, #1
nop
nop
nop
nop
nop
addi r6, r6, #1		
nop
nop
nop
nop
nop			
addi r5, r0, #1
beqz r5, L1
nop
nop
nop
xor r4, r4, r4
