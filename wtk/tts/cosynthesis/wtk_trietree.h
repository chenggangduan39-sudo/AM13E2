#ifndef __WTK_TRIETREE_H__
#define __WTK_TRIETREE_H__
#include "wtk/core/cfg/wtk_cfg_queue.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_trieitem
{
	int start;
	int len;
	int num;
}wtk_trieitem_t;

typedef struct
{
    wtk_queue_node_t q_n;
    wtk_queue_t *child_q;
    char *sname;
    uint8_t sname_len;
    uint16_t *unit_id;
    uint16_t nunit;
    unsigned is_leaf;
}wtk_trienode_t;

typedef struct
{
    wtk_queue_t *child_q;
    uint16_t *unit_id;
    uint16_t nunit;
}wtk_trieroot_t;

wtk_trieroot_t *wtk_trietree_new();
void wtk_trietree_delete(wtk_trieroot_t *root);
uint16_t *wtk_trietree_unit_get_root(wtk_trieroot_t *root,uint16_t *nunit);
uint16_t *wtk_trietree_unit_get_any(wtk_trieroot_t *root,char *s,int *len,uint16_t *nunit,int deep);
uint16_t *wtk_trietree_unit_get(wtk_trieroot_t *root,char *s,int *len, int size, uint16_t *nunit);
void wtk_trietree_root_insert(wtk_trieroot_t *root,char *s,int *len,uint16_t* unit, uint16_t nunit,int deep, int shift);
void wtk_trietree_update(wtk_trieroot_t *root);
int wtk_trietree_readitem(wtk_heap_t* heap, FILE *fp, wtk_trieroot_t **root, int wrd_idx, wtk_trieitem_t* item, int update);
#ifdef __cplusplus
};
#endif
#endif
