#include "wtk_tts_poshmm.h" 

wtk_tts_poshmm_t* wtk_tts_poshmm_new(wtk_strpool_t *pool,int nwrd)
{
	wtk_tts_poshmm_t *h;

	h=(wtk_tts_poshmm_t*)wtk_malloc(sizeof(wtk_tts_poshmm_t));
	h->pool=pool;
	h->N=0;
	h->M=0;
	h->A=NULL;
	h->pi=NULL;
	h->nwrd=nwrd;
	h->mat=NULL;
	h->hash=NULL;
	return h;
}

void wtk_tts_poshmm_delete(wtk_tts_poshmm_t *h)
{
	if(h->A)
	{
		wtk_matrix_delete(h->A);
	}
	if(h->pi)
	{
		wtk_vector_delete(h->pi);
	}
	if(h->mat)
	{
		wtk_sparsem_delete(h->mat);
		//wtk_matrix_delete(h->mat);
	}
	if(h->hash)
	{
		wtk_str_hash_delete(h->hash);
	}
	wtk_free(h);
}

int wtk_tts_poshmm_bytes(wtk_tts_poshmm_t *h)
{
	int bytes=sizeof(wtk_tts_poshmm_t);

	if(h->A)
	{
		bytes+=wtk_matrix_bytes2(h->A);
	}
	if(h->pi)
	{
		bytes+=wtk_vector_bytes2(h->pi);
	}
	if(h->mat)
	{
		bytes+=wtk_sparsem_bytes(h->mat);
	}
	if(h->hash)
	{
		bytes+=wtk_str_hash_bytes(h->hash);
	}

	return bytes;
}


int wtk_tts_poshmm_load(wtk_tts_poshmm_t *h,wtk_source_t *src)
{
	//wtk_matrix_t *m;
	wtk_sparsem_t *m;
	int v[3];
	int ret;
	float f;
	//int cnt=0;

	src->swap=0;
	ret=wtk_source_read_int(src,v,2,1);
	if(ret!=0){goto end;}
	//wtk_debug("%d/%d\n",v[0],v[1]);
	h->M=v[0];h->N=v[1];
	h->A=wtk_matrix_new(h->N,h->N);
	ret=wtk_source_read_matrix(src,h->A,1);
	if(ret!=0){goto end;}
	//wtk_matrix_print(h->A);
	h->pi=wtk_vector_new(h->N);
	ret=wtk_source_read_vector(src,h->pi,1);
	if(ret!=0){goto end;}
	//wtk_vector_print(h->pi);
	m=wtk_sparsem_new(h->N,h->M,h->M);

	//m=wtk_matrix_new(h->N,h->M);
	//wtk_debug("N=%d M=%d\n",h->N,h->M);
	///wtk_debug("bytes=%f M\n",wtk_sparsem_bytes(m)*1.0/(1024*1024));
	h->mat=m;
	while(1)
	{
		ret=wtk_source_read_int(src,v,3,1);
		if(ret!=0){break;}
		f=((float*)(v))[2];
		wtk_sparsem_set(m,v[0]-1,v[1]-1,f);
		//++cnt;
		//exit(0);
	}
	//wtk_debug("cnt=%d\n",cnt);
	ret=0;
end:
	//exit(0);
	return 0;
}

int wtk_tts_poshmm_load_voc(wtk_tts_poshmm_t *h,wtk_source_t *src)
{
	wtk_str_hash_t *hash;
	wtk_strbuf_t *buf;
	wtk_tts_hashidx_t *hi;
	int idx=0;
	int ret;
	char *data;

	buf=wtk_strbuf_new(256,1);
	hash=wtk_str_hash_new(h->nwrd*2+1);
	h->hash=hash;
	while(1)
	{
		ret=wtk_source_read_string(src,buf);
		if(ret!=0){ret=0;goto end;}
		ret=wtk_source_read_int(src,&(idx),1,0);
		if(ret!=0){goto end;}
		//wtk_debug("[%.*s]=%d\n",buf->pos,buf->data,idx);
		hi=(wtk_tts_hashidx_t*)wtk_heap_malloc(hash->heap,sizeof(wtk_tts_hashidx_t));
		hi->idx=idx;
		data=wtk_heap_dup_data(hash->heap,buf->data,buf->pos);
		wtk_str_hash_add_node(hash,data,buf->pos,hi,&(hi->node));
		//exit(0);
	}
	ret=0;
end:
	h->oov=idx;
	//wtk_debug("hash=%p ret=%d\n",h->hash,ret);
	wtk_strbuf_delete(buf);
	return ret;
}


int wtk_tts_poshmm_get_idx(wtk_tts_poshmm_t *h,char *data,int bytes)
{
	wtk_tts_hashidx_t *idx;

	idx=(wtk_tts_hashidx_t*)wtk_str_hash_find(h->hash,data,bytes);
	//wtk_debug("oov=%d\n",h->oov);
	return idx?idx->idx:h->oov;
}
