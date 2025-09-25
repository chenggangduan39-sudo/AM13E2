#ifndef QTK_NNWRAP_CFG_H_
#define QTK_NNWRAP_CFG_H_
#include "wtk/asr/fextra/kparm/wtk_kxparm.h"
#include "wtk/core/cfg/wtk_cfg_file.h"
#include "wtk/core/rbin/wtk_rbin2.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/asr/vad/kvad/wtk_kvad.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_nnwrap_cfg qtk_nnwrap_cfg_t;

struct qtk_nnwrap_cfg
{
	void *hook;
	wtk_cfg_file_t *cfile;
	wtk_rbin2_t *rbin;
    wtk_kxparm_cfg_t kxparm;
    wtk_kvad_cfg_t vad;
    char *encoder_fn;
	char *gender_fn;
	char *age_fn;
	char *pool_fn;
	wtk_source_loader_t sl;
    unsigned int use_vad:1;
};

int qtk_nnwrap_cfg_init(qtk_nnwrap_cfg_t *cfg);
int qtk_nnwrap_cfg_clean(qtk_nnwrap_cfg_t *cfg);
int qtk_nnwrap_cfg_update_local(qtk_nnwrap_cfg_t *cfg,wtk_local_cfg_t *lc);
int qtk_nnwrap_cfg_update(qtk_nnwrap_cfg_t *cfg);

/**
 * @brief used for bin loader;
 */
int qtk_nnwrap_cfg_update2(qtk_nnwrap_cfg_t *cfg,wtk_source_loader_t *sl);

qtk_nnwrap_cfg_t* qtk_nnwrap_cfg_new_bin(char *bin_fn,char *cfg_fn);
qtk_nnwrap_cfg_t* qtk_nnwrap_cfg_new_bin2(char *bin_fn);
qtk_nnwrap_cfg_t* qtk_nnwrap_cfg_new_bin3(char *bin_fn,unsigned int seek_pos);
int qtk_nnwrap_cfg_delete_bin(qtk_nnwrap_cfg_t *cfg);
int qtk_nnwrap_cfg_delete_bin2(qtk_nnwrap_cfg_t *cfg);
qtk_nnwrap_cfg_t* qtk_nnwrap_cfg_new(char *cfg_fn);
void qtk_nnwrap_cfg_delete(qtk_nnwrap_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
