/*
 * wtk_mtts_cfg.h
 *
 *  Created on: Dec 17, 2016
 *      Author: dm
 */

#ifndef WTK_TFIRE_KEL_WTK_MTTS_CFG_H_
#define WTK_TFIRE_KEL_WTK_MTTS_CFG_H_
#include "wtk/tts//wtk_tts_cfg.h"
#ifdef __cplusplus
extern "C"{
#endif
typedef struct wtk_mtts_cfg wtk_mtts_cfg_t;

struct wtk_mtts_cfg{
	wtk_main_cfg_t *main_cfg;
	char *cn;                   // cn tts path
	wtk_tts_cfg_t *cncfg;

	//for sole wav convert.
	char *swav;                 // sole wavdir
	float svol;                 //sole volume [0.5-3]
	float sshift;               //sole shift [0.7-1.4]
	float r_shift;              //rate btween cn and s in shift
	int mix_sil_time;           // for mix conn sil

	unsigned use_cnbin:1;       //use cn tts with bin res.
};

int wtk_mtts_cfg_init(wtk_mtts_cfg_t *cfg);
int wtk_mtts_cfg_clean(wtk_mtts_cfg_t *cfg);
int wtk_mtts_cfg_update_local(wtk_mtts_cfg_t *cfg, wtk_local_cfg_t *main);
int wtk_mtts_cfg_update(wtk_mtts_cfg_t *cfg);

wtk_mtts_cfg_t* wtk_mtts_cfg_new(char *cfg_fn);
void wtk_mtts_cfg_delete(wtk_mtts_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif /* WTK_mtts_CFG_H_ */
