#ifndef WTK_VITE_F0_WTK_F0_CFG_H_
#define WTK_VITE_F0_WTK_F0_CFG_H_
#include "wtk/core/wtk_type.h"
#include "wtk/core/cfg/wtk_cfg_file.h"
#include "wtk/asr/fextra/f0/post/wtk_fpost_cfg.h"
#include "wtk/asr/fextra/f0/avg/wtk_favg_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_f0_cfg wtk_f0_cfg_t;

struct wtk_f0_cfg
{
	wtk_fpost_cfg_t post;
	wtk_favg_cfg_t avg;
	float frame_dur;	//in seconds
	unsigned use_post:1;
	unsigned use_avg:1;
};

int wtk_f0_cfg_init(wtk_f0_cfg_t *cfg);
int wtk_f0_cfg_clean(wtk_f0_cfg_t *cfg);
int wtk_f0_cfg_update_local(wtk_f0_cfg_t *cfg,wtk_local_cfg_t* cf);
int wtk_f0_cfg_update(wtk_f0_cfg_t *cfg);
void wtk_f0_cfg_print(wtk_f0_cfg_t *cfg);
int wtk_f0_cfg_update_set_example(wtk_f0_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
