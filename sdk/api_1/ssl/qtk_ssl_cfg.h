#ifndef __QTK_SSL_CFG_H__
#define __QTK_SSL_CFG_H__
#include "wtk/bfio/ssl/wtk_ssl.h"
#include "wtk/bfio/ssl/wtk_gainnet_ssl_cfg.h"
#include "wtk/core/cfg/wtk_local_cfg.h"
#ifdef __cplusplus
extern "C"
{
#endif

typedef struct qtk_ssl_cfg{
	wtk_string_t cfg_fn;
	wtk_ssl_cfg_t *ssl_cfg;
	wtk_gainnet_ssl_cfg_t *gainnet_ssl_cfg;
	wtk_main_cfg_t *main_cfg;
	wtk_rbin2_t *rbin;
	wtk_cfg_file_t *cfile;
	int *skip_channels;
	float mic_shift;
	float echo_shift;
	int nskip;
	int mics;
	int continue_time;
	int energy_sum;
	int max_extp;
	int online_tms;
	int online_frame_step;
	int theta_range;
	int theta_step;
	int specsum_fs;
	int specsum_fe;
	int use_maskssl;
	int use_maskssl2;
	int use_echoenable;

	unsigned int use_ssl:1;
	unsigned int use_gainnet_ssl:1;
	unsigned int use_bin:1;
	unsigned int use_manual:1;
}qtk_ssl_cfg_t;

int qtk_ssl_cfg_init(qtk_ssl_cfg_t *cfg);
int qtk_ssl_cfg_clean(qtk_ssl_cfg_t *cfg);
int qtk_ssl_cfg_update_local(qtk_ssl_cfg_t  *cfg, wtk_local_cfg_t *main);
int qtk_ssl_cfg_update(qtk_ssl_cfg_t *cfg);
int qtk_ssl_cfg_update2(qtk_ssl_cfg_t *cfg, wtk_source_loader_t *sl);

qtk_ssl_cfg_t *qtk_ssl_cfg_new(char *fn);
void qtk_ssl_cfg_delete(qtk_ssl_cfg_t *cfg);
qtk_ssl_cfg_t *qtk_ssl_cfg_new_bin(char *bin_fn);
void qtk_ssl_cfg_delete_bin(qtk_ssl_cfg_t *cfg);
#ifdef __cplusplus
};
#endif



#endif