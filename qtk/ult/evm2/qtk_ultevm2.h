#ifndef AB4D3F15_847F_3E38_6332_F2EAD2A5A8E5
#define AB4D3F15_847F_3E38_6332_F2EAD2A5A8E5

#include "qtk/ult/evm2/qtk_ultevm2_cfg.h"
#include "wtk/core/fft/wtk_rfft.h"
#include "wtk/core/fft/wtk_cfft.h"
#include "wtk/signal/wtk_conv2.h"

typedef struct qtk_ultevm2 qtk_ultevm2_t;

struct qtk_ultevm2 {
    qtk_ultevm2_cfg_t *cfg;
    wtk_rfft_t *fc_est_fft;
    float *fc_est_F;
    wtk_strbuf_t *data;
    wtk_conv2_t *bandpass;

    wtk_conv2_t *sig_re_rcos;
    wtk_conv2_t *sig_im_rcos;
    wtk_conv2_t *sig_side_re_rcos;
    wtk_conv2_t *sig_side_im_rcos;

    wtk_conv2_t *sig_re_median;
    wtk_conv2_t *sig_im_median;
    wtk_conv2_t *sig_side_re_median;
    wtk_conv2_t *sig_side_im_median;

    wtk_strbuf_t *sig_re_data;
    wtk_strbuf_t *sig_im_data;
    wtk_strbuf_t *sig_side_re_data;
    wtk_strbuf_t *sig_side_im_data;

    short *sig;
    float *carrier_iphase;
    float *carrier_qphase;
    float *carrier_iphase_side;
    float *carrier_qphase_side;

    int sig_period;
    int sig_side_period;

    wtk_strbuf_t *sig_frame;
    wtk_strbuf_t *sig_side_frame;
    wtk_cfft_t *frame_fft;

    int sig_Q_idx;
    int sig_side_Q_idx;

    float *tri_filter_kernel;
    int sample_idx;
    float feat_max_smoothed;
    int frame_idx;
    int post_trap;

    unsigned active : 1;
};

qtk_ultevm2_t *qtk_ultevm2_new(qtk_ultevm2_cfg_t *cfg);
void qtk_ultevm2_delete(qtk_ultevm2_t *u);
int qtk_ultevm2_feed(qtk_ultevm2_t *u, short *data, int len);
int qtk_ultevm2_feed_end(qtk_ultevm2_t *u);
int qtk_ultevm2_get_signal(qtk_ultevm2_t *u, short **dat);

#endif /* AB4D3F15_847F_3E38_6332_F2EAD2A5A8E5 */
