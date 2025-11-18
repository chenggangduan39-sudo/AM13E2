#ifndef WTK_BFIO_QTK_SWEETSPOT
#define WTK_BFIO_QTK_SWEETSPOT
#include "qtk_sweetspot_cfg.h"
#include "wtk/bfio/agc/qtk_gain_controller.h"

#ifdef __cplusplus
extern "C" {
#endif
typedef struct qtk_sweetspot qtk_sweetspot_t;
typedef void (*qtk_sweetspot_notify_f)(void *upval, short **out, int len);

typedef struct qtk_sweetspot_gc qtk_sweetspot_gc_t;

struct qtk_sweetspot_gc {
    int truncation_count;
    int truncation_thre2Adjust_gain;
    float mu;
    
    int gidx;
    float *gain_set;
    float gain;

    int cold_delay;
    int cold_delay_count_down;
    float *gidx_result_in_truncation;

    int num_gain;
};

struct qtk_sweetspot
{
	qtk_sweetspot_cfg_t *cfg;

    float dr;
    float dl;
    float delay;
    float delay_left;
    float delay_right;
    int ndelay_left;
    int ndelay_right;

    float vol_left;
    float vol_right;

    float gain_factor;

    wtk_strbuf_t *left;
    wtk_strbuf_t *right;

    float *left_buff;
    float *right_buff;
    float *left_prev_half_win;
    float *right_prev_half_win;
    float *left_frame;
    int left_frame_len;
    float *right_frame;
    int right_frame_len;

    float *left_frame_delayed;
    float *right_frame_delayed;
    int fs;

    short **output;
    void *upval;
    qtk_sweetspot_notify_f notify;

    qtk_ahs_limiter_t left_limitor;
    qtk_ahs_limiter_t right_limitor;
    qtk_sweetspot_gc_t *gc;
};

qtk_sweetspot_t *qtk_sweetspot_new(qtk_sweetspot_cfg_t *cfg);
void qtk_sweetspot_delete(qtk_sweetspot_t *sspot);
void qtk_sweetspot_reset(qtk_sweetspot_t *sspot);
void qtk_sweetspot_update(qtk_sweetspot_t *sspot, float x, float y);
void qtk_sweetspot_feed(qtk_sweetspot_t *sspot, short **data,int len,int is_end);
void qtk_sweetspot_set_notify(qtk_sweetspot_t *sspot, void *upval, qtk_sweetspot_notify_f notify);
void qtk_sweetspot_set(qtk_sweetspot_t *sspot, float delay, float gain);


qtk_sweetspot_gc_t *qtk_sweetspot_gc_new();
void qtk_sweetspot_gc_delete(qtk_sweetspot_gc_t *gc);
void qtk_sweetspot_gc_reset(qtk_sweetspot_gc_t *gc);
void qtk_sweetspot_gc_run(qtk_sweetspot_gc_t *gc, float *frm, int len);
void qtk_sweetspot_gc_gain(qtk_sweetspot_gc_t *gc, float *frm, int len);
#ifdef __cplusplus
};
#endif
#endif