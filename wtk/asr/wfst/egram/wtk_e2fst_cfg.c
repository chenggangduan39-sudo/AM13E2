#include "wtk/core/cfg/wtk_source.h"
#include "wtk_e2fst_cfg.h"

int wtk_e2fst_cfg_init(wtk_e2fst_cfg_t *cfg)
{
	wtk_fst_net_cfg_init(&(cfg->net));
        wtk_fst_net_cfg_init(&(cfg->filler_net));
        cfg->sym_out_fn=NULL;
	cfg->sym_out=NULL;
	cfg->phn_map_fn=NULL;
	cfg->hmm_map_fn=NULL;
	cfg->phn_hash_hint=35007;
	cfg->sym_hash_hint=35007;
	cfg->use_merge=0;
	cfg->use_chain=1;
	cfg->selfloop_scale=1.0;
	cfg->transition_scale=1.0;
	cfg->use_txt=0;
	cfg->phn_id_fn=NULL;
	cfg->phn_ids=NULL;
	cfg->add_sil=1;
	cfg->sil_id=1;
	cfg->sil_S_id=1;
	cfg->remove_dup=1;
	cfg->use_opt_sil=1;
	cfg->use_cross_wrd=1;
	cfg->use_sil_ctx=1;
	cfg->use_pre_wrd=1;
	cfg->use_biphone=0;
	cfg->use_posi=0;
	cfg->use_eval=0;
	cfg->hmm_map=NULL;
	cfg->hmm_maps=NULL;
	cfg->type = 0;
        cfg->add_filler = 0;
	cfg->filler = 0;
        return 0;
}

int wtk_e2fst_cfg_clean(wtk_e2fst_cfg_t *cfg)
{
	if(cfg->sym_out)
	{
		wtk_fst_insym_delete(cfg->sym_out);
	}
	if(cfg->phn_ids)
	{
		wtk_free(cfg->phn_ids);
	}
	wtk_fst_net_cfg_clean(&(cfg->net));
        wtk_fst_net_cfg_clean(&(cfg->filler_net));
        if(cfg->hmm_map)
	{
		wtk_free(cfg->hmm_map);
	}
	if(cfg->hmm_maps)
	{
		wtk_free(cfg->hmm_maps);
	}
	return 0;
}

int wtk_e2fst_cfg_update_local(wtk_e2fst_cfg_t *cfg,wtk_local_cfg_t *main)
{
	wtk_local_cfg_t *lc;
	wtk_string_t *v;
	int ret;

	cfg->heap=main->heap;
	lc=main;
	wtk_local_cfg_update_cfg_str(lc,cfg,sym_out_fn,v);
	wtk_local_cfg_update_cfg_str(lc,cfg,phn_map_fn,v);
	wtk_local_cfg_update_cfg_str(lc,cfg,hmm_map_fn,v);
	wtk_local_cfg_update_cfg_str(lc,cfg,phn_id_fn,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,phn_hash_hint,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,sym_hash_hint,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,selfloop_scale,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,transition_scale,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_merge,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_chain,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_txt,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,add_sil,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,remove_dup,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_opt_sil,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,sil_id,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,sil_S_id,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_cross_wrd,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_sil_ctx,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_pre_wrd,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_biphone,v);
	wtk_local_cfg_update_cfg_b(lc, cfg, add_filler, v);
	wtk_local_cfg_update_cfg_b(lc, cfg, filler, v);
        //wtk_local_cfg_update_cfg_b(lc,cfg,use_phn_bin,v);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_posi,v);
	if (cfg->sil_S_id==cfg->sil_id && cfg->use_posi)
	{
		cfg->sil_S_id=2;
	}
	wtk_local_cfg_update_cfg_b(lc,cfg,use_eval,v);
	lc=wtk_local_cfg_find_lc_s(main,"net");
	if(lc)
	{
		ret=wtk_fst_net_cfg_update_local(&(cfg->net),lc);
		if(ret!=0){goto end;}
	}
        lc = wtk_local_cfg_find_lc_s(main, "filler_net");
        if (lc) {
            ret = wtk_fst_net_cfg_update_local(&(cfg->filler_net), lc);
            if (ret != 0) {
                goto end;
            }
        }
        ret=0;
end:
	return ret;
}

