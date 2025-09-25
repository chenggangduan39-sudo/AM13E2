#ifndef QTK_DLG_AUDIO_UPLOAD_QTK_UPLOADS_CFG
#define QTK_DLG_AUDIO_UPLOAD_QTK_UPLOADS_CFG

#include "wtk/core/cfg/wtk_local_cfg.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_uploads_cfg qtk_uploads_cfg_t;
struct qtk_uploads_cfg
{
	wtk_string_t port;
	wtk_string_t dn;
};

int qtk_uploads_cfg_init(qtk_uploads_cfg_t *cfg);
int qtk_uploads_cfg_clean(qtk_uploads_cfg_t *cfg);
int qtk_uploads_cfg_update_local(qtk_uploads_cfg_t *cfg,wtk_local_cfg_t *lc);
int qtk_uploads_cfg_update(qtk_uploads_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
