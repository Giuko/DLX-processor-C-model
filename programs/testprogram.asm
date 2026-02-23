; ============================================================
; testprogram.asm
; Assumptions:
;   - r0 = 0 hardwired
; ============================================================

; ------------------------------------------------------------
; SECTION 1: Basic arithmetich (ADDI / SUBI)
; ------------------------------------------------------------
addi r1, r0, #1          ; r1 = 1
addi r2, r0, #10         ; r2 = 10
subi r3, r0, #1          ; r3 = -1  (0xFFFFFFFF)
addi r4, r0, #7          ; r4 = 7
nop
nop
nop
nop

; ------------------------------------------------------------
; SECTION 2: R-type arithmetic (ADD / SUB)
; ------------------------------------------------------------
add  r5, r1, r2          ; r5 = 1 + 10 = 11
nop
nop
nop
nop
sub  r6, r2, r1          ; r6 = 10 - 1 = 9
nop
nop
nop
nop

; ------------------------------------------------------------
; SECTION 3: Logics R-type (AND / OR / XOR)
; ------------------------------------------------------------
addi r8,  r0, #0xFF      ; r8  = 255  (0x000000FF)
addi r9,  r0, #0x0F      ; r9  = 15   (0x0000000F)
nop
nop
nop
nop
and  r10, r8, r9         ; r10 = 0xFF & 0x0F = 0x0F = 15
nop
nop
nop
nop
or   r11, r8, r9         ; r11 = 0xFF | 0x0F = 0xFF = 255
nop
nop
nop
nop
xor  r12, r8, r9         ; r12 = 0xFF ^ 0x0F = 0xF0 = 240
nop
nop
nop
nop

; ------------------------------------------------------------
; SECTION 4: Logics with immediate (ANDI / ORI / XORI)
; ------------------------------------------------------------
andi r13, r8, #0x0F      ; r13 = 0xFF & 0x0F = 15
nop
nop
nop
nop
ori  r14, r0, #0xAA      ; r14 = 0x00 | 0xAA = 170
nop
nop
nop
nop
xori r15, r14, #0xFF     ; r15 = 0xAA ^ 0xFF = 0x55 = 85
nop
nop
nop
nop

; ------------------------------------------------------------
; SECTION 5: Shift (SLL / SRL / SRA)
; ------------------------------------------------------------
addi r16, r0, #1         ; r16 = 1
nop
nop
nop
nop
sll  r17, r16, r4        ; r17 = 1 << 7 = 128
nop
nop
nop
nop
srl  r18, r17, r4        ; r18 = 128 >> 7 = 1  (logical)
nop
nop
nop
nop
; SRA: arithmetic shift
addi r19, r0, #0         ; r19 = 0
subi r19, r19, #1        ; r19 = -1 = 0xFFFFFFFF
nop
nop
nop
nop
sra  r20, r19, r4        ; r20 = 0xFFFFFFFF >> 7 (arith) = 0xFFFFFFFF = -1
nop
nop
nop
nop

; Shift with immediate
slli r21, r1, #3         ; r21 = 1 << 3 = 8
nop
nop
nop
nop
srli r22, r21, #3        ; r22 = 8 >> 3 = 1
nop
nop
nop
nop
srai r23, r19, #4        ; r23 = 0xFFFFFFFF >> 4 (arith) = 0xFFFFFFFF = -1
nop
nop
nop
nop

; ------------------------------------------------------------
; SECTION 6: Set instructions (SEQ / SNE / SLT / SGT / SLE / SGE)
; ------------------------------------------------------------
addi r24, r0, #5         ; r24 = 5
addi r25, r0, #5         ; r25 = 5
addi r26, r0, #3         ; r26 = 3
nop
nop
nop
nop
seq  r27, r24, r25       ; r27 = (5 == 5) = 1
nop
nop
nop
nop
sne  r28, r24, r26       ; r28 = (5 != 3) = 1
nop
nop
nop
nop
slt  r29, r26, r24       ; r29 = (3 < 5)  = 1
nop
nop
nop
nop
sgt  r30, r24, r26       ; r30 = (5 > 3)  = 1
nop
nop
nop
nop

; ------------------------------------------------------------
; SECTION 7: Store and Load (SW / LW)
; ------------------------------------------------------------
; r1=1, r2=10, r5=11 already computed 
sw   r1,  r0, #0         ; MEM[0]  = 1
sw   r2,  r0, #4         ; MEM[4]  = 10
sw   r5,  r0, #8         ; MEM[8]  = 11
sw   r19, r0, #12        ; MEM[12] = 0xFFFFFFFF
nop
nop
nop
nop
lw   r1,  r0, #0         ; r1  = MEM[0]  = 1   
lw   r2,  r0, #4         ; r2  = MEM[4]  = 10   
lw   r7,  r0, #8         ; r7  = MEM[8]  = 11
lw   r8,  r0, #12        ; r8  = MEM[12] = 0xFFFFFFFF
nop
nop
nop
nop

; ------------------------------------------------------------
; SECTION 8: Branch (BEQZ / BNEZ) with 3 delay slot
; ------------------------------------------------------------
addi r31, r0, #0         ; r31 = 0  (iterator)
addi r28, r0, #2         ; r28 = 2  (limit)
nop
nop
nop
nop

LOOP:
addi r31, r31, #1        ; r31++
nop
nop
nop
nop
sub  r28, r28, r1        ; r28 = r28 - 1  (r1=1)
nop
nop
nop
nop
bnez r28, LOOP           ; se r28 != 0, jump LOOP
nop                      ; delay slot 1
nop                      ; delay slot 2
nop                      ; delay slot 3

; Dopo il loop: r31=2, r28=0

; ------------------------------------------------------------
; SECTION 9: Unsigned set (SLTU / SGTU)
; ------------------------------------------------------------
addi r1,  r0, #1         ; r1 = 1
addi r2,  r0, #2         ; r2 = 2
nop
nop
nop
nop
sltu r3,  r1,  r2        ; r3 = (1 <u 2) = 1
nop
nop
nop
nop
sgtu r4,  r2,  r1        ; r4 = (2 >u 1) = 1
nop
nop
nop
nop

; ------------------------------------------------------------
; END PROGRAM
; ------------------------------------------------------------
nop
nop
nop
nop
