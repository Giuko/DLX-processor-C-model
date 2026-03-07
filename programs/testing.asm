.define UART1   0x200


addi r1, r0, #104		; h
addi r2, r0, #101		; e
addi r3, r0, #108		; l
addi r4, r0, #108		; l
addi r5, r0, #111		; o
addi r6, r0, #32		; space
addi r7, r0, #119		; w
addi r8, r0, #111		; o
addi r9, r0, #114		; r
addi r10, r0, #108		; l
addi r11, r0, #100		; d
jal hello_world
nop
halt:
j halt
nop
hello_world:
addi r30, r0, #UART1	; UART1 address 
addi r12, r0, #10		; newline (\n)
sw r1, r30, #0
sw r2, r30, #0
sw r3, r30, #0
sw r4, r30, #0
sw r5, r30, #0
sw r6, r30, #0
sw r7, r30, #0
sw r8, r30, #0
sw r9, r30, #0
sw r10, r30, #0
sw r11, r30, #0
sw r12, r30, #0
jr r31
