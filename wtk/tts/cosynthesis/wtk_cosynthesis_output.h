/*
 * wtk_cosynthesis_info.h
 *
 *  Created on: Dec 31, 2021
 *      Author: dm
 */

#ifndef WTK_COSYNTHESIS_WTK_COSYNTHESIS_OUTPUT_H_
#define WTK_COSYNTHESIS_WTK_COSYNTHESIS_OUTPUT_H_
#include "wtk/core/wtk_strbuf.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
	int state;        //synthesis success(1) or failed(0).
	float loss;
	char* log;
}wtk_cosynthesis_info_t;

typedef struct
{
	wtk_strbuf_t *audiodata;
	wtk_strbuf_t *unitid;
	wtk_strbuf_t *log;
	int isend;
	int wrds_tot;     // total of words
	int snt_num;
	float loss;
	int state;        //synthesis success(1) or failed(0).
	int in;           //synthesis in corpus(1) or not (0). if multi-sentences, all must in corpus.
}wtk_cosynthesis_output_t;


enum{
	QTK_COSYN_STATUS_FAILED=0,
	QTK_COSYN_STATUS_SUCCESS,
	QTK_COSYN_STATUS_ERR_INPUT,         //input text error
	QTK_COSYN_STATUS_ERR_USEAUTH,       //engine authorize
	QTK_COSYN_STATUS_ERR_MAXNWRD,       //input text error, overcome max num of words.
	QTK_COSYN_STATUS_ERR_OUTDICT,       //input text error
	QTK_COSYN_STATUS_ERR_PROC,          //process error
	QTK_COSYN_STATUS_ERR_MP3DECODE,     //MP3 decode error
	QTK_COSYN_STATUS_ERR_OTHER         //other unknown error
};

wtk_cosynthesis_output_t* wtk_cosynthesis_output_new();
int wtk_cosynthesis_output_reset(wtk_cosynthesis_output_t* output);
void wtk_cosynthesis_output_delete(wtk_cosynthesis_output_t* output);
int wtk_cosynthesis_output_append(wtk_cosynthesis_output_t* output, short* data, int len, short* path, int path_len, float loss);
void wtk_cosynthesis_output_set(wtk_cosynthesis_output_t* output, int type, float loss, char* extdata, int extlen);
int wtk_cosynthesis_output_issucc(wtk_cosynthesis_output_t* output);

#ifdef __cplusplus
};
#endif
#endif /* WTK_COSYNTHESIS_WTK_COSYNTHESIS_OUTPUT_H_ */
