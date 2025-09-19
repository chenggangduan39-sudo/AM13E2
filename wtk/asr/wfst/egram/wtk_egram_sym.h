#ifndef WTK_FST_EGRAM_WTK_EGRAM_SYM_H_
#define WTK_FST_EGRAM_WTK_EGRAM_SYM_H_
#include "wtk/core/wtk_type.h"
#include "wtk_kvdict.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef wtk_dict_word_t*(*wtk_egram_sym_get_word_f)(void *ths,char *wrd,int wrd_bytes);
typedef wtk_dict_word_t*(*wtk_egram_sym_get_word_f2)(void *ths,char *wrd,int wrd_bytes, void *info);
typedef wtk_string_t* (*wtk_egram_sym_get_str_f)(void *ths,int id);
typedef int (*wtk_egram_sym_get_id_f)(void *ths,char *wrd,int wrd_bytes);

typedef struct
{
	void *ths;
	wtk_egram_sym_get_word_f get_word;
	wtk_egram_sym_get_word_f2 get_word2;
	wtk_egram_sym_get_str_f get_str;
	wtk_egram_sym_get_id_f get_id;
}wtk_egram_sym_t;

#ifdef __cplusplus
};
#endif
#endif
