#ifndef QTK_DLG_AUDIO_UPLOAD_QTK_UPLOADC_CFG
#define QTK_DLG_AUDIO_UPLOAD_QTK_UPLOADC_CFG

#include "wtk/core/cfg/wtk_local_cfg.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_uploadc_cfg qtk_uploadc_cfg_t;
struct qtk_uploadc_cfg
{
	wtk_string_t host;
	wtk_string_t port;
	wtk_strbuf_t *host_buf;
	wtk_strbuf_t *port_buf;
	int timeout;
	unsigned int send_string:1;
};

int qtk_uploadc_cfg_init(qtk_uploadc_cfg_t *cfg);
int qtk_uploadc_cfg_clean(qtk_uploadc_cfg_t *cfg);
int qtk_uploadc_cfg_update_local(qtk_uploadc_cfg_t *cfg,wtk_local_cfg_t *lc);
int qtk_uploadc_cfg_update(qtk_uploadc_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
