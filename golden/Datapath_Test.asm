addi r1, r0, #5
subi r2, r0, #5
ori r3, r0, #12
xori r4, r0, #8
andi r5, r1, #4
sw 1(r0), r1
slli r6, r2, #1
srli r7, r3, #1
snei r8, r4, #1
slei r9, r5, #1
sgei r10, r6, #1
add r1, r1, r2
sub r2, r2, r3
or r3, r3, r4
xor r4, r4, r5
and r5, r5, r6 
sll r6, r6, r7
srl r7, r7, r8
sne r8, r8, r9
sle r9, r9, r10
sge r10, r10, r11
lw r21, 1(r0)
