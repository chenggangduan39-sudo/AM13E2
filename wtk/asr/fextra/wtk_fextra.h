#ifndef WTK_MATH_WTK_FEXTRA_H_
#define WTK_MATH_WTK_FEXTRA_H_
#include "wtk/core/math/wtk_vector_buffer.h"
#include "nnet3/qtk_nnet3.h"
#include "wtk_sigp.h"
#include "wtk/core/wtk_hoard.h"
#include "wtk/core/wtk_bit_heap.h"
#include "wtk_feat.h"
#include "wtk/core/wtk_queue.h"
#include "wtk/core/wtk_robin.h"
#include "wtk/asr/model/wtk_hmmset.h"
#include "cmn/wtk_cmn.h"
#include "wtk/os/wtk_lockhoard.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_fextra wtk_fextra_t;

/**
 * @brief wtk_extra_post_f want to do some post process after get features.
 * @param hook, application data
 * @param feature, if null means is end,want to flush robins.
 */
typedef void (*wtk_fextra_post_f)(void *inst,wtk_feat_t *feature);

/**
 * @brief wtk_extra_static_post_f want to do some process after get static features;
 */
typedef void (*wtk_fextra_static_post_f)(void *inst,wtk_feat_t *feature);

typedef void (*wtk_fextra_feature_notify_f)(void *ths,wtk_feat_t *f);

typedef void (*wtk_fextra_window_notify_f)(void *ths,wtk_feat_t *f);

//#define USE_extra_MT

struct wtk_fextra
{
	wtk_hoard_t feature_hoard;
	wtk_fextra_cfg_t* cfg;
	qtk_nnet3_t* nnet3;
	wtk_sigp_t sigp;
	wtk_vector_buffer_t *frame_buffer;
	wtk_vector_t *frame_vector;
	wtk_robin_t* robins[3];
	//wtk_queue_t feature_cache_q;			//used for cache parm feature for f0 feature if it is need;
	wtk_vector_t **v_tmp_array;		//vector temp array used for window;
	wtk_feat_t **f_tmp_array;	//feature tmp array;
	wtk_matrix_t *hlda_mat;
	int diffs;
	int	feature_pos[3];
	int win[3];
	int xform_rows;
	int xform_cols;
	int n_frame_index;
	int float_vector_idx;
	//================ call after static feature is processed ==============
	//post process must be rewrite later.
	wtk_fextra_static_post_f static_post_f;
	void *static_post_hook;
	//================ call after static and dynamic feature is processed ===
	wtk_fextra_post_f post_f;
	void *post_hook;
	//======================= post proc ==========
	union{
		wtk_cmn_t *zmean;
	}zpost;
	wtk_fnn_t *dnn;

	//-------------------- check output queue--------------
	wtk_queue_t *output_queue;	//wtk_feature_t queue;
	void *notify_ths;
	wtk_fextra_feature_notify_f notify;

	wtk_fextra_window_notify_f win_notify;
	void *win_ths;
};

wtk_fextra_t* wtk_fextra_new(wtk_fextra_cfg_t *cfg);
wtk_fextra_t* wtk_fextra_new2(wtk_fextra_cfg_t *cfg,wtk_matrix_t* hlda_mat);
int wtk_fextra_delete(wtk_fextra_t *p);
int wtk_fextra_init(wtk_fextra_t *p,wtk_fextra_cfg_t *cfg);
int wtk_fextra_init2(wtk_fextra_t *p,wtk_fextra_cfg_t *cfg, wtk_matrix_t* hlda_mat);
int wtk_fextra_clean(wtk_fextra_t *p);
int wtk_fextra_reset(wtk_fextra_t *p);

/**
 * @brief wtk_feature_t queue
 */
void wtk_fextra_set_output_queue(wtk_fextra_t *p,wtk_queue_t *q);

/**
 * @brief set notify;
 */
void wtk_fextra_set_notify(wtk_fextra_t *p,wtk_fextra_feature_notify_f notify,void *notify_ths);

void wtk_fextra_set_win_notify(wtk_fextra_t *p,wtk_fextra_window_notify_f notify,void *win_ths);

/**
 * @brief reuse feature;
 */
int wtk_fextra_push_feature(wtk_fextra_t *p,wtk_feat_t *f);

/**
 * @brief pop feature for use;
 */
wtk_feat_t* wtk_fextra_pop_feature(wtk_fextra_t *p);

/**
 * @brief reuse feature;
 */
int wtk_fextra_reuse_feature(wtk_fextra_t *p,wtk_feat_t *f);

/**
 * @brief feaute real vector size;
 */
int wtk_fextra_feature_size(wtk_fextra_t *p);

/**
 * @brief raise feature and there is some post function to process like cvn;
 */
void wtk_fextra_output_feature(wtk_fextra_t *p,wtk_feat_t *f);


/**
 * @biref raise feature to dnn;
 */
void wtk_fextra_output_feature_to_dnn(wtk_fextra_t *p,wtk_feat_t *f);

/**
 * @biref raise feature to output queue;
 */
void wtk_fextra_output_feature_to_queue(wtk_fextra_t *p,wtk_feat_t *f);

/**
 * @brief set post call-back.
 *
 * when parm get static and dynamic feature,will call post to do static feature post process,
 */
void wtk_fextra_set_post(wtk_fextra_t *p,wtk_fextra_post_f post,void *hook);

/**
 *	@brief set static post call-back;
 */
void wtk_fextra_set_static_post(wtk_fextra_t *p,wtk_fextra_static_post_f static_post,void *hook);

/**
 *	@brief feed sample data, each sample type is short .
 */
int wtk_fextra_feed(wtk_fextra_t *p,short* data,int samples,int is_end);

/**
 *	@brief feed raw data, data can have odd byte, parm will save odd data wait for next byte.
 */
int wtk_fextra_feed2(wtk_fextra_t *p,char* data,int bytes,int is_end);

int wtk_fextra_feed_float(wtk_fextra_t *p,float* data,int n,int is_end);


/**
 * @brief do Z;
 */
void wtk_fextra_z(wtk_fextra_t *p,wtk_queue_t *q);

/**
 * @brief do ENORMALIZE
 */
void wtk_fextra_normalize_energy(wtk_fextra_t *p,wtk_queue_t *q);


//------------------ feture process --------------------------
void wtk_fextra_feed_input_feature(wtk_fextra_t *p,wtk_feat_t *f);
void wtk_fextra_feed_end(wtk_fextra_t *p);

void wtk_fextra_feed_plp(wtk_fextra_t *p,wtk_feat_t *f);
void wtk_fextra_flush_end(wtk_fextra_t *p);

void wtk_fextra_flush_layer(wtk_fextra_t *p,int force);
int wtk_fextra_can_flush_all(wtk_fextra_t *p);
//----------------------------------------------------------
int wtk_fextra_bytes(wtk_fextra_t *p);
void wtk_fextra_test_file(wtk_fextra_t *p,char *fn);

void wtk_fextra_inc_feat_use(wtk_fextra_t *p,wtk_feat_t *f);
void wtk_fextra_dec_feat_use(wtk_fextra_t *p,wtk_feat_t *f);
void wtk_fextra_feed_static_input_feature(wtk_fextra_t *p,wtk_feat_t *f);

void wtk_fextra_get_cache(wtk_fextra_t *p);
#ifdef __cplusplus
};
#endif
#endif
