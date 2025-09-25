#ifndef QTK_KCMN_CFG_H_
#define QTK_KCMN_CFG_H_
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/core/math/wtk_vector.h"
#include "wtk/core/math/wtk_math.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_kcmn_cfg qtk_kcmn_cfg_t;
struct qtk_kcmn_cfg
{
	char *cmn_fn;			//!< cepstral mean normalize file name.
	wtk_vector_t *cmn_def;	//!< default cmn vector.
	int cmn_window;
	int speaker_frames;
	int global_frames;
	int modulus;
	int ring_buffer_size;
};

int qtk_kcmn_cfg_init(qtk_kcmn_cfg_t *cfg);
int qtk_kcmn_cfg_clean(qtk_kcmn_cfg_t *cfg);
int qtk_kcmn_cfg_update_local(qtk_kcmn_cfg_t *cfg,wtk_local_cfg_t *lc);
int qtk_kcmn_cfg_update(qtk_kcmn_cfg_t *cfg,wtk_source_loader_t *sl);
#ifdef __cplusplus
};
#endif
#endif
