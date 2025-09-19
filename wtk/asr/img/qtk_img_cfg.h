#ifndef QTK_IMG_REC_CFG_H_
#define QTK_IMG_REC_CFG_H_
#include "qtk/nnrt/qtk_nnrt_cfg.h"
#include "wtk/asr/fextra/kparm/wtk_kxparm.h"
#include "wtk/asr/wfst/net/wtk_fst_sym.h"
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"

#ifdef __cplusplus
extern "C"
{
#endif
typedef struct qtk_img_thresh_cfg qtk_img_thresh_cfg_t;
typedef struct qtk_img_cfg qtk_img_cfg_t;

struct qtk_img_thresh_cfg
{
	float av_prob0;
	float maxx0;
	float avx0;
	float max_prob0;
	float av_prob1;
	float maxx1;
	float avx1;
	float max_prob1;
	int speech_dur;
};

struct qtk_img_cfg
{
	union
	{
		wtk_main_cfg_t *main_cfg;
		wtk_mbin_cfg_t *bin_cfg;
	} cfg;
	wtk_kxparm_cfg_t kxparm;
        qtk_nnrt_cfg_t nnrt;
        int chunk;
        int subsample;
	int pad_len;
        int right_context;
        int left_context;

        wtk_fst_sym_t *sym;
	wtk_rbin2_t *rbin;
	wtk_cfg_file_t *cfile;
        int use_waitwake;
        char *sym_fn;
        int noise_id;
        int start_off;
        int end_off;
        int min_noise_dur;
        int notify_bias;
        float min_vad_thresh;
        float avg_vad_thresh;
	float max_vad_thresh;       //max_vad_thresh
	float m_thresh;             //max_thresh
	float av_thresh;            //avg_thresh
        float f_dur;
        float blank_penalty;
        qtk_img_thresh_cfg_t thresh;
	qtk_img_thresh_cfg_t thresh_echo;
        unsigned use_nnrt : 1;
        // Used in img_cal before callback.
	
};

int qtk_img_cfg_init(qtk_img_cfg_t *cfg);
int qtk_img_cfg_clean(qtk_img_cfg_t *cfg);
int qtk_img_cfg_update_local(qtk_img_cfg_t *cfg, wtk_local_cfg_t *lc);
int qtk_img_cfg_update(qtk_img_cfg_t *cfg);
int qtk_img_cfg_update2(qtk_img_cfg_t *cfg, wtk_source_loader_t *sl);
qtk_img_cfg_t *qtk_img_cfg_new_bin(char *fn);
qtk_img_cfg_t *qtk_img_cfg_new_binb(char *data, int size);
void qtk_img_cfg_delete_bin(qtk_img_cfg_t *cfg);
qtk_img_cfg_t *qtk_img_cfg_new_bin3(char *bin_fn, unsigned int seek_pos);
void qtk_img_cfg_delete_bin2(qtk_img_cfg_t *cfg);
int qtk_img_thresh_cfg_init(qtk_img_thresh_cfg_t *img_thresh);
int qtk_img_thresh_cfg_update_local(qtk_img_thresh_cfg_t *cfg, wtk_local_cfg_t *lc);
#ifdef __cplusplus
};
#endif
#endif
