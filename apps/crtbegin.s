; crtbegin.s -- Brad Slayter
; This is basically my version of crt0
; Lets give it a shot

[BITS 32]
[GLOBAL _start]
_start:
;	pop eax ; I dont know if this is needed
;	add esp, 4
;	extern main
;	call main ; should be done here
;	jmp $
	mov eax, 3
	int 0x80