#include "revert_string.h"
#include <string.h>

void RevertString(char *str)
{
	size_t len = strlen(str);
	
	char buffer;
	for (int i = 0; i < len/2; i++) {

		buffer = str[i];
		str[i] = str[len - (i + 1)];
		str[len - (i + 1)] = buffer;
	}
}

