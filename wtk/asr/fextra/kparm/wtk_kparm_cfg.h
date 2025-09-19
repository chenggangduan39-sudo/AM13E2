#ifndef WTK_ASR_PARM_WTK_KPARM_CFG
#define WTK_ASR_PARM_WTK_KPARM_CFG
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/asr/fextra/kcmn/qtk_kcmn_cfg.h"
#include "wtk/asr/fextra/ivector/qtk_ivector.h"
#include "wtk_cmvn.h"
#include "wtk_pcen.h"
#include "wtk_delta.h"
#include "wtk_lda.h"
#include "wtk/core/wtk_fixpoint.h"
#include "wtk/asr/fextra/kcmn/wtk_kcmn_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif
#ifndef M_2PI
#define M_2PI 6.283185307179586476925286766559005
#endif
typedef struct wtk_kparm_cfg wtk_kparm_cfg_t;


#ifdef USE_XY
#define HASENERGY  0100       /* _E log energy included */
#define HASNULLE   0200       /* _N absolute energy suppressed */
#define HASDELTA   0400       /* _D delta coef appended */
#define HASACCS   01000       /* _A acceleration coefs appended */
#define HASCOMPX  02000       /* _C is compressed */
#define HASZEROM  04000       /* _Z zero meaned */
#define HASCRCC  010000       /* _K has CRC check */
#define HASZEROC 020000       /* _0 0'th Cepstra included */
#define HASVQ    040000       /* _V has VQ index attached */
#define HASTHIRD 0100000       /* _T has Delta-Delta-Delta index attached */
#endif

typedef enum
{
	WTK_FBANK,
	WTK_MFCC,
	WTK_PLP,
}wtk_kbfkind_t;


typedef struct
{
	wtk_kbfkind_t bkind;
	unsigned has_energy:1;  /* _E log energy included */
//	unsigned has_d:1; /* _D delta coef appended */
//	unsigned has_a:1; /* _A acceleration coefs appended */
//	unsigned has_t:1; /* _T has Delta-Delta-Delta index attached */
	unsigned has_z:1; /* _Z zero meaned */
	unsigned has_0:1; /* _0 0'th Cepstra included */
}wtk_kfkind_t;

typedef struct
{
	int num_bins;
	float low_freq;
	float high_freq;
	float vtln_low;
	float vtln_high;
	unsigned use_htk:1;
	unsigned use_torch:1;
	unsigned debug_mel:1;
	unsigned htk_mode:1;
	unsigned use_normal:1;
}wtk_melbank_cfg_t;

struct wtk_kparm_cfg
{
	char *kind;
	wtk_kfkind_t kfind;
	wtk_melbank_cfg_t melbank;
	wtk_cmvn_cfg_t cmvn;
	wtk_pcen_cfg_t pcen;
	wtk_kcmn_cfg_t kcmvn;
	char *cmvn_stats_fn;
	wtk_vector_t *cmvn_stats;
	qtk_kcmn_cfg_t kcmn;
	qtk_ivector_cfg_t ivector;
	wtk_delta_cfg_t delta;
	wtk_lda_cfg_t lda;
	int rate;
	float frame_size_ms;
	float frame_step_ms;
	int frame_size;
	int frame_step;
	int NUMCEPS;
	float CEPLIFTER;
	//----- window -----------
	char *window_type;
	float blackman_coeff;
	float preemph_coeff;
	float *window;
	float dither;
	int cache;
	int vec_size;
	int vec_size2;
	int idle_trigger_frame;
	float energy_floor;
	float log_energy_floor;

	wtk_fix_t *fix_win;
	wtk_fix_t fix_preemph_coeff;
	unsigned use_pad:1;
	unsigned remove_dc:1;
	unsigned use_power:1;
	unsigned use_log_fbank:1;
	unsigned use_cmvn:1;
	unsigned use_pcen:1;
	unsigned use_kcmn:1;
	unsigned use_kcmvn:1;
	unsigned use_ivector:1;
	unsigned use_delta:1;
	unsigned use_lda:1;
	unsigned use_fixpoint:1;
	unsigned use_snip_edges:1;
	unsigned use_kind_notify:1;
	unsigned use_trick:1;
	unsigned use_k2_offline:1;
};
int wtk_kparm_cfg_bytes(wtk_kparm_cfg_t *cfg);
int wtk_kparm_cfg_init(wtk_kparm_cfg_t *cfg);
int wtk_kparm_cfg_clean(wtk_kparm_cfg_t *cfg);
int wtk_kparm_cfg_update_local(wtk_kparm_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_kparm_cfg_update(wtk_kparm_cfg_t *cfg);
int wtk_kparm_cfg_update2(wtk_kparm_cfg_t *cfg,wtk_source_loader_t *sl);

int wtk_kparm_cfg_feature_base_size(wtk_kparm_cfg_t *cfg);
int wtk_kparm_cfg_feature_size(wtk_kparm_cfg_t *cfg);
int wtk_kparm_cfg_feature_use_size(wtk_kparm_cfg_t *cfg);

int wtk_kparm_cfg_load_cmvn_stats(wtk_kparm_cfg_t *cfg,wtk_source_t *s);
#ifdef __cplusplus
};
#endif
#endif
