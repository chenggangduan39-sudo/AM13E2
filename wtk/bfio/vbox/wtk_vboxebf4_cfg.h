#ifndef WTK_BFIO_VBOX_WTK_VBOXBF4_CFG
#define WTK_BFIO_VBOX_WTK_VBOXBF4_CFG
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/bfio/masknet/wtk_gainnet5.h"
#include "wtk/bfio/masknet/wtk_gainnet6.h"
#include "wtk/bfio/masknet/wtk_gainnet.h"
#include "wtk/bfio/afilter/wtk_nlms.h"
#include "wtk/bfio/afilter/wtk_rls.h"
#include "wtk/bfio/qform/wtk_bf.h"
#include "wtk/bfio/qform/wtk_covm.h"
#include "wtk/bfio/qform/wtk_qmmse.h"
#include "wtk/bfio/eq/wtk_equalizer.h"
#include "wtk/bfio/ssl/wtk_ssl2.h"
#include "wtk/bfio/ssl/wtk_maskssl.h"
#include "wtk/bfio/ssl/wtk_maskssl2.h"
#include "wtk/bfio/maskdenoise/wtk_bankfeat.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_vboxebf4_cfg wtk_vboxebf4_cfg_t;
struct wtk_vboxebf4_cfg
{
    wtk_main_cfg_t *main_cfg;
    wtk_mbin_cfg_t *mbin_cfg;

	int wins;
    int rate;
    
    wtk_covm_cfg_t covm;
    wtk_covm_cfg_t echo_covm;
    float bfmu;
    float echo_bfmu;
	wtk_bf_cfg_t bf;
	wtk_rls_cfg_t echo_rls;

    wtk_qmmse_cfg_t qmmse;

    float spenr_thresh;
    int spenr_cnt;

    float micenr_thresh;
    int micenr_cnt;

    wtk_bankfeat_cfg_t bankfeat;

    char *mdl_fn;
	wtk_gainnet5_cfg_t *gainnet;
    wtk_gainnet6_cfg_t *gainnet6;

    wtk_ssl2_cfg_t ssl2;
    wtk_maskssl_cfg_t maskssl;
    wtk_maskssl2_cfg_t maskssl2;

    float agc_a;
    float agc_a2;
    float agc_b;

    float ralpha;
    int mchannel;
    int schannel;
	int channel;
	int *mic_channel;
	int nmicchannel;
	int *sp_channel;
	int nspchannel;
    int nbfchannel;

    float theta;
	float phi;

    float gbias;

    int pframe_fs;
    int pframe_fe;
    float pframe_thresh;
    float pframe_alpha;

    int clip_s;
    int clip_e;
    
	wtk_equalizer_cfg_t eq;

    float sym;

    int de_clip_s;
    int de_clip_e;
    float de_thresh;
    float de_alpha;

    unsigned use_rbin_res:1;
    unsigned use_epostsingle:1;
    unsigned use_fixtheta:1;
    unsigned use_qmmse:1;
    unsigned use_eq:1;
    unsigned use_ssl:1;
    unsigned use_maskssl:1;
    unsigned use_maskssl2:1;
    unsigned use_gainnet6:1;
    unsigned use_fftsbf:1;
    unsigned use_efftsbf:1;
    unsigned use_aecmmse:1;
    unsigned use_demmse:1;
    unsigned use_phaseo:1;
    unsigned use_echocovm:1;
    unsigned use_cnon:1;
    unsigned use_ssl_delay:1;
};

int wtk_vboxebf4_cfg_init(wtk_vboxebf4_cfg_t *cfg);
int wtk_vboxebf4_cfg_clean(wtk_vboxebf4_cfg_t *cfg);
int wtk_vboxebf4_cfg_update_local(wtk_vboxebf4_cfg_t *cfg,wtk_local_cfg_t *lc);
int wtk_vboxebf4_cfg_update(wtk_vboxebf4_cfg_t *cfg);
int wtk_vboxebf4_cfg_update2(wtk_vboxebf4_cfg_t *cfg,wtk_source_loader_t *sl);

wtk_vboxebf4_cfg_t* wtk_vboxebf4_cfg_new(char *fn);
void wtk_vboxebf4_cfg_delete(wtk_vboxebf4_cfg_t *cfg);
wtk_vboxebf4_cfg_t* wtk_vboxebf4_cfg_new_bin(char *fn);
void wtk_vboxebf4_cfg_delete_bin(wtk_vboxebf4_cfg_t *cfg);

wtk_vboxebf4_cfg_t* wtk_vboxebf4_cfg_new2(char *fn, char *fn2);
void wtk_vboxebf4_cfg_delete2(wtk_vboxebf4_cfg_t *cfg);
wtk_vboxebf4_cfg_t* wtk_vboxebf4_cfg_new_bin2(char *fn, char *fn2);
void wtk_vboxebf4_cfg_delete_bin2(wtk_vboxebf4_cfg_t *cfg);
#ifdef __cplusplus
};
#endif
#endif
