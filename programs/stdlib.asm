;--------------------------------------------------------------------------------
; DLX Standard Library
; Calling convention:
;   Arguments:    a0=r4, a1=r5, a2=r6, a3=r7
;   Return value: v0=r2
;   Saved regs:   r16-r23 (callee must preserve)
;   Temporaries:  r8-r15  (caller-saved)
;   SP:           r29
;   RA:           r31
;
; String format: null-terminated, 4 chars packed per word, little-endian
;   word = [ char0(byte0) | char1(byte1) | char2(byte2) | char3(byte3) ]
;   Termination: detected when byte3 (the MSB) of a word is 0x00
;   Strings must be padded to a multiple of 4 chars, with 0x00 in byte3
;   of the last word as the null terminator.
;
; Example: "Hello\0\0\0"
;   word0 = 0x6C6C6548  ('H','e','l','l')
;   word1 = 0x00006F6F  ('o','\0','\0','\0')  ← byte3=0 → stop after this word
;--------------------------------------------------------------------------------
.define UART_TX_SLL  20         ; UART1_TX = 1 << 20 = 0x00100000

.text

;--------------------------------------------------------------------------------
; print_char
;   Print a single character to UART.
;   Arguments:  a0 (r4) = character value (low byte used)
;   Clobbers:   r8
;--------------------------------------------------------------------------------
print_char:
    addi r8, r0, #1
    slli r8, r8, #UART_TX_SLL          ; r8 = UART TX address (1 << 20)
    sw   r4, r8, #0                    ; write char to UART
    jr   ra
    nop

;--------------------------------------------------------------------------------
; strlen
;   Count words in a packed string until byte3 of a word is 0x00.
;   Returns the count in words (not bytes, not chars).
;   Arguments:  a0 (r4) = base address of string
;   Returns:    v0 (r2) = length in words (not counting the terminating word)
;   Clobbers:   r8, r9
;--------------------------------------------------------------------------------
strlen:
    addi r2, r0, #0                    ; r2 = word count
    addi r8, r4, #0                    ; r8 = current pointer
strlen_loop:
    lw   r9, r8, #0                    ; r9 = current word
    srli r9, r9, #24                   ; isolate byte3 (MSB)
    beqz r9, strlen_done               ; byte3 == 0 → this is the last word
    nop
    addi r2, r2, #1                    ; count this word
    addi r8, r8, #1                    ; advance pointer
    j    strlen_loop
    nop
strlen_done:
    jr   ra
    nop

;--------------------------------------------------------------------------------
; print_string
;   Print a null-terminated packed string (4 chars/word) to UART.
;   Each word is unpacked into 4 bytes sent sequentially.
;   Stops after sending the word whose byte3 is 0x00 (sends up to that 0).
;
;   Arguments:  a0 (r4) = base address of string
;   Clobbers:   r8, r9, r10, r11
;   Does NOT call other functions → no need to save ra
;--------------------------------------------------------------------------------
print_string:
    addi r8,  r0, #1
    slli r8,  r8, #UART_TX_SLL        ; r8  = UART TX address
    addi r10, r4, #0                   ; r10 = current word pointer

print_string_loop:
    lw   r9,  r10, #0                  ; r9 = word (4 chars)
    nop
    nop                                ; data ready (forwarding delay)
    nop

    ; byte 0 (bits 7:0)
    andi r11, r9,  #0xFF
    sw   r11, r8,  #0

    ; byte 1 (bits 15:8)
    srli r9,  r9,  #8
    andi r11, r9,  #0xFF
    sw   r11, r8,  #0

    ; byte 2 (bits 23:16)
    srli r9,  r9,  #8
    andi r11, r9,  #0xFF
    sw   r11, r8,  #0

    ; byte 3 (bits 31:24) — null terminator check
    srli r9,  r9,  #8
    andi r11, r9,  #0xFF
    sw   r11, r8,  #0
    bnez r11, print_string_next        ; if byte3 != 0 → continue
    nop
    j    print_string_done             ; byte3 == 0 → end of string
    nop

print_string_next:
    addi r10, r10, #1                  ; next word
    j    print_string_loop
    nop

print_string_done:
    jr   ra
    nop
