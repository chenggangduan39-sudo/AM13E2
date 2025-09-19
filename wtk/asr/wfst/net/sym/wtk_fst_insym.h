#ifndef WTK_FST_NET_SYM_WTK_FST_INSYM_H_
#define WTK_FST_NET_SYM_WTK_FST_INSYM_H_
#include "wtk/core/wtk_type.h"
#include "wtk/core/wtk_label.h"
#include "wtk/core/cfg/wtk_source.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_fst_insym wtk_fst_insym_t;

typedef enum
{
	WTK_FST_INSYM_NORM,
	WTK_FST_INSYM_SIL,
	WTK_FST_INSYM_SIL_PRE,
	WTK_FST_INSYM_SIL_SUF
}wtk_fst_insym_type_t;

typedef struct
{
	wtk_string_t *str;
	int id;
	//wtk_fst_insym_type_t type;
}wtk_fst_insym_item_t;

struct wtk_fst_insym
{
	wtk_label_t *label;
	wtk_fst_insym_item_t **ids;
	unsigned int nid;
	wtk_str_hash_t *hash;
	wtk_heap_t *heap;
	int sil_id;
};


wtk_fst_insym_t *wtk_fst_insym_new(wtk_label_t *label,char *fn,int use_hash);
wtk_fst_insym_t *wtk_fst_insym_new2(wtk_label_t *label,char *fn,int use_hash,wtk_source_loader_t *sl);
wtk_fst_insym_t *wtk_fst_insym_new3(wtk_label_t *label,char *fn,int use_hash,
		wtk_source_loader_t *sl,int use_bin);
void wtk_fst_insym_delete(wtk_fst_insym_t *sym);
int wtk_fst_insym_get_index(wtk_fst_insym_t *sym,wtk_string_t *v);
int wtk_fst_insym_get_index2(wtk_fst_insym_t *sym,char *data,int bytes);
int wtk_fst_insym_bytes(wtk_fst_insym_t *sym);
#ifdef __cplusplus
};
#endif
#endif
