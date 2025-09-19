#include "qtk_audio_daemon_cfg.h"

int qtk_audio_daemon_cfg_init(qtk_audio_daemon_cfg_t *cfg)
{
	cfg->vendor_id = 0x0483;
	cfg->product_id = 0x5740;
	return 0;
}

int qtk_audio_daemon_cfg_clean(qtk_audio_daemon_cfg_t *cfg)
{
	return 0;
}

int qtk_audio_daemon_cfg_update_local(qtk_audio_daemon_cfg_t *cfg,wtk_local_cfg_t *lc)
{
	wtk_string_t *v;

	wtk_local_cfg_update_cfg_i(lc,cfg,vendor_id,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,product_id,v);
	return 0;
}

int qtk_audio_daemon_cfg_update(qtk_audio_daemon_cfg_t *cfg)
{
	return 0;
}
