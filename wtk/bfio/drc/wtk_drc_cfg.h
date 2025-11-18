#ifndef WTK_BFIO_DRC_CFG
#define WTK_BFIO_DRC_CFG
#include "wtk/core/cfg/wtk_cfg_file.h"
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_drc_cfg wtk_drc_cfg_t;
struct wtk_drc_cfg
{
    wtk_main_cfg_t *main_cfg;
    wtk_mbin_cfg_t *mbin_cfg;

    int rate;        // input sample rate (samples per second)
	float pregain;   // dB, amount to boost the signal before applying compression [0 to 100]
	float threshold; // dB, level where compression kicks in [-100 to 0]
	float knee;      // dB, width of the knee [0 to 40]
	float ratio;     // unitless, amount to inversely scale the output when applying comp [1 to 20]
	float attack;    // seconds, length of the attack phase [0 to 1]
	float release;   // seconds, length of the release phase [0 to 1]

    float predelay;     // seconds, length of the predelay buffer [0 to 1]
	float releasezone1; // release zones should be increasing between 0 and 1, and are a fraction
	float releasezone2; //  of the release time depending on the input dB -- these parameters define
	float releasezone3; //  the adaptive release curve, which is discussed in further detail in the
	float releasezone4; //  demo: adaptive-release-curve.html
	float postgain;     // dB, amount of gain to apply after compression [0 to 100]
	float wet;          // amount to apply the effect [0 completely dry to 1 completely wet]

    int size; //number of samples per channel when feeding
    int numchannels; //number of channels in the input signal
};

int wtk_drc_cfg_init(wtk_drc_cfg_t *cfg);
int wtk_drc_cfg_clean(wtk_drc_cfg_t *cfg);
int wtk_drc_cfg_update_local(wtk_drc_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_drc_cfg_update(wtk_drc_cfg_t *cfg);
int wtk_drc_cfg_update2(wtk_drc_cfg_t *cfg,wtk_source_loader_t *sl);

wtk_drc_cfg_t* wtk_drc_cfg_new(char *fn);
void wtk_drc_cfg_delete(wtk_drc_cfg_t *cfg);
wtk_drc_cfg_t* wtk_drc_cfg_new_bin(char *fn);
void wtk_drc_cfg_delete_bin(wtk_drc_cfg_t *cfg);

wtk_drc_cfg_t* wtk_drc_cfg_new2(char *fn, char *fn2);
void wtk_drc_cfg_delete2(wtk_drc_cfg_t *cfg);
wtk_drc_cfg_t* wtk_drc_cfg_new_bin2(char *fn, char *fn2);
void wtk_drc_cfg_delete_bin2(wtk_drc_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
