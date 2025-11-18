#ifndef WTK_BFIO_SSL_WTK_SSL_CFG
#define WTK_BFIO_SSL_WTK_SSL_CFG
#include "wtk/core/fft/wtk_stft2.h"
#include "wtk/bfio/aspec/wtk_aspec.h" 
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_ssl_cfg wtk_ssl_cfg_t;
struct wtk_ssl_cfg
{
    wtk_main_cfg_t *main_cfg;
    wtk_mbin_cfg_t *mbin_cfg;
    
    wtk_stft2_cfg_t stft2;
    wtk_aspec_cfg_t aspec;

    int theta_step;
    int phi_step;

    int max_extp;
    int min_thetasub;

    float specsum_fs;
    float specsum_fe;
    int specsum_ns;
    int specsum_ne;
    float specsum_thresh;

    int lf;
    int lt;

    int max_theta;
    int max_phi;

    float notify_time;
	float min_energy;
	int notify_len;

    int rate;

    unsigned use_stft2:1;
    unsigned use_line:1;
};

int wtk_ssl_cfg_init(wtk_ssl_cfg_t *cfg);
int wtk_ssl_cfg_clean(wtk_ssl_cfg_t *cfg);
int wtk_ssl_cfg_update_local(wtk_ssl_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_ssl_cfg_update(wtk_ssl_cfg_t *cfg);
int wtk_ssl_cfg_update2(wtk_ssl_cfg_t *cfg,wtk_source_loader_t *sl);

wtk_ssl_cfg_t* wtk_ssl_cfg_new(char *cfg_fn);
void wtk_ssl_cfg_delete(wtk_ssl_cfg_t *cfg);
wtk_ssl_cfg_t* wtk_ssl_cfg_new_bin(char *bin_fn);
void wtk_ssl_cfg_delete_bin(wtk_ssl_cfg_t *cfg);

#ifdef __cplusplus
};
#endif
#endif