int wtk_str_hash_load_int(wtk_str_hash_t *hash,wtk_source_t *src)
{
	wtk_strbuf_t *buf;
	wtk_heap_t *heap;
	wtk_string_t *v;
	int ret;
	int i;

	heap=hash->heap;
	buf=wtk_strbuf_new(256,1);
	while(1)
	{
		ret=wtk_source_read_string(src,buf);
		if(ret!=0){ret=0;goto end;}
		//wtk_debug("[%.*s]\n",buf->pos,buf->data);
		ret=wtk_source_read_int(src,&i,1,0);
		if(ret!=0){goto end;}
		//wtk_debug("v=%d\n",i);
		v=wtk_heap_dup_string(heap,buf->data,buf->pos);
		wtk_str_hash_add(hash,v->data,v->len,(void*)(long)i);
	}
	ret=0;
end:
	wtk_strbuf_delete(buf);
	return ret;
}

int wtk_str_hash_load_int_bin(wtk_str_hash_t *hash,wtk_source_t *src)
{
	wtk_strbuf_t *buf;
	wtk_heap_t *heap;
	wtk_string_t *v;
	int ret;
	int i;
	int vi;
	char bi;
	wtk_string_t **strs;
	char a,b,c;
	unsigned short id;
	int sil;

	heap=hash->heap;
	buf=wtk_strbuf_new(256,1);
	ret=wtk_source_fill(src,(char*)&vi,4);
	if(ret!=0){goto end;}
	//wtk_debug("vi=%d\n",vi);
	strs=(wtk_string_t**)wtk_heap_zalloc(heap,vi*sizeof(wtk_string_t*));
	sil=-1;
	for(i=0;i<vi;++i)
	{
		ret=wtk_source_fill(src,(char*)&bi,1);
		if(ret!=0){goto end;}
		//wtk_debug("bi=%d\n",bi);
		ret=wtk_source_fill(src,buf->data,bi);
		if(ret!=0){goto end;}
		//print_data(buf->data,bi);
		strs[i]=wtk_heap_dup_string(heap,buf->data,bi);
		if(wtk_str_equal_s(buf->data,bi,"sil"))
		{
			sil=i;
		}
	}
	ret=wtk_source_fill(src,(char*)&vi,4);
	if(ret!=0){goto end;}
	//wtk_debug("vi=%d\n",vi);
	for(i=0;i<vi;++i)
	{
		ret=wtk_source_fill(src,buf->data,5);
		if(ret!=0){goto end;}
		a=buf->data[0];
		b=buf->data[1];
		c=buf->data[2];
		id=*((unsigned short*)(buf->data+3));
		//wtk_debug("%d,%d,%d,%d\n",a,b,c,id);
		if(a==b && b==c && a==sil)
		{
			v=strs[(int)a];
			//wtk_debug("[%.*s]=%d\n",v->len,v->data,id);
			wtk_str_hash_add_s(hash,"sil",(void*)(long)id);
		}else
		{
			wtk_strbuf_reset(buf);
			v=strs[(int)a];
			wtk_strbuf_push(buf,v->data,v->len);
			wtk_strbuf_push_c(buf,'-');
			v=strs[(int)b];
			wtk_strbuf_push(buf,v->data,v->len);
			wtk_strbuf_push_c(buf,'+');
			v=strs[(int)c];
			wtk_strbuf_push(buf,v->data,v->len);
			v=wtk_heap_dup_string(heap,buf->data,buf->pos);
			//wtk_debug("[%.*s]=%d\n",v->len,v->data,id);
			wtk_str_hash_add(hash,v->data,v->len,(void*)(long)id);
		}
	}
	ret=0;
end:
	wtk_strbuf_delete(buf);
	return ret;
}

int wtk_e2fst_cfg_load_phn_id(wtk_e2fst_cfg_t *cfg,wtk_source_t *src)
{
	wtk_strbuf_t *buf;
	int ret;
	wtk_string_t *k;
	wtk_heap_t *heap=cfg->heap;//cfg->net.label->heap;
	int v;
	wtk_string_t **strs;

	strs=wtk_calloc(255,sizeof(wtk_string_t*));
	buf=wtk_strbuf_new(256,1);
	cfg->phn_ids=strs;
	while(1)
	{
		ret=wtk_source_read_string(src,buf);
		if(ret!=0){ret=0;goto end;}
		//wtk_debug("[%.*s]\n",buf->pos,buf->data);
		k=wtk_heap_dup_string(heap,buf->data,buf->pos);
		ret=wtk_source_read_string(src,buf);
		if(ret!=0){goto end;}
		//wtk_debug("[%.*s]:[%.*s]\n",k->len,k->data,buf->pos,buf->data);
		v=wtk_str_atoi(buf->data,buf->pos);
		strs[v]=k;
	}
end:
	//exit(0);
	wtk_strbuf_delete(buf);
	return 0;
}

