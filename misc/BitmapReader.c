#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>

int main(int argc, char const *argv[]) {
	if (argc < 2) {
		fprintf(stderr, "[usage] %s <filename>\n", argv[0]);
		exit(1);
	}

	int fd = open(argv[1], O_RDONLY);
	if (fd == -1) {
		fprintf(stderr, "Error opening file: %s\n", argv[1]);
		exit(1);
	}

	off_t fsize = lseek(fd, 0, SEEK_END);
	printf("FileSize: %jd\n", fsize);
	char *buffer = (char *)malloc(fsize+1);
	read(fd, &buffer, fsize);
	close(fd);

	int i = 0, j, k, buffOff = 1;
	int numBmps = (int)buffer[0];
	printf("Reading...\n");
	for (i = 0; i < numBmps; i++) {
		int w = (int)buffer[buffOff++];
		int h = (int)buffer[buffOff++];

		for (j = 0; j < w; j++) {
			for (k = 0; k < h; k++)  {
				printf("%c", (buffer[buffOff++] == 0) ? '0' : '1');
			}
			printf("\n");
		}
	}

	return 0;
}