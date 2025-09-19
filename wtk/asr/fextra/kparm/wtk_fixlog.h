#ifndef WTK_ASR_PARM_WTK_FIXLOG
#define WTK_ASR_PARM_WTK_FIXLOG
#include "wtk/core/wtk_type.h" 
#include "wtk/core/wtk_fixpoint.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_fixlog wtk_fixlog_t;
struct wtk_fixlog
{
	short n;
	short step;
	int vs;
	char shift;
	short *table;
};

int wtk_fixlog_bytes(wtk_fixlog_t *fixe);
void wtk_fixlog_write(wtk_fixlog_t *exp,FILE *f);
wtk_fixlog_t* wtk_fixlog_read(wtk_source_t *src);
wtk_fixlog_t* wtk_fixlog_new(char shift,int step);
void wtk_fixlog_delete(wtk_fixlog_t *fixl);
int wtk_fixlog_calc(wtk_fixlog_t *fixl,int v);

#ifdef __cplusplus
};
#endif
#endif
