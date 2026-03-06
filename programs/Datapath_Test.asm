addi r1, r0, 0x15
addi r30, r0, #1
subi r2, r0, #15
ori r3, r0, #12
xori r4, r0, #8
andi r5, r1, #4
sw r1, r1, #1
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
lw r21, r0, #6
slli r30, r30, #28			; R30 = UART_TX 
addi r1, r0, #72			; H
addi r2, r0, #101			; e
addi r3, r0, #108			; l
addi r4, r0, #108			; l
addi r5, r0, #111			; o
addi r6, r0, #32			;  
addi r7, r0, #119			; w
addi r8, r0, #111			; o
addi r9, r0, #114			; r
addi r10, r0, #108			; l
addi r11, r0, #100			; d
addi r12, r0, #10			; \n
sw r1, r30, 0x0
sw r2, r30, 0x0
sw r3, r30, 0x0
sw r4, r30, 0x0
sw r5, r30, 0x0
sw r6, r30, 0x0
sw r7, r30, 0x0
sw r8, r30, 0x0
sw r9, r30, 0x0
sw r10, r30, 0x0
sw r11, r30, 0x0
sw r12, r30, 0x0
