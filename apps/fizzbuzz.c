#include "syscall.h"

int main() {
	int n = 25;

	syscall_kprintf(4, "Performing Fizz Buzz to 25\n");

	for (int i = 0; i < n; i++) {
		if (i % 3 == 0 && i % 5 == 0) {
			syscall_kprintf(4, "Fizz Buzz\n");
		} else if (i % 3 == 0) {
			syscall_kprintf(4, "Fizz\n");
		} else if (i % 5 == 0) {
			syscall_kprintf(4, "Buzz\n");
		} else {
			syscall_mon_write_dec(i);
			syscall_kprintf(4, "\n");
		}
	}

	syscall_kprintf(4, "\n");

	return 0;
}