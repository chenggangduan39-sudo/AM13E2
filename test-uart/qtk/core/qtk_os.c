#include "qtk_os.h"
#include <ctype.h>
#include <sys/time.h>
#define __USE_GNU
#include <sys/resource.h>

uint64_t file_length(FILE *f)
{
	uint64_t len;

    fseek(f, 0, SEEK_END);
    len = ftell(f);
    fseek(f, 0, SEEK_SET);
    //printf("len=%ld\n",len);
    return len;
}



char* file_read_buf(char* fn, int *n)
{
	FILE* file = fopen(fn, "rb");
	char* p = 0;
	int len;

	if (file)
	{
        len=file_length(file);
		p = (char*) malloc(len + 1);
		len = fread(p, 1, len, file);
		if (n)
		{
			*n = len;
		}
		fclose(file);
		p[len] = 0;
	}
	return p;
}







