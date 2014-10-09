; crtbegin.s -- Brad Slayter
; This is basically my version of crt0
; Lets give it a shot

[BITS 32]
[GLOBAL _start]
_start:
	extern main
	call main 	; should be done here
	mov eax, 3 	; exit syscall
	int 0x80	; gtfo