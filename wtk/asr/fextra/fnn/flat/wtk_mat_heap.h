#ifndef WTK_VITE_PARM_POST_DNN_FLAT_WTK_MAT_HEAP_H_
#define WTK_VITE_PARM_POST_DNN_FLAT_WTK_MAT_HEAP_H_
#include "wtk/core/wtk_type.h"
#include "wtk/core/math/wtk_mat.h"
#include "wtk/core/wtk_hash.h"
#include "wtk/core/wtk_queue.h"
#include "wtk/core/wtk_vpool.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_mat_heap wtk_mat_heap_t;

typedef struct
{
	wtk_queue_node_t q_n;
	wtk_mati_t *m;
}wtk_mat_heap_node_t;

struct wtk_mat_heap
{
	wtk_heap_t *heap;
	wtk_queue_t **slot;
	int nslot;
	int max_row;
	int max_col;
	wtk_vpool_t *node_pool;
};

wtk_mat_heap_t* wtk_mat_heap_new(int max_row,int max_col);
void wtk_mat_heap_delete(wtk_mat_heap_t *h);

wtk_mati_t* wtk_mat_heap_pop(wtk_mat_heap_t *h,int row,int col);
void wtk_mat_heap_push(wtk_mat_heap_t *h,wtk_mati_t *m);

#ifdef __cplusplus
};
#endif
#endif
