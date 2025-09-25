#ifndef WTK_FST_REC_RNN_WTK_HS_TREE
#define WTK_FST_REC_RNN_WTK_HS_TREE
#include "wtk/core/wtk_type.h" 
#include "wtk/core/wtk_str_hash.h"
#include "wtk/core/cfg/wtk_source.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_hs_tree wtk_hs_tree_t;
#define wtk_hs_tree_get_word_s(r,s)  wtk_hs_tree_get_word(r,s,sizeof(s)-1)

typedef struct
{
	wtk_string_t *name;
	unsigned char *code;
	unsigned int wrd_index;
	unsigned short *point;
	unsigned int codelen;
}wtk_hs_tree_wrd_t;


struct wtk_hs_tree
{
	wtk_str_hash_t *voc_hash;
	wtk_hs_tree_wrd_t *eos;
	wtk_hs_tree_wrd_t *oov;

};

wtk_hs_tree_t* wtk_hs_tree_new(char *fn,int use_bin,wtk_source_loader_t *sl);
void wtk_hs_tree_delete(wtk_hs_tree_t *tree);
wtk_hs_tree_wrd_t* wtk_hs_tree_get_word(wtk_hs_tree_t *r,char *name,int name_bytes);
#ifdef __cplusplus
};
#endif
#endif
