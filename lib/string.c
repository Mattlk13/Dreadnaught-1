// string.c -- Brad Slayter

#include "lib/string.h"
#include "lib/stdio.h"

int strcmp(char *str1, char *str2) {
	int i = 0;
	int ret;
	while (str1[i]) {
		if (str1[i] == str2[i])
			ret = 0;
		else if (str1[i] < str2[i]) {
			ret = -1;
			break;
		} else if (str1[i] > str2[i]) {
			ret = 1;
			break;
		}
		i++;
	}

	if ((!str1[i] && str2[i]) && ret == 0)
		ret = -1;

	return ret;
}

char *strcpy(char *dest, const char *src) {
    do {
      *dest++ = *src++;
    } while (*src != 0);
}

char *strcat(char *dest, const char *source) {
	char *a;
	for (a = dest; *a; a++)
		;

	for (; *source; a++, source++)
		*a = *source;
	*a = 0;

	return dest;
}