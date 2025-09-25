#ifndef WTK_CORE_WTK_ROBIN2
#define WTK_CORE_WTK_ROBIN2
#include "wtk/core/wtk_type.h" 
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_robin2 wtk_robin2_t;

struct wtk_robin2
{
	int nslot;
	int pop;
	int used;
	int slot_size;
	char *p;
};


wtk_robin2_t* wtk_robin2_new(int n,int vsize);
void wtk_robin2_delete(wtk_robin2_t *rb);
void wtk_robin2_reset(wtk_robin2_t *rb);
void wtk_robin2_push(wtk_robin2_t *rb,void *p);
void* wtk_robin2_get(wtk_robin2_t *rb,int idx);
void* wtk_robin2_pop(wtk_robin2_t *rb);
#ifdef __cplusplus
};
#endif
#endif
