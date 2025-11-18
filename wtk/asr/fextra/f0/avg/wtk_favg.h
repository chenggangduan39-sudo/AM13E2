#ifndef WTK_VITE_F0_AVG_WTK_FAVG_H_
#define WTK_VITE_F0_AVG_WTK_FAVG_H_
#include "wtk/core/wtk_type.h"
#include "wtk/core/wtk_robin.h"
#include "wtk/core/wtk_bit_heap.h"
#include "wtk/core/wtk_slot.h"
#include "wtk/core/math/wtk_guassrand.h"
#include "wtk_favg_cfg.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_favg wtk_favg_t;

typedef struct
{
	wtk_queue_node_t q_n;
	int index;
	int used;
	float f0;
	float post_f0;
}wtk_avgfeat_t;
typedef void (*wtk_avgfeat_raise_feat_f)(void *ths,wtk_avgfeat_t *f);

struct wtk_favg
{
	wtk_favg_cfg_t *cfg;
	wtk_robin_t *win_robin;	//wtk_avgfeat_t* robin;
	wtk_avgfeat_t **win_avgfeat;
	wtk_bit_heap_t *avgheap;
	int index;
	int valid_f0;
	double avg_f0;
	double last_post_f0;
	wtk_guassrand_t guass_rand;
	wtk_queue_t outputq;	//wtk_avgfeat_t;
	//wtk_avgfeat_raise_feat_f raise_f;
	//void *raise_ths;
};

wtk_favg_t* wtk_favg_new(wtk_favg_cfg_t *cfg);
void wtk_favg_delete(wtk_favg_t *avg);
//void wtk_favg_set_raise_feat_callback(wtk_favg_t *avg,void *raise_ths,wtk_avgfeat_raise_feat_f raise);
void wtk_favg_reset(wtk_favg_t *avg);
void wtk_favg_reuse(wtk_favg_t *avg,wtk_avgfeat_t *f);
void wtk_favg_feed(wtk_favg_t *avg,float f0);
void wtk_favg_flush_end(wtk_favg_t *avg);
#ifdef __cplusplus
};
#endif
#endif
