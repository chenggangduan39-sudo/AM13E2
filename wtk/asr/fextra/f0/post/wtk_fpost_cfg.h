#ifndef WTK_VITE_F0_POST_WTK_FPOST_CFG_H_
#define WTK_VITE_F0_POST_WTK_FPOST_CFG_H_
#include "wtk/core/cfg/wtk_local_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_fpost_cfg wtk_fpost_cfg_t;

/*
#for wtone
wEngThd=25.0			//Energy thresh
wPlsFltThd=30			//Pls thresh for f0
wMinVocThd=5			//min voice dur f0
wEngyRatio=0.1			//energy sil,speech
wUvMergrThd=0			//unvoice thresh
wAvgVocDur=5			//average voice dur
wNosType=1
wGlbMean=-1.631121e-03 #tontrn10aug_wtone
wGlbVar=7.200515e-02   #tontrn10aug_wtone
#END_OF_CONFIG_FOR_POSTF0
 */

typedef struct
{
	int min_voice_dur;		//wMinVocThd: f0 min duration of voice
	int pls_thresh;			//wPlsFltThd: f0 pls thresh;
	float energy_thresh;	//wEngThd: energy thresh;
	float energy_ratio;		//wEngyRatio: energy ration of sil,speech
	float unvoice_thresh;	//wUvMergrThd: unvoice thresh
	float glb_mean;			//wGlbMean: Mean for Gauss noise producing
	float glb_var;			//wGlbVar: Variance for Gauss noise producing
	float noise_var_ratio;
}wtk_splitf0_cfg_t;


struct wtk_fpost_cfg
{
	wtk_splitf0_cfg_t ctone;
	wtk_splitf0_cfg_t wtone;
};

//-------------- fpost cfg ----------------------------------
int wtk_fpost_cfg_init(wtk_fpost_cfg_t *cfg);
int wtk_fpost_cfg_clean(wtk_fpost_cfg_t *cfg);
int wtk_fpost_cfg_update_local(wtk_fpost_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_fpost_cfg_update(wtk_fpost_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
