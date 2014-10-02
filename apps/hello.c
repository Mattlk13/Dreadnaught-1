int main() {
	char *str = "\n\n\nHelloWorld.exe\n\nI\'m out!\n\n";

	asm(" \
		mov %0, %%ebx; \
		mov 2, %%eax; \
		int $0x80; \
		":: "r" (str));

	asm(" \
		mov 3, %eax; \
		int $0x80; \
		");

	for (;;);
}