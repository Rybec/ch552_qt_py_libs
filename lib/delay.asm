	.module delay

	.globl delay_us
	.globl delay_ms
	.globl _delay_us
	.globl _delay_ms

_delay_us = delay_us
_delay_ms = delay_ms


; Set beginning of data area (register segment)
	.area RSEG	(ABS, DATA)
	.org 0x0000

; Allocate register banks
	.area REG_BANK_0	(REL, OVR, DATA)
	.ds 8

;	.area REG_BANK_1	(REL, OVR, DATA)
;	.ds 8

;	.area REG_BANK_2	(REL, OVR, DATA)
;	.ds 8

;	.area REG_BANK_3	(REL, OVR, DATA)
;	.ds 8


	.area TEXT	(CODE)

; Expects microsecond value in r0 (if r0 = 0, 256us delay)
; Changes r0, a, and b
; Tuned to 16MHz
; Delays 1 or 2 extra cycles (1/16 or 1/8 microsecond)
	.even  ; This is to make jump cycles predictable
delay_us:
	nop                ; [1] 1 byte        0
	nop                ; [1] 1 byte        1
	nop                ; [1] 1 byte        0
	div ab             ; [4] 1 byte        1
	div ab             ; [4] 1 byte        0
	djnz r0, delay_us  ; [2 | 5] 2 bytes   1
	ret                ; [4/5(target)]     1


; Expects millisecond value in r0 (if r0 = 0, 256ms delay)
; Changes r0, r1, a, and b
; Tuned to 16MHz
; Delays 4 or 5 cycles extra
	.even
delay_ms:
	; Move r0 into r1 (delay_us does not touch r1)
	mov a, r0               ; [1] 1 byte
	mov r1, a               ; [1] 1 byte
delay_ms_loop:
	mov r0, #250            ; [2] 2 bytes
	acall delay_us          ; [4] 2 bytes (+4001 cycles)

	mov r0, #250            ; [2] 2 bytes
	acall delay_us          ; [4] 2 bytes (+4001 cycles)

	mov r0, #250            ; [2] 2 bytes
	acall delay_us          ; [4] 2 bytes (+4001 cycles)

	mov r0, #248            ; [2] 2 bytes
	acall delay_us          ; [4] 2 bytes (+3969 cycles)

	djnz r1, delay_ms_loop  ; [2 | 4] 2 bytes
	ret                     ; [4/5(target)]