//int wtk_e2fst_cfg_load_hmm_map(wtk_e2fst_cfg_t *cfg,wtk_source_t *src)
//{
//	wtk_strbuf_t *buf;
//	int ret,v,cnt;
//	int* tmp;
//
//	tmp=(int*)wtk_malloc(sizeof(int)*35007);
//	cnt=0;
//	buf=wtk_strbuf_new(256,1);
//	while(1)
//	{
//		ret=wtk_source_read_string(src,buf);
//		if(ret!=0){break;}
//		v=wtk_str_atoi(buf->data,buf->pos);
//		*(tmp+cnt)=v;
//		cnt++;
//	}
//	cfg->hmm_map=(int*)wtk_malloc(sizeof(int)*cnt);
//	memcpy(cfg->hmm_map,tmp,sizeof(int)*cnt);
//	//exit(0);
//	wtk_free(tmp);
//	wtk_strbuf_delete(buf);
//	return 0;
//}

int wtk_e2fst_cfg_load_hmm_map(wtk_e2fst_cfg_t *cfg,wtk_source_t *src)
{
	wtk_strbuf_t *buf;
	int ret,v,cnt,i,j,n;
	int bin=0;
	wtk_e2fst_hmm_expand_t *hmm_exp;
	wtk_e2fst_hmm_expand_pdf_t *hmm_pdf;
	float fv;

	cnt=0;
	buf=wtk_strbuf_new(256,1);

	ret=wtk_source_read_int(src,&cnt,1,bin);
	cfg->hmm_maps=(wtk_e2fst_hmm_expand_t**)wtk_calloc(cnt,sizeof(wtk_e2fst_hmm_expand_t*));

	for(i=0;i<cnt;i++)
	{
		hmm_exp=(wtk_e2fst_hmm_expand_t*)wtk_heap_malloc(cfg->heap,sizeof(wtk_e2fst_hmm_expand_t));
		cfg->hmm_maps[i]=hmm_exp;
		ret=wtk_source_read_string(src,buf);
		if(ret!=0){break;}
		//wtk_debug("%.*s\n",buf->pos,buf->data);
		if(wtk_str_equal_s(buf->data, buf->pos, "sil") || wtk_str_equal_s(buf->data, buf->pos, "sil_S"))
		{
			ret=wtk_source_read_int(src,&v,1,bin);
			hmm_exp->num_pdfs=1;
			hmm_exp->pdf=(wtk_e2fst_hmm_expand_pdf_t*)wtk_heap_malloc(cfg->heap,sizeof(wtk_e2fst_hmm_expand_pdf_t));
			n=(v-1)*4+2;

			//hmm_exp->pdf->id=(int*)wtk_malloc(sizeof(int)*n);
			hmm_exp->pdf->id=(int*)wtk_heap_malloc(cfg->heap, sizeof(int)*n);
			wtk_source_read_int(src,hmm_exp->pdf->id,n,bin);
			//hmm_exp->pdf->weight=(float*)wtk_malloc(sizeof(float)*n);
			hmm_exp->pdf->weight=(float*)wtk_heap_malloc(cfg->heap, sizeof(float)*n);
			hmm_exp->pdf->forward_id=-1;
			wtk_source_read_float(src,hmm_exp->pdf->weight,n,bin);
//			for(j=0;j<n;j++)
//			{
//				*(hmm_exp->pdf->weight+j)=-log(*(hmm_exp->pdf->weight+j));
//			}
			continue;
		}
		ret=wtk_source_read_int(src,&v,1,bin);
		//wtk_debug("%d\n",i);
		hmm_exp->num_pdfs=v;
		hmm_exp->pdf=(wtk_e2fst_hmm_expand_pdf_t*)wtk_heap_malloc(cfg->heap,sizeof(wtk_e2fst_hmm_expand_pdf_t)*v);

		for(j=0;j<v;j++)
		{
			hmm_pdf=hmm_exp->pdf+j;
			hmm_pdf->id=NULL;
			hmm_pdf->weight=NULL;
			wtk_source_read_int(src,&(hmm_pdf->selfloop_id),1,bin);
			wtk_source_read_int(src,&(hmm_pdf->forward_id),1,bin);
			wtk_source_read_float(src,&(fv),1,bin);
//			hmm_pdf->selfloop_weight=-log(fv);
			hmm_pdf->selfloop_weight=fv;
			wtk_source_read_float(src,&(fv),1,bin);
//			hmm_pdf->forward_weight=-log(fv);
			hmm_pdf->forward_weight=fv;
			//wtk_debug("%d %d\n",hmm_pdf->selfloop_id,hmm_pdf->forward_id);
		}
	}
	//exit(0);
	wtk_strbuf_delete(buf);
	return 0;
}

