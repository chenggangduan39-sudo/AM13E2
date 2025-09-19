#ifndef WTK_VITE_F0_POST_WTK_FPOST_H_
#define WTK_VITE_F0_POST_WTK_FPOST_H_
#include "wtk/core/wtk_type.h"
#include "wtk_fpost_cfg.h"
#include "extrPFfeat.h"
#include "splitf02.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_fpost wtk_fpost_t;
struct wtk_f0;

struct wtk_fpost
{
	wtk_fpost_cfg_t *cfg;
	struct wtk_f0 *f0;
	int nframe;
	double *f;		//F0
	double *fe;		//F0 energy
	double *f_delta;		//delta or f;
	//double *fe_delta;		//delta of fe;
	//--------- guass rand ----------
	double v1;
	double v2;
	double s;
	int phase;
};

wtk_fpost_t* wtk_fpost_new(wtk_fpost_cfg_t *cfg,struct wtk_f0 *f0);
void wtk_fpost_delete(wtk_fpost_t *p);
void wtk_fpost_reset(wtk_fpost_t *p);
int wtk_fpost_process(wtk_fpost_t *p,int nwrd);
void wtk_fpost_print(wtk_fpost_t *p);
#ifdef __cplusplus
};
#endif
#endif
