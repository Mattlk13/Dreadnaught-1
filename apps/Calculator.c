#include "syscall.h"

int main() {
	int *array = (int *)syscall_malloc(sizeof(int)*10);

	for (int i = 0; i < 10; i++) {
		array[i] = i * i;
	}

	for (int i = 0; i < 10; i++) {
		syscall_mon_write_dec(array[i]);
		syscall_mon_write("\n");
	}

	int average = 0;
	for (int i = 0; i < 10; i++)
		average += array[i];
	average /= 10;

	syscall_mon_write("The average is ");
	syscall_mon_write_dec(average);
	syscall_mon_write("\n");

	syscall_free(array);

	return 0;
}