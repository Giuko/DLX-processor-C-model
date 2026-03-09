.define UART1_SLL 20
.reg UART_reg r28
.reg BUFFER_reg r27

.rodata
msg1 db "Hello, World!", 10, 0
msg2 db "Second string!", 10, 0
msg3 db "Third string!", 10, 0
msg4 db "Random string!", 10, 0

.text
addi BUFFER_reg, zero, #msg1
jal print
nop
addi BUFFER_reg, zero, #msg2
jal print
nop
addi BUFFER_reg, zero, #msg3
jal print
nop
addi BUFFER_reg, zero, #msg4
jal print
nop

halt:
j halt
nop

print:
addi UART_reg, zero, 1			
slli UART_reg, UART_reg, #UART1_SLL		; UART1 address 
loop:
lw r1, BUFFER_reg, #0					; loading word
nop
nop								; data is ready
nop

; First byte
andi r2, r1, 0xff				; get byte
sw r2, UART_reg, #0

; Second byte
srli r1, r1, #8
andi r2, r1, 0xff				; get byte
sw r2, UART_reg, #0

; Third byte
srli r1, r1, #8
andi r2, r1, 0xff				; get byte
sw r2, UART_reg, #0

; Fourth byte
srli r1, r1, #8
andi r2, r1, 0xff				; get byte
sw r2, UART_reg, #0

addi BUFFER_reg, BUFFER_reg, #1
bnez r2, loop					; null terminator
nop

return:
jr ra
nop
