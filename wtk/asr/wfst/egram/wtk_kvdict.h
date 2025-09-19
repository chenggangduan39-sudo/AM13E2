#ifndef WTK_FST_EGRAM_WTK_KVDICT_H_
#define WTK_FST_EGRAM_WTK_KVDICT_H_
#include "wtk/core/wtk_type.h"
#include "wtk/asr/model/wtk_dict.h"
#include "wtk/core/wtk_fkv.h"
#ifdef __cplusplus
extern "C" {
#endif
#define wtk_kvdict_get_word_s(kv,w) wtk_kvdict_get_word(kv,w,sizeof(w)-1)

typedef struct wtk_kvdict wtk_kvdict_t;
struct wtk_kvdict
{
	wtk_fkv_t *fkv;
	wtk_dict_t *dict;
	wtk_label_t *label;
	int phn_hash_hint;
	int wrd_hash_hint;
};

wtk_kvdict_t* wtk_kvdict_new(wtk_label_t *label,char *fn,
		int hash_hint,int phn_hash_hint,int wrd_hash_hint);
wtk_kvdict_t* wtk_kvdict_new2(wtk_label_t *label,
		int hash_hint,int phn_hash_hint,int wrd_hash_hint,
		FILE *f,int of,int len);
void wtk_kvdict_reset(wtk_kvdict_t *kv);
void wtk_kvdict_delete(wtk_kvdict_t *kv);
wtk_dict_word_t* wtk_kvdict_get_word(wtk_kvdict_t *kv,char *w,int bytes);
wtk_dict_word_t* wtk_kvdict_add_word2(wtk_dict_t *dict,char *w,int bytes, char *s, int len);
void wtk_kvdict_wrd_print(wtk_dict_word_t* w);
void wtk_kvdict_pron_print(wtk_dict_pron_t* pron);
#ifdef __cplusplus
};
#endif
#endif
