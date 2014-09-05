// string.c -- Brad Slayter

#include "lib/string.h"
#include "lib/stdio.h"

int strcmp(const char *str1, const char *str2) {
	//kprintf(K_INFO, "Comparing %s and %s\n", str1, str2);

	int i = 0;
	int ret;
	while (str1[i]) {
		if (str1[i] == str2[i]) {
			ret = 0;
			//kprintf(K_INFO, "STRCMP() attempting to return %d\n", ret);
		} else if (str1[i] < str2[i]) {
			ret = -1;
			//kprintf(K_INFO, "STRCMP() attempting to return %d\n", ret);
			//kprintf(K_INFO, "str1[%d]: c=\'%c\' d=%d str2: c=\'%c\' d=%d\n", i, str1[i], str1[i], str2[i], str2[i]);
			break;
		} else if (str1[i] > str2[i]) {
			ret = 1;
			//kprintf(K_INFO, "STRCMP() attempting to return %d\n", ret);
			break;
		}
		i++;
	}

	if ((!str1[i] && str2[i]) && ret == 0) {
		ret = -1;
		//kprintf(K_INFO, "str1: c=\'%c\' d=%d str2(l): c=\'%c\' d=%d\n", str1[i], str1[i], str2[i], str2[i]);
		//kprintf(K_INFO, "STRCMP() attempting to return %d\n", ret);
	}

	return ret;
}
/*int strcmp (const char* str1, const char* str2) {

	kprintf(K_INFO, "Comparing %s and %s\n", str1, str2);

	int res=0;
	while (!(res = *(unsigned char*)str1 - *(unsigned char*)str2) && *str2)
		++str1, ++str2;

	if (res < 0)
		res = -1;
	else if (res > 0)
		res = 1;
	//else
	//	res = 0;

	kprintf(K_INFO, "STRCMP() attempting to return %x\n", res);

	return res;
}*/

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

char toupper(char c) {
	if (c >= 'a' && c <= 'z')
		return 'A' + (c - 'a');
	else
		return c;
}