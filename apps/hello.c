#include "syscall.h"

int main() {
	syscall_mon_write("\nHello World!!!\n\n");

	for (int i = 0; i < 10; i++) {
		for (int j = 0; j < i; j++)
			syscall_mon_write(" ");
		
		syscall_mon_write(">\n");
	}

	return 0;
}