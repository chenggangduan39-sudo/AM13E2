#ifndef WTK_DECODER_VITERBI_WTK_TRANSCRIPTION_H_
#define WTK_DECODER_VITERBI_WTK_TRANSCRIPTION_H_
#include "wtk_lat.h"
#include "wtk/core/wtk_strbuf.h"
#include "wtk/core/cfg/wtk_source.h"
#include <stdio.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct wtk_transcription wtk_transcription_t;
typedef struct wtk_lablist wtk_lablist_t;
typedef struct wtk_lab wtk_lab_t;
typedef struct wtk_nbestentry wtk_nbestentry_t;

struct wtk_nbestentry
{
	wtk_nbestentry_t *next;	    //next for entry list.
	wtk_nbestentry_t *prev;		//prev entry in list.
	wtk_nbestentry_t *path_prev;		//next entry in one best path.
	wtk_lnode_t *lnode;
	wtk_larc_t *larc;
	double score;
	double like;
};

struct wtk_lab
{
	wtk_queue_node_t lablist_n;		//node in lablist.
	wtk_string_t *name;
	int name_id;                    //valid when name is phone.
	float score;			//primary score eg. logP
	wtk_string_t **aux_lab;		//array[1 .. max_aux_lab] [1]->model name; [2]->lm name;
	float 		*aux_score;		//array[1 .. max_aux_lab] [1]->model score; [2]->lm score;
	double start,end;			//start and end times 100ns units.
};

struct wtk_lablist
{
	wtk_queue_node_t trans_n; //node in transcription lab queue.
	wtk_queue_t lable_queue;
	int max_aux_lab;			//max aux labels(default=0)
};

struct wtk_transcription
{
	wtk_queue_t lab_queue;
	wtk_string_t *name;
	unsigned forceout:1;
	unsigned use_dummy_wrd:1;
	unsigned issil:1;      //all rec is sil.
};

wtk_transcription_t* wtk_transcription_new_h(wtk_heap_t *heap);
wtk_lablist_t* wtk_lablist_new_h(wtk_heap_t *heap,int naux);
wtk_lab_t* wtk_lab_new_h(wtk_heap_t *heap,int naux);
wtk_lab_t* wtk_transcription_peek(wtk_transcription_t *trans,int index);
double wtk_transcription_all_score(wtk_transcription_t *trans);
void wtk_lab_print(wtk_lab_t *l);
double wtk_transcription_get_overlap_prob(wtk_transcription_t* trans,wtk_lab_t* lab);
double wtk_transcription_get_tot_prob(wtk_transcription_t* trans);
void wtk_transcription_to_string(wtk_transcription_t *trans,wtk_strbuf_t *buf);
void wtk_transcription_print(wtk_transcription_t* trans);
void wtk_transcription_print2(wtk_transcription_t* trans);
void wtk_transcription_print3(wtk_transcription_t* trans,FILE* file);
void wtk_transcription_print4(wtk_transcription_t* trans,char *fn);
void wtk_transcription_print_hresults(wtk_transcription_t* trans,FILE* file);
void wtk_nbestentry_print(wtk_nbestentry_t *n);
void wtk_transcription_to_string2(wtk_transcription_t *trans,wtk_strbuf_t *buf,char sep);
int wtk_transcription_to_string3(wtk_transcription_t *trans,wtk_strbuf_t *buf,char sep);
wtk_array_t* wtk_transcription_getrec(wtk_transcription_t* trans, wtk_heap_t *heap);

typedef struct wtk_trans_info wtk_trans_info_t;
struct wtk_trans_info{
	wtk_array_t* a;
	int trace_model;
	int trace_state;
	int trace_word;
};

int wtk_transcription_load(wtk_array_t *array, wtk_source_t *sl);
wtk_array_t* wtk_transcription_load_file(char* fn, wtk_heap_t* heap, int n);
wtk_trans_info_t* wtk_transcription_load_file2(char* fn, wtk_heap_t* heap, int n, int state, int model, int word);
wtk_transcription_t* wtk_string_to_transcrpiton(wtk_heap_t* heap, char* data, int len, int trace_model,int trace_state,int trace_word);
wtk_transcription_t* wtk_string_to_transcrpiton2(wtk_heap_t* heap, char* data, int len, int trace_model,int trace_state,int trace_word);
#ifdef __cplusplus
};
#endif
#endif
