; boot.s -- Brad Slayter
; This is the entry point of the kernel
; This does all the multiboot stuff

[BITS 32]
[GLOBAL start]
start:
	mov esp, _sys_stack
	jmp stublet

[GLOBAL mboot]
ALIGN 4
mboot:
	MBOOT_PAGE_ALIGN	equ 1<<0
	MBOOT_MEM_INFO		equ 1<<1
	MBOOT_HEADER_MAGIC	equ 0x1BADB002
	MBOOT_HEADER_FLAGS	equ MBOOT_PAGE_ALIGN | MBOOT_MEM_INFO
	MBOOT_CHECKSUM		equ -(MBOOT_HEADER_MAGIC + MBOOT_HEADER_FLAGS)
	[EXTERN code]
	[EXTERN bss]
	[EXTERN end]

	dd MBOOT_HEADER_MAGIC
	dd MBOOT_HEADER_FLAGS
	dd MBOOT_CHECKSUM

	dd mboot
	dd code
	dd bss
	dd end
	dd start

[EXTERN kmain]

stublet:
	push ebx

	cli
	call kmain ; Execute kernel
	jmp $

SECTION .bss
	resb 8192
_sys_stack: