#include <stdio.h>
#include <stdlib.h>

int main(int argc, char const *argv[]) {
	if (argc < 2) {
		fprintf(stderr, "[usage] %s <filename>\n", argv[0]);
		exit(1);
	}

	int numBmp;
	printf("How many bitmaps: ");
	scanf("%d", &numBmp);

	FILE *f = fopen(argv[1], "w");
	if (f == NULL) {
		fprintf(stderr, "Error opening file: %s\n", argv[1]);
		exit(1);
	}

	fprintf(f, "%c", (char)numBmp);

	int i;
	for (i = 0; i < numBmp; i++) {
		int w, h;
		printf("Enter bitmap[%d] width: ", i+1);
		scanf("%d", &w);
		printf("Enter bitmap[%d] height: ", i+1);
		scanf("%d", &h);

		fprintf(f, "%c", (char)w);
		fprintf(f, "%c", (char)h);

		int j;
		for (j = 0; j < h; j++) {
			printf("Enter line %d: ", i);
			char *line = (char *)malloc(w+1);
			scanf("%s", line);

			int k = 0;
			while (line[k++])
				fprintf(f, "%c", (char)((line[i] == '0') ? 0 : 15));

			free(line);
		}
	}

	fclose(f);

	return 0;
}