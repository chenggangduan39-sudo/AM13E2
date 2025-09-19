#ifndef WTK_VAD_GMMVAD_WTK_GMMVAD_CFG
#define WTK_VAD_GMMVAD_WTK_GMMVAD_CFG
#include "wtk/core/cfg/wtk_local_cfg.h" 
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_gmmvad_cfg wtk_gmmvad_cfg_t;

enum { kNumChannels = 6 };  // Number of frequency bands (named channels).
enum { kNumGaussians = 2 };  // Number of Gaussians per channel in the GMM.
enum { kTableSize = kNumChannels * kNumGaussians };
enum { kMinEnergy = 10 };  // Minimum energy required to trigger audio signal.


struct wtk_gmmvad_cfg
{
	int frame_size;
	int mode;
};

int wtk_gmmvad_cfg_init(wtk_gmmvad_cfg_t *cfg);
int wtk_gmmvad_cfg_clean(wtk_gmmvad_cfg_t *cfg);
int wtk_gmmvad_cfg_update_local(wtk_gmmvad_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_gmmvad_cfg_update(wtk_gmmvad_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
