/*
 * wtk_defpron.c
 *
 *  Created on: Mar 1, 2017
 *      Author: dm
 */
#include "wtk_defpron.h"

wtk_defpron_t* wtk_defpron_new()
{
	wtk_defpron_t * defpron;

	defpron=(wtk_defpron_t*)wtk_malloc(sizeof(*defpron));
	defpron->heap=wtk_heap_new(1024);
	defpron->buf=wtk_strbuf_new(1,256);
	defpron->wrds=NULL;

	return defpron;
}

int wtk_defpron_bytes(wtk_defpron_t* defpron)
{
	int bytes;

	bytes=wtk_heap_bytes(defpron->heap);
	//wtk_debug("bytes=%f M\n",bytes*1.0/(1024*1024));

	return bytes;
}

int wtk_defpron_setwrd(wtk_defpron_t* defpron, wtk_string_t* k, wtk_string_t* v)
{
	wtk_tts_wrd_pron_t* pron;

	if (!defpron->wrds){
		defpron->wrds=wtk_kdict_new(NULL,10,defpron->buf,NULL);
	}
	pron=wtk_kdict_get(defpron->wrds, k->data, k->len);
	if(!pron)
	{
		pron=wtk_tts_wrd_pron_parse(NULL, defpron->heap, v->data, v->len);
		wtk_str_hash_add2(defpron->wrds->hash,k->data,k->len,pron);
	}

	return 0;
}

wtk_tts_wrd_pron_t* wtk_defpron_find(wtk_defpron_t* defpron, char* data, int len)
{
	wtk_tts_wrd_pron_t* pron;
	pron=NULL;
	if (defpron->wrds){
		pron=(wtk_tts_wrd_pron_t*)wtk_kdict_get(defpron->wrds, data, len);
	}

	return pron;
}

int wtk_defpron_reset(wtk_defpron_t* defpron)
{
	wtk_strbuf_reset(defpron->buf);
	if (defpron->wrds)
		wtk_kdict_delete(defpron->wrds);
	defpron->wrds=NULL;
	wtk_heap_reset(defpron->heap);
	return 0;
}

void wtk_defpron_delete(wtk_defpron_t* defpron)
{
	if (defpron->wrds){
		wtk_kdict_delete(defpron->wrds);
	}
	wtk_strbuf_delete(defpron->buf);
	wtk_heap_delete(defpron->heap);
	wtk_free(defpron);
}