int wtk_e2fst_cfg_load_hmm_map_chain(wtk_e2fst_cfg_t *cfg,wtk_source_t *src)
{
	wtk_strbuf_t *buf;
	int ret,v,cnt,i,j;
	int bin=0;
	wtk_e2fst_hmm_expand_t *hmm_exp;
	wtk_e2fst_hmm_expand_pdf_t *hmm_pdf;

	cnt=0;
	buf=wtk_strbuf_new(256,1);

	ret=wtk_source_read_int(src,&cnt,1,bin);
	cfg->hmm_maps=(wtk_e2fst_hmm_expand_t**)wtk_calloc(cnt,sizeof(wtk_e2fst_hmm_expand_t*));

	for(i=0;i<cnt;i++)
	{
		hmm_exp=(wtk_e2fst_hmm_expand_t*)wtk_heap_malloc(cfg->heap,sizeof(wtk_e2fst_hmm_expand_t));
		cfg->hmm_maps[i]=hmm_exp;
		ret=wtk_source_read_string(src,buf);
		if(ret!=0){break;}
		ret=wtk_source_read_int(src,&v,1,bin);
		//wtk_debug("%d\n",i);
		hmm_exp->num_pdfs=v;
		hmm_exp->pdf=(wtk_e2fst_hmm_expand_pdf_t*)wtk_heap_malloc(cfg->heap,sizeof(wtk_e2fst_hmm_expand_pdf_t)*v);

		for(j=0;j<v;j++)
		{
			hmm_pdf=hmm_exp->pdf+j;
			wtk_source_read_int(src,&(hmm_pdf->selfloop_id),1,bin);
			wtk_source_read_int(src,&(hmm_pdf->forward_id),1,bin);
			wtk_source_read_float(src,&(hmm_pdf->selfloop_weight),1,bin);
			wtk_source_read_float(src,&(hmm_pdf->forward_weight),1,bin);
			//wtk_debug("%d %d\n",hmm_pdf->selfloop_id,hmm_pdf->forward_id);
			//wtk_debug("%f %f\n",hmm_pdf->selfloop_weight,hmm_pdf->forward_weight);
		}
	}
	//exit(0);
	wtk_strbuf_delete(buf);
	return 0;
}

int wtk_e2fst_cfg_update2(wtk_e2fst_cfg_t *cfg,wtk_label_t *label,wtk_source_loader_t *sl)
{
	int ret;
	//int wtk_fst_net_cfg_update3(wtk_fst_net_cfg_t *cfg,wtk_label_t *label,wtk_source_loader_t *sl)
	ret=wtk_fst_net_cfg_update3(&(cfg->net),label,sl);
        ret = wtk_fst_net_cfg_update3(&(cfg->filler_net), label, sl);
        if(ret!=0){goto end;}
	if(cfg->phn_id_fn)
	{
		ret=wtk_source_load_file(cfg,(wtk_source_load_handler_t)wtk_e2fst_cfg_load_phn_id,cfg->phn_id_fn);
		if(ret!=0){goto end;}
	}
	if(cfg->sym_out_fn)
	{
		cfg->sym_out=wtk_fst_insym_new2(NULL,cfg->sym_out_fn,1,sl);
	}

	if(cfg->hmm_map_fn)
	{
		if(cfg->use_chain)
			ret=wtk_source_loader_load(sl,cfg,(wtk_source_load_handler_t)wtk_e2fst_cfg_load_hmm_map_chain,cfg->hmm_map_fn);
		else
			ret=wtk_source_loader_load(sl,cfg,(wtk_source_load_handler_t)wtk_e2fst_cfg_load_hmm_map,cfg->hmm_map_fn);
		if(ret!=0){goto end;}
	}

	ret=0;
end:
	return ret;
}
