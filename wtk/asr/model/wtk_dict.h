#ifndef WTK_MODEL_WTK_DICT_H_
#define WTK_MODEL_WTK_DICT_H_
#include "wtk/core/wtk_label.h"
#include "wtk/core/wtk_array.h"
#include "wtk/core/cfg/wtk_source.h"
#include "wtk/core/math/wtk_math.h"
#ifdef __cplusplus
extern "C" {
#endif
#define MINPRONPROB 1E-6
#define wtk_dict_find_word_s(d,w) wtk_dict_find_word(d,w,sizeof(w)-1)
typedef struct wtk_dict wtk_dict_t;
typedef struct wtk_dict_phone wtk_dict_phone_t;
typedef	struct wtk_dict_word wtk_dict_word_t;
typedef struct wtk_dict_pron wtk_dict_pron_t;
struct wtk_hmmset;
typedef wtk_dict_word_t* (*wtk_dict_word_find_f)(void *data,char *w,int w_bytes);
typedef wtk_dict_word_t* (*wtk_dict_word_find_f2)(void *data,char *w,int w_bytes, void* info);

typedef enum
{
	WTK_PHN_CN=0,	//context dependent.
	WTK_PHN_CI,		//context independent e.g. sil
	WTK_PHN_CF,		//context free	e.g. sp
}wtk_phone_type_t;

struct wtk_dict_phone
{
	wtk_string_t *name;
	wtk_phone_type_t type;
};

struct wtk_dict_pron
{
	wtk_dict_word_t *word;
	wtk_dict_pron_t *next;
	wtk_string_t *outsym;
	//wtk_string_t phns;
	wtk_dict_phone_t **pPhones;
	int nPhones;
	int pnum;		///* Pronunciation number 1..nprons */
	float prob;
};

/**
add by dmd
*/
enum{
	WTK_PRON_NOEXIST_INDICT=0,      //word pron doesn't exist in dict.
	WTK_PRON_EXIST_INDICT,        //word pron exist in dict
	WTK_PRON_GEN,           //word pron generate
	WTK_PRON_INCOMP,              //word pron incomplete
	WTK_PRON_DUMMY                //dummy pron.
};

struct wtk_dict_word
{
	wtk_string_t *name;
	wtk_dict_pron_t *pron_list;
	int npron;
	void *aux;		//attach user information;
	unsigned indict; //if is in pron_dict. wtk_pron_noexist: no exist, wtk_pron_exist: exist, wtk_pron_dummy: dummy pron.
	int maxnphn;    //max of number of phones all prons.
};

struct wtk_dict
{
	wtk_label_t *label;
	wtk_heap_t *heap;
	wtk_str_hash_t* phone_hash;
	wtk_str_hash_t* word_hash;
	wtk_dict_pron_t	*null_pron;
	wtk_dict_word_t *null_word;
	int 	npron;
	int 	nword;
	int		nphone;
	unsigned use_db:1;
};

/**
 * @param use_db not use now.
 */
wtk_dict_t* wtk_dict_new(wtk_label_t *l,int use_db);

wtk_dict_t* wtk_dict_new2(wtk_label_t *label,int use_db,
		int phn_hash_hint,int wrd_hash_hint);

int wtk_dict_delete(wtk_dict_t *d);
int wtk_dict_init(wtk_dict_t *d,wtk_label_t *label,int use_db);
int wtk_dict_init2(wtk_dict_t *d,wtk_label_t *label,int use_db,
		int phn_hash_hint,int wrd_hash_hint);
int wtk_dict_load(wtk_dict_t *d,wtk_source_t *s);
int wtk_dict_clean(wtk_dict_t *d);
int wtk_dict_add_sent_flat(wtk_dict_t *d);
wtk_dict_phone_t* wtk_dict_get_phone(wtk_dict_t* d,wtk_string_t *n,int insert);
wtk_dict_phone_t* wtk_dict_find_phone(wtk_dict_t *d,char* n,int nl);

/**
 * @brief dict will use n as word name, so n should be not freed;
 */
wtk_dict_word_t* wtk_dict_get_word(wtk_dict_t *d,wtk_string_t *n,int insert);

/**
 * @brief dict will cp n as word name, n can be freed;
 */
wtk_dict_word_t* wtk_dict_get_word2(wtk_dict_t *d,wtk_string_t *n,int insert);
wtk_dict_word_t* wtk_dict_get_word_handler(wtk_dict_t *d,char *w,int w_bytes);
wtk_dict_word_t* wtk_dict_find_word(wtk_dict_t *d,char* n,int nl);
int wtk_dict_is_closed(wtk_dict_t *d,struct wtk_hmmset *hl);
void wtk_dict_print(wtk_dict_t *d);
int wtk_dict_reset(wtk_dict_t *d);

/**
 * @brief add phones of specified pron;
 * @param prob: default is 0;
 */
wtk_dict_pron_t* wtk_dict_add_pron(wtk_dict_t *d,wtk_dict_word_t *w,wtk_string_t *outsym,wtk_string_t **phones,int nphones,float prob);
wtk_dict_word_t* wtk_dict_get_dummy_wrd(wtk_dict_t *d,char *w,int bytes);
void wtk_dict_word_print(wtk_dict_word_t *dw, int use_kv);
int wtk_dict_word_check(wtk_dict_word_t *wrd);
void wtk_dict_pron_print(wtk_dict_pron_t *pron);
void wtk_dict_pron_print2(wtk_dict_pron_t *pron, int use_kv);
wtk_dict_word_t* wtk_dict_add_merge_wordstr(wtk_dict_t *d,wtk_string_t *sym,wtk_dict_word_t **wrds,int nwrd);
wtk_dict_word_t* wtk_dict_add_merge_word(wtk_dict_t *d,wtk_string_t *sym,wtk_dict_word_t **wrds,int nwrd);
wtk_dict_word_t* wtk_dict_add_merge_wordid(wtk_dict_t *d,wtk_string_t *sym,wtk_dict_word_t **wrds,int nwrd);

void wtk_dict_reset_aux(wtk_dict_t *d);
int wtk_dict_word_npron(wtk_dict_word_t *dw);
#ifdef __cplusplus
};
#endif
#endif
