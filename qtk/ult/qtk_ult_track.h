#ifndef BEBB5AE2_5B63_47A8_B042_D49CF1DF0501
#define BEBB5AE2_5B63_47A8_B042_D49CF1DF0501

#include "qtk/mdl/qtk_sonicnet.h"
#include "qtk/ult/fmcw/qtk_ult_fmcw.h"
#include "qtk/ult/qtk_ult_msc2d.h"
#include "qtk/ult/qtk_ult_ofdm.h"
#include "qtk/ult/qtk_ult_perception.h"
#include "qtk/ult/qtk_ult_track_cfg.h"
#include "qtk/ult/qtk_ult_track_type.h"
#include "qtk/ult/qtk_ultm2.h"
#include "wtk/core/wtk_complex.h"
#include "wtk/core/wtk_robin.h"
#include "wtk/core/wtk_vpool.h"

typedef struct qtk_ult_track qtk_ult_track_t;

typedef int (*qtk_ult_track_notifier_t)(void *upval, int nframe, int nobj,
                                        qtk_ult_track_result_t *res);

struct qtk_ult_track {
    qtk_ult_track_cfg_t *cfg;
    qtk_ult_ofdm_t *ofdm;
    qtk_ult_fmcw_t *fmcw;
    qtk_ult_perception_t *perception;
    qtk_ult_perception_result_t perception_res;
    qtk_sonicnet_t *sonicnet;
    qtk_ultm2_t *ultm2;
    qtk_ult_msc2d_t *msc2d;
    qtk_ult_msc2d_t *y_msc2d;
    float *msc2d_prob;
    wtk_queue_t chan_resp;
    wtk_heap_t *heap;
    float *smoothed_range_prob;
    float *smoothed_cir_amp;
    wtk_robin_t *cir_ctx;
    wtk_complex_t *cir_ctx_data;
    wtk_strbuf_t *tmp_buf;
    wtk_strbuf_t *obj;
    wtk_vpool_t *tl_info;
    qtk_mot_sort_t sort;
    int nframe;
    float dis_unit;
    int n;
    uint32_t vad_pre_chunk_s;

    unsigned active : 1;
    unsigned sonicnet_with_angle : 1;

    qtk_ult_track_notifier_t notifier;
    void *upval;
};

qtk_ult_track_t *qtk_ult_track_new(qtk_ult_track_cfg_t *cfg);
int qtk_ult_track_feed(qtk_ult_track_t *s, short **wav, int len);
int qtk_ult_track_feed_end(qtk_ult_track_t *s);
void qtk_ult_track_delete(qtk_ult_track_t *s);
void qtk_ult_track_set_notifier(qtk_ult_track_t *s, void *upval,
                                qtk_ult_track_notifier_t notifier);

#endif /* BEBB5AE2_5B63_47A8_B042_D49CF1DF0501 */
