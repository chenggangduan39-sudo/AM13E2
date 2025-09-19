#ifndef WTK_VITE_PITCH_WTK_PITCH_CFG
#define WTK_VITE_PITCH_WTK_PITCH_CFG
#include "wtk/core/cfg/wtk_local_cfg.h" 
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"

//#include "wtk/vite/f0/wtk_f0.h"
#include <math.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_pitch_cfg wtk_pitch_cfg_t;
struct wtk_pitch_cfg
{
	wtk_main_cfg_t *main_cfg;
	wtk_mbin_cfg_t *mbin_cfg;
	wtk_rbin2_t *rbin;
	wtk_cfg_file_t *cfile;
	int sample_rate;
	int fft_frame_size;	//1024 pow 2
	int over_sampling;	//4 - 32
	int max_v;
	int step_size;
	int latency;
	float thresh;
	float scale;
	float expct;
};

int wtk_pitch_cfg_init(wtk_pitch_cfg_t *cfg);
int wtk_pitch_cfg_clean(wtk_pitch_cfg_t *cfg);
int wtk_pitch_cfg_update_local(wtk_pitch_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_pitch_cfg_update(wtk_pitch_cfg_t *cfg);
int wtk_pitch_cfg_update2(wtk_pitch_cfg_t *cfg,wtk_source_loader_t  *sl);

wtk_pitch_cfg_t* wtk_pitch_cfg_new(char *fn);
void wtk_pitch_cfg_delete(wtk_pitch_cfg_t *cfg);

wtk_pitch_cfg_t* wtk_pitch_cfg_new_bin(char *bin_fn);
void wtk_pitch_cfg_delete_bin(wtk_pitch_cfg_t *cfg);

#ifdef __cplusplus
};
#endif
#endif
