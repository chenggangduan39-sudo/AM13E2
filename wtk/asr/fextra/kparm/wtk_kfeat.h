#ifndef WTK_ASR_PARM_WTK_KFEAT
#define WTK_ASR_PARM_WTK_KFEAT
#include "wtk/core/wtk_type.h" 
#include "wtk/core/wtk_queue.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_kfeat wtk_kfeat_t;

typedef void(*wtk_kfeat_notify_f)(void *ths,wtk_kfeat_t *feat);

struct wtk_kfeat
{
    wtk_queue_node_t hoard_n;
    wtk_queue_node_t q_n;
    wtk_queue_node_t feat_n;
    int len;
    float *v;
    unsigned int index;
    unsigned char used;
};

wtk_kfeat_t* wtk_kfeat_new(int n);
int wtk_kfeat_bytes(int n);
int wtk_kfeat_delete(wtk_kfeat_t *f);

#ifdef __cplusplus
};
#endif
#endif

