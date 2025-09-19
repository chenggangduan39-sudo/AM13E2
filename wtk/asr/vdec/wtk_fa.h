#ifndef WTK_EVAL_WTK_FA_H_
#define WTK_EVAL_WTK_FA_H_
#include "wtk/asr/vdec/rec/wtk_rec.h"
#include "wtk/core/wtk_array.h"
#include <stdio.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_fa wtk_fa_t;

#define wtk_fa_word_nphn(w) ((w)->phones ? (w)->phones->nslot:0)

typedef enum
{
	wtk_no_forceout=0,
	wtk_fa_forceout=1,
	wtk_phn_forceout=2,
	wtk_both_forceout=3,
	wtk_empty_out=4
}wtk_fa_forceout_type_t;

typedef struct
{
	wtk_lab_t *lab;
	double score;				//MMF overlaped score
	double start;				//100ns
	double dur;					//100ns
	unsigned sil:1;
	unsigned embed:1;			//fa(phone) enmbed in  phone rec  or not;
}wtk_faphn_t;

typedef struct
{
	wtk_string_t *name;
	wtk_array_t *phones;	//wtk_faphn_t*
	double start;		//1OOns
	double end;			//100ns
	double dur;	//not include sil duration.
	double score;
	unsigned sil:1;
	unsigned embed:1;		//fa(word) enmbed in  phone rec  or not;
}wtk_fawrd_t;

struct wtk_fa
{
	wtk_array_t *words;		//wtk_fawrd_t* array,[!SENT_START WORD WORD !SENT_END],the first and last element is silence.
	double frame_dur;		//100ns
	double start;			//100ns
	double end;
	double dur;
	double score;	//FA Score
	int valid_wrds;		//include !SENT_START and !SENT_END
	int n_ref_words;
	int force_out;		//indicate which part is forceout(phn or fa);
	unsigned is_forceout:1;
};

wtk_fa_t* wtk_fa_new_h(wtk_heap_t *heap,wtk_transcription_t *fa_trans,
		wtk_transcription_t* phn_trans,double frame_dur);
int wtk_fa_nsilphn(wtk_fa_t *fa);
void wtk_fa_print(wtk_fa_t *fa);
int wtk_fawrd_nsilphn(wtk_fawrd_t *w);
void wtk_fa_cal_gop(wtk_fa_t *fa,wtk_transcription_t* phn_trans);
void wtk_fa_get_rec(wtk_fa_t *fa,wtk_strbuf_t *buf);
void wtk_fa_print_mlf(wtk_fa_t *fa,FILE* f);
void wtk_fa_post_proc(wtk_fa_t *fa,double cut_off);
void wtk_fa_phone_hack(wtk_fa_t *fa,double cut_off);
int wtk_fa_get_err_id(wtk_fa_t *fa);
wtk_faphn_t* wtk_fawrd_get_phn(wtk_fawrd_t *wrd,int index);
#ifdef __cplusplus
};
#endif
#endif
