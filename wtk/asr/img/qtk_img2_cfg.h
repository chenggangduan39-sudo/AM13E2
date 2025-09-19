#ifndef QTK_IMG2_REC_CFG_H_
#define QTK_IMG2_REC_CFG_H_
#include "wtk/asr/fextra/kparm/wtk_kparm.h"
#include "wtk/asr/fextra/nnet3/qtk_nnet3.h"
#include "wtk/core/cfg/wtk_local_cfg.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"
#ifdef __cplusplus
extern "C"
{
#endif
    typedef struct qtk_img2_cfg qtk_img2_cfg_t;
    typedef struct qtk_img2_thresh_cfg qtk_img2_thresh_cfg_t;

    struct qtk_img2_thresh_cfg
    {
        float av_prob0;
        float maxx0;
        float avx0;
        float max_prob0;
        int speech_dur0;
        float av_prob1;
        float maxx1;
        float avx1;
        float max_prob1;
        int speech_dur1;
    };

    struct qtk_img2_cfg {
        union {
            wtk_main_cfg_t *main_cfg;
            wtk_mbin_cfg_t *bin_cfg;
        } cfg;
        wtk_kparm_cfg_t kparm;
        qtk_nnet3_cfg_t nnet3;
        wtk_rbin2_t *rbin;
        wtk_cfg_file_t *cfile;
        int channels;
        int noise_id;
        int min_noise_dur;
        float min_vad_thresh;
        float avg_vad_thresh;
        float max_vad_thresh; // max_vad_thresh
        float m_thresh;       // max_thresh
        float av_thresh;      // avg_thresh
        // Used in img2_cal before callback.
        qtk_img2_thresh_cfg_t *thresh;
        qtk_img2_thresh_cfg_t *thresh_echo;
    };

        int qtk_img2_cfg_init(qtk_img2_cfg_t *cfg);
	int qtk_img2_cfg_clean(qtk_img2_cfg_t *cfg);
	int qtk_img2_cfg_update_local(qtk_img2_cfg_t *cfg, wtk_local_cfg_t *lc);
	int qtk_img2_cfg_update(qtk_img2_cfg_t *cfg);
	int qtk_img2_cfg_update2(qtk_img2_cfg_t *cfg, wtk_source_loader_t *sl);
	qtk_img2_cfg_t *qtk_img2_cfg_new_bin(char *fn);
	void qtk_img2_cfg_delete_bin(qtk_img2_cfg_t *cfg);
	qtk_img2_cfg_t *qtk_img2_cfg_new_bin3(char *bin_fn, unsigned int seek_pos);
    void qtk_img2_cfg_delete_bin2(qtk_img2_cfg_t *cfg);
    qtk_img2_thresh_cfg_t *qtk_img2_thresh_cfg_init(void);
    int qtk_img2_thresh_cfg_update_local(qtk_img2_thresh_cfg_t *cfg, wtk_local_cfg_t *lc);

#ifdef __cplusplus
};
#endif
#endif
