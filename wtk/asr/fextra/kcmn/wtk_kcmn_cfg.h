#ifndef WTK_KCMN_CFG_H_
#define WTK_KCMN_CFG_H_
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/core/math/wtk_vector.h"
#include "wtk/core/math/wtk_math.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_kcmn_cfg wtk_kcmn_cfg_t;
struct wtk_kcmn_cfg
{
	char *cmn_fn;			//!< cepstral mean normalize file name.
	wtk_vector_t *cmn_def;	//!< default cmn vector.
	int cmn_window;
	int speaker_frames;
	int global_frames;
	int modulus;
	int ring_buffer_size;
};

int wtk_kcmn_cfg_init(wtk_kcmn_cfg_t *cfg);
int wtk_kcmn_cfg_clean(wtk_kcmn_cfg_t *cfg);
int wtk_kcmn_cfg_update_local(wtk_kcmn_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_kcmn_cfg_update(wtk_kcmn_cfg_t *cfg);
int wtk_kcmn_cfg_update2(wtk_kcmn_cfg_t *cfg,wtk_source_loader_t *sl);

int wtk_kcmn_zmean_cfg_load_cmn2(wtk_kcmn_cfg_t *cfg,wtk_source_t *s);
#ifdef __cplusplus
};
#endif
#endif
