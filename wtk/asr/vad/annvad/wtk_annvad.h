#ifndef WTK_ANNVAD_WTK_ANNVAD_H_
#define WTK_ANNVAD_WTK_ANNVAD_H_
#include "wtk/asr/fextra/ann/wtk_ann.h"
#include "wtk/asr/fextra/wtk_fextra.h"
#include "wtk/core/wtk_robin.h"
#include "wtk/core/math/wtk_matrix.h"
#include "wtk/asr/fextra/ann/wtk_ann_wbop.h"
#include "wtk/asr/vad/wtk_vframe.h"
#include "wtk/asr/vad/wtk_short_buffer.h"
#include "wtk_annvad_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_annvad wtk_annvad_t;
typedef void (*wtk_annvad_debug_f)(void *ths,int index,float sil,float speech);

typedef enum
{
	WTK_ANNVAD_SIL,
	WTK_ANNVAD_SPEECH,
}wtk_annvad_state_t;

typedef struct
{
	wtk_queue_node_t hoard_n;
	int index;
	int ref;						//referenced by robins;
	float *v;
}wtk_annfeat_t;

struct wtk_annvad
{
	wtk_annvad_cfg_t *cfg;
	//----------------- raise callback ------------
	void *raise_hook;
	wtk_vframe_raise_f raise;
	void *debug_ths;
	wtk_annvad_debug_f debug_f;
	//---------------------------------------------
	wtk_queue_t param_output_q;
	wtk_fextra_t *parm;
	wtk_robin_t *feature_robin;		//save feature,used as window; wtk_annfeat_t;
	wtk_robin_t *sil_robin;			//save sil wtk_feature;
	wtk_robin_t *speech_robin;		//save speech wtk_feature;
	wtk_annfeat_t **v_array;			//used for get current windowed feature array;
	wtk_matrix_t *merge_feature;	//used for merged feature;
	wtk_ann_wbop_t *wbop;
	wtk_short_buffer_t *frame_buffer;
	wtk_queue_t frame_q;		//wtk_vframe_t queue used for save audio;
	wtk_hoard_t frame_hoard;
	wtk_hoard_t annfeat_hoard;		//wtk_annfeat_t hoard;
	int n_frame_index;
	wtk_annvad_state_t state;
};

wtk_annvad_t* wtk_annvad_new(wtk_annvad_cfg_t *cfg,void *raise_hook,wtk_vframe_raise_f raise);
void wtk_annvad_delete(wtk_annvad_t *v);
void wtk_annvad_reset(wtk_annvad_t *v);
int wtk_annvad_feed(wtk_annvad_t *v,char *data,int bytes,int state);
void wtk_annvad_push_vframe(wtk_annvad_t *v,wtk_vframe_t *f);
//------------------------ private/example section ---------------------------
void wtk_annvad_flush_frame_queue(wtk_annvad_t *v,wtk_queue_t *q);
void wtk_annvad_print(wtk_annvad_t *v);
int wtk_annvad_feed_end(wtk_annvad_t *v);
int wtk_annvad_feed2(wtk_annvad_t *v,wtk_feat_t *f);
#ifdef __cplusplus
};
#endif
#endif
