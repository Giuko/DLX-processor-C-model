
.rodata
msg  db "Hello, World!", 10, 0
msg2 db "Press s to continue", 10, 0

.text
addi sp, r0, #0x0FFF    ; init stack pointer
addi a0, r0, #msg       ; a0 = indirizzo stringa
jal  print_string
nop
addi a0, r0, #msg2      ; a0 = indirizzo stringa
jal  print_string
nop
halt:
j halt
nop

; Include section
.include "stdlib.asm"
