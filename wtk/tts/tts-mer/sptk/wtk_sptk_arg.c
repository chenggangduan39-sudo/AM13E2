#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "tts-mer/sptk/SPTK.h"

void wtk_sptk_arg_delete(argv_t *at)
{
    int argc, i;
    argc = at->argc;
    for (i=0; i<argc; i++)
    {
        free(at->argvs[i]);
    }
    free(at->argvs);
    free(at);
}

argv_t* wtk_sptk_arg_new(char *cmnd)
{
    argv_t *dst;
    char *c = cmnd;
    char **argvs
       , *argv;
    int flag = 0 /* 连续空格只计一次 */
      , count = 0
      , len = 0
      , i = 0;
    while (*c != '\0')
    {
        if (isspace(*c))
        {
            if (flag == 0)
            {
                count++;
            }
            flag = 1;
        }
        else
        {
            flag = 0;
        }
        c++;
    }
    if (strlen(cmnd) > 0) { count += 1;}

    c = cmnd;
    argvs = malloc(count*sizeof(char*));
    dst = malloc(sizeof(argv_t));
    dst->argc = count;
    dst->argvs = argvs;
    // printf("cmnd: %s\n", c);
    while (1)
    {
         if (isspace(*c) || *c == '\0')
        {
            if (len > 0)
            {
                argv = malloc(sizeof(char)*(len+1));
                strncpy(argv, c-len, len);
                argv[len] = '\0';
                argvs[i] = argv;
                i++;
                len = 0;
            }
        } else { len++; }
        if (*c == '\0') break;
        c++;
    }
    return dst;
}
