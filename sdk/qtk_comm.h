#ifndef __QTK_COMM_H__
#define __QTK_COMM_H__
#ifdef __cplusplus
extern "C" {
#endif

#include "wtk/core/wtk_type.h"
#include <string.h>

typedef struct qtk_comm qtk_comm_t;
struct qtk_comm
{
        char *comm;
        char *num;
};

int qtk_comm_get_number(char *input,int ilen,char *output, int *olen);

#ifdef __cplusplus
};
#endif
#endif
