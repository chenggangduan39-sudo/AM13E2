#ifndef QTK_DLG_AUDIO_UPLOAD_QTK_UPLOADS
#define QTK_DLG_AUDIO_UPLOAD_QTK_UPLOADS

#include "qtk_uploads_cfg.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_uploads qtk_uploads_t;
struct qtk_uploads
{
	qtk_uploads_cfg_t *cfg;
};

qtk_uploads_t* qtk_uploads_new(qtk_uploads_cfg_t *cfg);
void qtk_uploads_delete(qtk_uploads_t *ups);

int qtk_uploads_proc(qtk_uploads_t *ups);

#ifdef __cplusplus
};
#endif
#endif
