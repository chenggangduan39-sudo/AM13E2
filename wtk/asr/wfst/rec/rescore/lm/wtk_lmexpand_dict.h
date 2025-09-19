#ifndef WTK_FST_LM_WTK_LMEXPAND_DICT_H_
#define WTK_FST_LM_WTK_LMEXPAND_DICT_H_
#include "wtk/core/wtk_type.h"
#include "wtk/core/wtk_str_hash.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_lmexpand_dict wtk_lmexpand_dict_t;

typedef struct
{
	wtk_string_t *name;
	int *ids;
	int nid;
}wtk_lmexpand_dict_item_t;

struct wtk_lmexpand_dict
{
	wtk_str_hash_t *hash;
};

wtk_lmexpand_dict_t* wtk_lmexpand_dict_new(char *fn);
void wtk_lmexpand_dict_delete(wtk_lmexpand_dict_t *d);
#ifdef __cplusplus
};
#endif
#endif
