#include "syscall.h"

void swap(int *a, int *b) {
	int temp = *a;
	*a = *b;
	*b = temp;
}

void bubble_sort(int *A, int n) {
	int swapped;

	do {
		swapped = 0;
		for (int i = 1; i < n; i++) {
			if (A[i-1] > A[i]) {
				swap(&A[i-1], &A[i]);
				swapped = 1;
			}
		}
	} while (swapped);
}

int main() {
	int A[] = {32, 56, 43, 22, 55, 2, 4, 8, 11, 78};
	int n = 10;

	syscall_kprintf(4, "Original array:\n");
	for (int i = 0; i < n; i++) {
		syscall_mon_write_dec(A[i]);
		syscall_mon_write(" ");
	}

	syscall_kprintf(4, "\n\nSorted array:\n");
	bubble_sort(A, n);

	for (int i = 0; i < n; i++) {
		syscall_mon_write_dec(A[i]);
		syscall_mon_write(" ");
	}

	syscall_kprintf(4, "\n\n");

	return 0;
}