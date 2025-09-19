#ifndef WTK_ASR_PARM_WTK_FIXEXP
#define WTK_ASR_PARM_WTK_FIXEXP
#include "wtk/core/wtk_type.h" 
#include "wtk/core/cfg/wtk_source.h"
#include <math.h>
#include "./wtk/core/wtk_fixpoint.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_fixexp wtk_fixexp_t;
struct wtk_fixexp
{
	short *table;
	short n;
	char shifti;
	char shifto;
	int vs;
	int step;
};

int wtk_fixe_bytes(wtk_fixexp_t *fixe);
wtk_fixexp_t* wtk_fixexp_new(char shifti,char shifto,int step);
void wtk_fixexp_delete(wtk_fixexp_t *exp);
int wtk_fixexp_calc(wtk_fixexp_t *exp,int v);

wtk_fixexp_t* wtk_fixexp_read(wtk_source_t *src);
void wtk_fixexp_write(wtk_fixexp_t *exp,FILE *f);

void wtk_fixexp_test(void);
#ifdef __cplusplus
};
#endif
#endif
