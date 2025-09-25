#ifndef WTK_FST_NET_FST_SYM_H_
#define WTK_FST_NET_FST_SYM_H_
#include "wtk/core/wtk_type.h"
#include "wtk/core/wtk_label.h"
#include "wtk/core/cfg/wtk_source.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_fst_sym wtk_fst_sym_t;

struct wtk_fst_sym
{
	wtk_label_t *label;
	wtk_string_t **strs;
	//int *len;
	unsigned int nstrs;
	wtk_heap_t *heap;
};

wtk_fst_sym_t *wtk_fst_sym_new(wtk_label_t *label,char *fn);
wtk_fst_sym_t* wtk_fst_sym_new2(wtk_label_t *label,char *fn,wtk_source_loader_t *sl);
wtk_fst_sym_t *wtk_fst_sym_new3(wtk_label_t *label,char *fn,wtk_source_loader_t *sl,int use_bin);
void wtk_fst_sym_delete(wtk_fst_sym_t *sym);
int wtk_fst_sym_get_index(wtk_fst_sym_t *sym,wtk_string_t *v);
int wtk_fst_sym_bytes(wtk_fst_sym_t *sym);
#ifdef __cplusplus
};
#endif
#endif
