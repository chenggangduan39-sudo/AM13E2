#ifndef WTK_SYN_HMM_CFG
#define WTK_SYN_HMM_CFG
#include "wtk/core/cfg/wtk_local_cfg.h" 
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_cosynthesis_hmm_cfg wtk_cosynthesis_hmm_cfg_t;
struct wtk_cosynthesis_hmm_cfg
{
	wtk_heap_t *heap;
    char *bin_fn;
	char *dur_fn;
	char *lf0_fn;
	char *mcp_fn;
	char *bap_fn;
	char *lf0_gv_fn;
	char *mcp_gv_fn;
	char *bap_gv_fn;
    char *conca_lf0_fn;
    char *conca_mcp_fn;

	int rate;		/* sampring rate                              */
	int fperiod;	/* frame period (point)                       */
    float rho;     /* variable for speaking rate control         */
    float alpha;   /* variable for frequency warping parameter   */
    float f0_mean; /* variable for f0 control                    */
    float f0_std;  /* variable for f0 control                    */
    float beta;    /* variable for postfiltering                 */
    float uv;      /* variable for U/V threshold                 */
    float length;  /* total number of frame for generated speech */
    int fftlen;
    float sigp;    /* mapping parameter for mixed excitation */
	float gamma;   /* variable for control the cepstral scale in MGC or MGC-LSP */
	char *bapcfgfn; /* bap configure file */
    unsigned use_algnst:1;  /* use state level alignment for duration     */
    unsigned use_algnph:1;  /* use phoneme level alignment for duration   */
    unsigned use_rnd_flag:1;/* use pulse train with phase manipulation   */
    unsigned use_bapcfg:1;
    unsigned load_all:1;
    wtk_array_t *dynamic_spec;
    wtk_array_t *dynamic_pitch;
};

int wtk_cosynthesis_hmm_cfg_init(wtk_cosynthesis_hmm_cfg_t *cfg);
int wtk_cosynthesis_hmm_cfg_clean(wtk_cosynthesis_hmm_cfg_t *cfg);
int wtk_cosynthesis_hmm_cfg_update_local(wtk_cosynthesis_hmm_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_cosynthesis_hmm_cfg_update(wtk_cosynthesis_hmm_cfg_t *cfg);
int wtk_cosynthesis_hmm_cfg_update2(wtk_cosynthesis_hmm_cfg_t *cfg,wtk_source_loader_t *sl);
#ifdef __cplusplus
};
#endif
#endif
