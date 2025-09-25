#ifndef WTK_BFIO_SSL_WTK_GAINNET_SSL_CFG
#define WTK_BFIO_SSL_WTK_GAINNET_SSL_CFG
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/bfio/masknet/wtk_gainnet7.h"
#include "wtk/bfio/masknet/wtk_gainnet2.h"
#include "wtk/bfio/masknet/wtk_gainnet.h"
#include "wtk/bfio/afilter/wtk_nlms.h"
#include "wtk/bfio/afilter/wtk_rls.h"
#include "wtk/bfio/qform/wtk_qmmse.h"
#include "wtk/bfio/ssl/wtk_ssl2.h"
#include "wtk/bfio/ssl/wtk_maskssl.h"
#include "wtk/bfio/ssl/wtk_maskssl2.h"
#include "wtk/bfio/maskdenoise/wtk_bankfeat.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_gainnet_ssl_cfg wtk_gainnet_ssl_cfg_t;
struct wtk_gainnet_ssl_cfg
{
    wtk_main_cfg_t *main_cfg;
    wtk_mbin_cfg_t *mbin_cfg;

	int wins;
    int rate;
    
	wtk_rls_cfg_t echo_rls;

    wtk_qmmse_cfg_t qmmse;

    float spenr_thresh;
    int spenr_cnt;

    float micenr_thresh;
    int micenr_cnt;

    wtk_bankfeat_cfg_t bankfeat;

    char *mdl_fn;
	wtk_gainnet7_cfg_t *gainnet7;
    char *aecmdl_fn;
    wtk_gainnet2_cfg_t *gainnet2;

    wtk_maskssl_cfg_t maskssl;
    wtk_maskssl2_cfg_t maskssl2;

	int channel;
	int *mic_channel;
	int nmicchannel;
	int *sp_channel;
	int nspchannel;

    float theta;
	float phi;

    int pframe_fs;
    int pframe_fe;
    float pframe_alpha;

    float ralpha;

    unsigned use_rbin_res:1;
    unsigned use_erlssingle:1;
    unsigned use_firstds:1;
    unsigned use_maskssl:1;
    unsigned use_maskssl2:1;
    unsigned use_qmmse:1;
};

int wtk_gainnet_ssl_cfg_init(wtk_gainnet_ssl_cfg_t *cfg);
int wtk_gainnet_ssl_cfg_clean(wtk_gainnet_ssl_cfg_t *cfg);
int wtk_gainnet_ssl_cfg_update_local(wtk_gainnet_ssl_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_gainnet_ssl_cfg_update(wtk_gainnet_ssl_cfg_t *cfg);
int wtk_gainnet_ssl_cfg_update2(wtk_gainnet_ssl_cfg_t *cfg,wtk_source_loader_t *sl);

wtk_gainnet_ssl_cfg_t* wtk_gainnet_ssl_cfg_new(char *fn);
void wtk_gainnet_ssl_cfg_delete(wtk_gainnet_ssl_cfg_t *cfg);
wtk_gainnet_ssl_cfg_t* wtk_gainnet_ssl_cfg_new_bin(char *fn);
void wtk_gainnet_ssl_cfg_delete_bin(wtk_gainnet_ssl_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
