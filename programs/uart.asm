.define UART1_SLL 20
.reg ruart r26
.rodata
msg db "Hello, World!", 10, 0

.text
addi r1, r0, #msg       ; risolto automaticamente → indirizzo RODATA assoluto
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
addi ruart, r0, 1			
slli ruart, ruart, #UART1_SLL		; UART1 address 
addi r12, r0, #10				; newline (\n)
sw r1, ruart, #0
sw r2, ruart, #0
sw r3, ruart, #0
sw r4, ruart, #0
sw r5, ruart, #0
sw r6, ruart, #0
sw r7, ruart, #0
sw r8, ruart, #0
sw r9, ruart, #0
sw r10, ruart, #0
sw r11, ruart, #0
sw r12, ruart, #0
jr r31
