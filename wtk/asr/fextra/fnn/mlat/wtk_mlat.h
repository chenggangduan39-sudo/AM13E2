#ifndef WTK_VITE_PARM_POST_DNN_MLAT_WTK_MLAT
#define WTK_VITE_PARM_POST_DNN_MLAT_WTK_MLAT
#include "wtk/core/wtk_type.h" 
#include "wtk_mlat_cfg.h"
#include "../../wtk_feat.h"
#include "wtk/core/wtk_robin.h"
#include "wtk/os/wtk_lockhoard.h"
#include "wtk/os/wtk_blockqueue.h"
#include "wtk/os/wtk_lockvpool.h"
#include "wtk/os/wtk_thread.h"
#include "wtk/core/wtk_queue_slot.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_mlat wtk_mlat_t;
struct wtk_fnn;

typedef struct
{
	wtk_queue_node_t q_n;
	wtk_vecf_t *v;
	wtk_feat_t *f;
}wtk_mlat_feature_t;

typedef struct
{
	wtk_queue_node_t q_n;
	void *hook;
}wtk_mlat_msg_t;

typedef struct
{
	wtk_queue_node_t q_n;
	wtk_vecf_t *v;
}wtk_mlat_thread_env_vec_t;

typedef struct
{
	wtk_queue_node_t q_n;
	wtk_veci_t *v;
}wtk_mlat_fix_env_veci_t;

typedef struct
{
	wtk_queue_node_t q_n;
	wtk_vecuc_t *v;
}wtk_mlat_fix_env_vecuc_t;


typedef struct
{
	wtk_vecf_t *layer0;
	union
	{
		wtk_veci_t *i;
		wtk_vecuc_t *u;
	}layer0_output;
	wtk_queue_t veci_q;
	wtk_queue_t vecuc_q;
}wtk_mlat_fix_env_t;

typedef union {
	double d;
	struct {
		int j, i;
	} n;
}wtk_xexp_t;

typedef struct
{
	wtk_mlat_t *mlat;
	wtk_thread_t thread;
	wtk_blockqueue_t msg_q;
	wtk_queue_t vecf_q;
	wtk_mlat_fix_env_t *fix;
	wtk_xexp_t d2i;
}wtk_mlat_thread_env_t;

struct wtk_mlat
{
	wtk_mlat_cfg_t *cfg;
	struct wtk_fnn *dnn;
	wtk_lockhoard_t feature_hoard;
	wtk_lockvpool_t *feature_qn_pool;
	wtk_blockqueue_t input_q;
	wtk_blockqueue_t output_q;
	wtk_sem_t wait_sem;
	wtk_mlat_thread_env_t **thread_env;
	wtk_thread_t output_thread;
	int nthread;
	int index;
	int raise_index;
	unsigned run:1;
	unsigned route_run:1;
};

wtk_mlat_t* wtk_mlat_new(wtk_mlat_cfg_t *cfg,struct wtk_fnn *dnn);
void wtk_mlat_delete(wtk_mlat_t *m);
void wtk_mlat_reset(wtk_mlat_t *m);
void wtk_mlat_process_layer(wtk_mlat_t *d,wtk_feat_t **pv,int npv,wtk_feat_t *f);
void wtk_mlat_flush(wtk_mlat_t *f);
void wtk_mlat_flush_end(wtk_mlat_t *f);
void wtk_mlat_wait_end(wtk_mlat_t *m);
#ifdef __cplusplus
};
#endif
#endif
