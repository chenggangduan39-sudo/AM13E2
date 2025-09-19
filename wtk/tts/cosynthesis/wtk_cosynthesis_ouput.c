/*
 * wtk_cosynthesis_output.c
 *
 *  Created on: Dec 31, 2021
 *      Author: dm
 */
#include <stdio.h>
#include <float.h>
#include "wtk_cosynthesis_output.h"

wtk_cosynthesis_output_t* wtk_cosynthesis_output_new()
{
	wtk_cosynthesis_output_t *output;

	output=(wtk_cosynthesis_output_t*) wtk_malloc(sizeof(*output));
	output->audiodata = wtk_strbuf_new(16000*2, 1);
	output->unitid = wtk_strbuf_new(30*3, 1);
	output->log = wtk_strbuf_new(64,1);

	output->isend = 0;
	output->snt_num = 0;
	output->loss = 0;
	output->in=0;
	output->state=1;
	output->wrds_tot=0;

	return output;
}

int wtk_cosynthesis_output_reset(wtk_cosynthesis_output_t* output)
{
	if (output->isend)
	{
		wtk_strbuf_reset(output->audiodata);
		wtk_strbuf_reset(output->unitid);
		wtk_strbuf_reset(output->log);
		output->isend = 0;
		output->snt_num = 0;
		output->loss = 0;
		output->in=0;
		output->state=1;
		output->wrds_tot=0;
	}

	return 0;
}

void wtk_cosynthesis_output_delete(wtk_cosynthesis_output_t* output)
{
	wtk_strbuf_delete(output->audiodata);
	wtk_strbuf_delete(output->unitid);
	wtk_strbuf_delete(output->log);
	wtk_free(output);
}

int wtk_cosynthesis_output_append(wtk_cosynthesis_output_t* output, short* data, int len, short* path, int path_len, float loss)
{
	int ret=0;

	if(len > 0 && path_len > 0)
	{
		wtk_strbuf_push(output->audiodata, (char*)data, len*2);
		wtk_strbuf_push(output->unitid, (char*)path, path_len*2);
	}

	//wtk_debug("output->loss=%f loss=%f\n", output->loss, loss);
	if (output->loss < loss)
		output->loss = loss;

	output->wrds_tot +=path_len;

	return ret;
}

void wtk_cosynthesis_output_set(wtk_cosynthesis_output_t* output, int type, float loss, char* extdata, int extlen)
{
	char a[100]={0};

	switch(type){
	case QTK_COSYN_STATUS_SUCCESS:
		output->state=1;
		if (output->loss < loss)
			output->loss = loss;
		if (loss < DBL_EPSILON)
		{
			if (output->snt_num <= 1)  //first part-sentences
				output->in = 1;
		}
		else
			output->in = 0;
		break;
	case QTK_COSYN_STATUS_FAILED:
		output->state=0;
		output->in = 0;
		output->loss = __FLT_MAX__;
		break;
	case QTK_COSYN_STATUS_ERR_INPUT:
		output->state=0;
		output->in = 0;
		if (extdata)
			sprintf(a, "type[input err]: %.*s", extlen, extdata);
		else
			sprintf(a, "type[input err]");
		output->loss = __FLT_MAX__;
		wtk_strbuf_push(output->log, a, strlen(a)+1);
		break;
	case QTK_COSYN_STATUS_ERR_USEAUTH:
		output->state=0;
		output->in = 0;
		output->loss = __FLT_MAX__;
		sprintf(a, "engine times limits: overcome max times.");
		wtk_strbuf_push(output->log, a, strlen(a)+1);
		break;
	case QTK_COSYN_STATUS_ERR_MAXNWRD:
		output->state=0;
		output->in = 0;
		output->loss = __FLT_MAX__;
		sprintf(a, "type[input err]: overcome max words.");
		wtk_strbuf_push(output->log, a, strlen(a)+1);
		break;
	case QTK_COSYN_STATUS_ERR_OUTDICT:       //input text error
		output->state=0;
		output->in = 0;
		output->loss = __FLT_MAX__;
		sprintf(a, "type[input err]: [%.*s] don't in corpus.", extlen, extdata);
		wtk_strbuf_push(output->log, a, strlen(a)+1);
		break;
	case QTK_COSYN_STATUS_ERR_PROC:          //process error
		output->state=0;
		output->in = 0;
		output->loss = __FLT_MAX__;
		sprintf(a, "type[engine err]: running exceptions.");
		wtk_strbuf_push(output->log, a, strlen(a)+1);
		break;
	case QTK_COSYN_STATUS_ERR_OTHER:
		output->state=0;
		output->in = 0;
		output->loss = __FLT_MAX__;
		sprintf(a, "type[engine err]: other unknown.");
		wtk_strbuf_push(output->log, a, strlen(a)+1);
		break;
	case QTK_COSYN_STATUS_ERR_MP3DECODE:
		output->state=0;
		output->in = 0;
		output->loss = __FLT_MAX__;
		sprintf(a, "decode error create mp3 bin error.");
		wtk_strbuf_push(output->log, a, strlen(a)+1);
		break;
	default:
		output->state=0;
		output->in = 0;
		output->loss = __FLT_MAX__;
		wtk_debug("unknown operations\n");
		break;
	}
	//saving end with '\0'. same don't avoid causing multi add.
	if (output->log->pos > 0)
	{
		wtk_strbuf_push_c(output->log, 0);
		output->log->pos--;
	}
}

int wtk_cosynthesis_output_issucc(wtk_cosynthesis_output_t* output)
{
	return output->state==QTK_COSYN_STATUS_SUCCESS;
}
