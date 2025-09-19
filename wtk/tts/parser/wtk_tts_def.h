#ifndef WTK_TTS_PARSER_WTK_TTS_DEF
#define WTK_TTS_PARSER_WTK_TTS_DEF
#include "wtk/core/wtk_type.h"
#include "wtk/core/wtk_array.h"
#include "wtk/core/wtk_strbuf.h"
#include "wtk/core/wtk_heap.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_tts_syl wtk_tts_syl_t;
typedef struct wtk_tts_sylphn wtk_tts_sylphn_t;
typedef struct wtk_tts_wrd_pron wtk_tts_wrd_pron_t;
typedef struct wtk_tts_wrd wtk_tts_wrd_t;
typedef struct wtk_tts_wrd_xpron wtk_tts_wrd_xpron_t;
typedef struct wtk_tts_xsyl wtk_tts_xsyl_t;
typedef struct wtk_tts_snt wtk_tts_snt_t;

typedef enum
{
	WTK_TTS_BOUND_PHONE=-1,//音素边界
	WTK_TTS_BOUND_SYL,//音节边界
	WTK_TTS_BOUND_RYTHYME,//韵律词边界
	WTK_TTS_BOUND_PHASE,//次短语边界
	WTK_TTS_BOUND_PHASE2,//主短语边界
	WTK_TTS_BOUND_CSEG,//子句边界
	WTK_TTS_BOUND_SEG,//句子边界
}wtk_tts_bound_t;

struct wtk_tts_sylphn
{
	wtk_string_t **phns;
	int nphn;
};

struct wtk_tts_syl
{
	//wtk_tts_sylphn_t *phn;
	wtk_string_t *v;
	int tone;
};

struct wtk_tts_wrd_pron
{
	wtk_tts_wrd_pron_t *next;
	wtk_tts_syl_t *syls;
	int nsyl;
	int npron;	//valid for head;
};

typedef struct
{
	wtk_tts_snt_t *snt;
	wtk_tts_wrd_t *wrd;
	wtk_tts_wrd_xpron_t *pron;
	wtk_tts_xsyl_t *syl;
	wtk_string_t *phn;
	wtk_string_t *lab;
	wtk_tts_bound_t bound;
	unsigned sil:1;
}wtk_tts_xphn_t;

struct wtk_tts_xsyl
{
	wtk_tts_wrd_t *wrd;
	wtk_tts_syl_t *syl;
	wtk_tts_sylphn_t *phn;
	wtk_array_t *phns;
	wtk_tts_bound_t bound;
	int tone;
};

struct wtk_tts_wrd_xpron
{
	wtk_tts_wrd_pron_t *pron;
	wtk_tts_xsyl_t *xsyl;
	wtk_tts_bound_t bound;
};

typedef enum
{
	WTK_TTS_SNT_NORM=0,	//陈述句
	WTK_TTS_SNT_QUES,	//一般疑问
	WTK_TTS_SNT_QUES2,	//特殊疑问
	WTK_TTS_SNT_SIGH,	//感叹
}wtk_tts_snt_type_t;

struct wtk_tts_wrd
{
	wtk_tts_snt_t *snt;
	wtk_string_t *v;
	wtk_string_t *pos;
	wtk_tts_wrd_xpron_t *pron;
	int index;
	int valid_pos;
	wtk_tts_bound_t bound;
	unsigned sil:1;
};

struct wtk_tts_snt
{
	wtk_tts_snt_type_t type;
	wtk_array_t *chars;
	wtk_array_t *wrds;
	wtk_array_t *syls;
	wtk_array_t *phns;
	wtk_string_t *snt;
	//int n_valid_wrd;
	//int n_valid_syl;
	unsigned sil_wrd_end:1;
	unsigned is_ctx_pick:1;     //if content of this sentence is picked up from context.
};

typedef struct
{
	wtk_array_t *snts;	//wtk_tts_snt_t;
	float speech_speed;
}wtk_tts_lab_t;

typedef struct
{
	wtk_heap_t *heap;
	wtk_strbuf_t *buf;
}wtk_tts_info_t;

void wtk_tts_sylphn_print(wtk_tts_sylphn_t *p);

void wtk_tts_wrd_pron_print(wtk_tts_wrd_pron_t *p);

wtk_tts_wrd_t* wtk_tts_wrd_new(wtk_heap_t *heap,wtk_string_t *v);
void wtk_tts_wrd_print(wtk_tts_wrd_t *wrd);
int wtk_tts_wrd_nsyl(wtk_tts_wrd_t *wrd);

