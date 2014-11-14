#include "syscall.h"

int main() {
	int n = 10, first = 0, second = 1, next, c;

	syscall_kprintf(4, "Computing the first 10 fibonachi numbers...\n");

	for (c = 0; c < n; c++) {
		if (c <= 1) {
			next = c;
		} else {
			next = first + second;
			first = second;
			second = next;
		}
		syscall_mon_write_dec(next);
		syscall_mon_write("\n");
	}

	syscall_mon_write("\n");
	return 0;
}