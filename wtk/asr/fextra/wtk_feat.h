#ifndef WTK_MATH_WTK_FEAT_H_
#define WTK_MATH_WTK_FEAT_H_
#include "wtk/core/math/wtk_vector.h"
#include "wtk/core/wtk_queue.h"
#include "wtk/core/cfg/wtk_source.h"
#include "wtk_feat_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_feat wtk_feat_t;
struct wtk_fextra_cfg;
typedef int (*wtk_feat_sender_t)(void*,wtk_feat_t*);
#define wtk_feat_inc(f) (++((f)->used))
#define wtk_feat_dec(f) (--((f)->used))
#define wtk_feat_size(f) wtk_vector_size((f)->rv)

struct wtk_feat
{
	wtk_feat_cfg_t *cfg;
	wtk_queue_node_t hoard_n;			//used for hoard cache;
	wtk_queue_node_t queue_n;			//used for output queue for connect two module.
	int index;							//index of feature;
	int used;							//feature used refrence;
	float energy;
	wtk_vector_t *v;	//sigp feature, PLP,MFCC,etc feature from  sig.
	wtk_vector_t *xf_v;	//if xform needed,save the transformed feature.
	wtk_vector_t *dnn_v;	//dnn vector
	wtk_vector_t *rv;	//pointer to current valid feature used for rec.
	void *app_hook;		//used for attached user data;
	void *send_hook;			//used for send callback;
	wtk_feat_sender_t send;
	union{
		int i;
		float f;
		void  *p;
	}hook;
};

wtk_feat_t* wtk_feat_new(wtk_feat_cfg_t *cfg);
wtk_feat_t* wtk_feat_new2(wtk_feat_cfg_t *cfg, int xf_size);
int wtk_feat_delete(wtk_feat_t *f);
void wtk_feat_print(wtk_feat_t *f);

/**
 * @brief send back feature, for reuse feature;
 */
int wtk_feat_send(wtk_feat_t *f);

void wtk_feat_print2(wtk_feat_t *f);
void wtk_feat_use_dec(wtk_feat_t *f);

/**
 * @brief decrease use and send back;
 */
void wtk_feat_push_back(wtk_feat_t *f);
int wtk_feat_bytes(struct wtk_fextra_cfg *cfg);
int wtk_feat_bytes2(struct wtk_fextra_cfg *cfg, int xf_size);

#ifdef __cplusplus
};
#endif
#endif