wtk_tts_snt_t* wtk_tts_snt_new(wtk_heap_t *heap,int hint);
char* wtk_tts_snt_type_to_str(wtk_tts_snt_t *snt);
void wtk_tts_snt_print(wtk_tts_snt_t *snt);
wtk_string_t* wtk_tts_snt_get_sylphn(wtk_tts_snt_t *snt,int idex);
int wtk_tts_snt_get_nsyl(wtk_tts_snt_t *snt);
wtk_tts_xsyl_t* wtk_tts_snt_get_syl(wtk_tts_snt_t *snt,int idx);
wtk_tts_xsyl_t* wtk_tts_snt_get_nxt_syl(wtk_tts_snt_t *snt,wtk_tts_xsyl_t *syl);
int wtk_tts_snt_get_syl_snt_pos(wtk_tts_snt_t *snt,wtk_tts_xsyl_t *syl);
wtk_tts_wrd_t* wtk_tts_snt_get_wrd(wtk_tts_snt_t *snt,int index);
int wtk_tts_snt_get_nwrd(wtk_tts_snt_t *s);

wtk_tts_xphn_t* wtk_tts_xphn_new(wtk_heap_t *heap);
int wtk_tts_xphn_is_last_in_syl(wtk_tts_xphn_t *p);
wtk_tts_bound_t wtk_tts_xphn_get_syl_left_bound(wtk_tts_xphn_t *phn);
wtk_tts_bound_t wtk_tts_xphn_get_syl_right_bound(wtk_tts_xphn_t *phn);
wtk_tts_bound_t wtk_tts_xphn_get_wrd_left_bound(wtk_tts_xphn_t *phn);
wtk_tts_bound_t wtk_tts_xphn_get_wrd_right_bound(wtk_tts_xphn_t *phn);
wtk_string_t* wtk_tts_xphn_get_phn_in_syl_pos(wtk_tts_xphn_t *phn);
wtk_string_t* wtk_tts_xphn_get_syl_in_wrd_pos(wtk_tts_xphn_t *phn);
wtk_string_t* wtk_tts_xphn_get_syl_in_phase_pos(wtk_tts_xphn_t *phn);
wtk_string_t* wtk_tts_xphn_get_syl_in_seg_pos(wtk_tts_xphn_t *phn);
wtk_string_t* wtk_tts_xphn_get_wrd_in_phase_pos(wtk_tts_xphn_t *phn);
wtk_string_t* wtk_tts_xphn_get_wrd_in_seg_pos(wtk_tts_xphn_t *phn);
wtk_string_t* wtk_tts_xphn_get_phase_in_seg_pos(wtk_tts_xphn_t *phn);
wtk_tts_wrd_t* wtk_tts_xphn_get_next_word(wtk_tts_xphn_t *phn);
int wtk_tts_xphn_get_forward_syl_pos(wtk_tts_xphn_t *phn);
int wtk_tts_xphn_get_backward_syl_pos(wtk_tts_xphn_t *phn);
int wtk_tts_xphn_get_syl_snt_pos(wtk_tts_xphn_t *phn);
int wtk_tts_xphn_is_shengmu(wtk_tts_xphn_t *phn);
int wtk_tts_is_vowel(wtk_string_t *v);
int wtk_tts_is_zero_init(wtk_string_t *v);
int wtk_tts_is_erhua(wtk_string_t *v);
int wtk_tts_is_erhua2(wtk_tts_sylphn_t *phn);

int wtk_tts_xsyl_get_forward_wrd_pos(wtk_tts_xsyl_t *syl);
int wtk_tts_xsyl_get_backward_wrd_pos(wtk_tts_xsyl_t *syl);

void wtk_tts_lab_print_bound(wtk_tts_lab_t *lab);
void wtk_tts_lab_print(wtk_tts_lab_t *lab);
void wtk_tts_wrd_xpron_print(wtk_tts_wrd_xpron_t *pron);
void wtk_tts_lab_print_syl(wtk_tts_lab_t *lab);
void wtk_tts_snt_print_syl(wtk_tts_snt_t *s);
void wtk_tts_segsnt_print(wtk_tts_lab_t *lab);
void wtk_tts_lab_segwrd_print(wtk_tts_lab_t *lab);
#ifdef __cplusplus
};
#endif
#endif
