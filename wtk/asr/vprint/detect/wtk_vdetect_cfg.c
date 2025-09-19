#include "wtk_vdetect_cfg.h" 

int wtk_vdetect_cfg_init(wtk_vdetect_cfg_t *cfg)
{
	cfg->ubin=0;
	cfg->hmmset=NULL;
	cfg->heap=wtk_heap_new(4096);
	wtk_queue_init(&(cfg->usr_q));
	wtk_string_set(&(cfg->usr_dn),0,0);
	cfg->thresh=0;
	cfg->skip_frame=0;
	cfg->load=1;
	cfg->use_ubin=0;
	return 0;
}

int wtk_vdetect_cfg_clean(wtk_vdetect_cfg_t *cfg)
{
	if(cfg->ubin)
	{
		wtk_ubin_write_all(cfg->ubin,cfg->ubin->fn);
		wtk_ubin_delete(cfg->ubin);
	}
	wtk_heap_delete(cfg->heap);
	return 0;
}

void wtk_vdetect_cfg_reset_usr(wtk_vdetect_cfg_t* cfg)
{
	wtk_queue_node_t *qn;
	wtk_vdetect_usr_t *usr;

	for(qn=cfg->usr_q.pop;qn;qn=qn->next)
	{
		usr=data_offset2(qn,wtk_vdetect_usr_t,q_n);
		usr->prob=0;
		usr->llr=-1000.0;
	}
}

wtk_vdetect_usr_t* wtk_vdetect_cfg_find_usr(wtk_vdetect_cfg_t *cfg,char *name,int bytes)
{
	wtk_queue_node_t *qn;
	wtk_vdetect_usr_t *usr,*ret;

	ret=NULL;
	for(qn=cfg->usr_q.pop;qn;qn=qn->next)
	{
		usr=data_offset2(qn,wtk_vdetect_usr_t,q_n);
		if(wtk_string_cmp(&(usr->name),name,bytes)==0)
		{
			ret=usr;
			break;
		}
	}
	return ret;
}


int wtk_vdetect_cfg_load_usr(wtk_vdetect_cfg_t *cfg,wtk_vdetect_usr_t *usr)
{
	wtk_source_t s;
	wtk_heap_t *heap=cfg->heap;
	wtk_hmm_t *hmm;
	wtk_state_t *state;
	int ret;
	int i,nmix,vsize;
	wtk_mixpdf_t *pdf;
	float f;
	wtk_source_file_item_t *src_item;
	wtk_ubin_item_t *ubin_item;
	int pos;

	if(cfg->ubin)
	{
		ubin_item=(wtk_ubin_item_t*)(usr->hmm_fn);
		ret=wtk_source_init_file(&(s),cfg->ubin->fn);
		if(ret!=0){goto end;}
		src_item=s.data;
		pos=ubin_item->seek_pos+4+ubin_item->fn->len+1;
		fseek(src_item->f,pos,SEEK_SET);
	}else
	{
		ret=wtk_source_init_file(&(s),usr->hmm_fn);
		if(ret!=0){goto end;}
	}
	hmm=(wtk_hmm_t*)wtk_heap_malloc(heap,sizeof(wtk_hmm_t));
	hmm->num_state=3;
	hmm->name=NULL;
	hmm->tIdx=0;
	hmm->transP=NULL;
	hmm->pState=(wtk_state_t**)wtk_heap_malloc(heap,sizeof(wtk_state_t*));
	hmm->pState-=2;
	hmm->pState[2]=state=(wtk_state_t*)wtk_heap_malloc(heap,sizeof(wtk_state_t));
	state->name=NULL;
	state->dnn=NULL;
	state->pfStreamWeight=NULL;
	state->used=0;
	state->index=0;
	ret=wtk_source_fill(&(s),(char*)&nmix,4);
	if(ret!=0){goto end;}
	ret=wtk_source_fill(&(s),(char*)&vsize,4);
	if(ret!=0){goto end;}
	state->pStream=(wtk_stream_t*)wtk_heap_malloc(heap,sizeof(wtk_stream_t));
	state->pStream->nMixture=nmix;
	state->pStream->pmixture=(wtk_mixture_t*)wtk_heap_malloc(heap,sizeof(wtk_mixture_t)*nmix);
	for(i=0;i<nmix;++i)
	{
		ret=wtk_source_fill(&(s),(char*)&f,4);
		if(ret!=0){goto end;}
		state->pStream->pmixture[i].fWeight=f;
		state->pStream->pmixture[i].pdf=pdf=(wtk_mixpdf_t*)wtk_heap_malloc(heap,sizeof(wtk_mixpdf_t));
		pdf->index=i;
		pdf->used=0;
		pdf->mean=wtk_svector_newh(heap,vsize);
		pdf->variance=wtk_svector_newh(heap,vsize);
		ret=wtk_source_fill(&(s),(char*)(pdf->mean+1),vsize*4);
		if(ret!=0)
		{
			wtk_debug("load mean failed[%d/%d/%d]\n",i,nmix,vsize);
			goto end;
		}
		ret=wtk_source_fill(&(s),(char*)(pdf->variance+1),vsize*4);
		if(ret!=0)
		{
			wtk_debug("load var failed\n");
			goto end;
		}
		ret=wtk_source_fill(&(s),(char*)(&(pdf->fGconst)),4);
		if(ret!=0)
		{
			wtk_debug("load gconst failed\n");
			goto end;
		}
		if(cfg->hmmset && cfg->hmmset->use_fix)
		{
			wtk_vector_fix_scale(pdf->variance,cfg->hmmset->var_scale);//set->cfg->var_scale);
			wtk_vector_fix_scale(pdf->mean,cfg->hmmset->mean_scale);//set->cfg->mean_scale);
		}
		//wtk_mixpdf_print(pdf);
	}
	usr->hmm=hmm;
	wtk_source_clean_file(&(s));
	ret=0;
end:
	return ret;
}

int wtk_vdetect_cfg_update_usr(wtk_vdetect_cfg_t *cfg,wtk_local_cfg_t *lc)
{
	wtk_queue_node_t *qn;
	wtk_cfg_item_t *item;
	wtk_heap_t *heap=cfg->heap;
	wtk_vdetect_usr_t *usr;
	wtk_local_cfg_t *lc2;
	wtk_string_t *v;
	int ret;

	//wtk_local_cfg_print(lc);
	for(qn=lc->cfg->queue.pop;qn;qn=qn->next)
	{
		item=data_offset2(qn,wtk_cfg_item_t,n);
		if(item->type==WTK_CFG_LC)
		{
			usr=(wtk_vdetect_usr_t*)wtk_heap_malloc(heap,sizeof(wtk_vdetect_usr_t));
			lc2=item->value.cfg;
			wtk_local_cfg_update_cfg_string_v(lc2,usr,name,v);
			wtk_local_cfg_update_cfg_str(lc2,usr,hmm_fn,v);
			if(usr->name.len<=0)
			{
				usr->name=*(item->key);
			}
			usr->hmm=NULL;
			usr->prob=0;
			ret=wtk_vdetect_cfg_load_usr(cfg,usr);
			if(ret!=0)
			{
				wtk_debug("load %s failed\n",usr->hmm_fn);
				goto end;
			}
			//wtk_debug("[%.*s]\n",usr->name.len,usr->name.data);
			wtk_queue_push(&(cfg->usr_q),&(usr->q_n));
		}
	}
	ret=0;
end:
	return ret;
}

int wtk_vdetect_cfg_update_local(wtk_vdetect_cfg_t *cfg,wtk_local_cfg_t *main)
{
	wtk_local_cfg_t *lc;
	wtk_string_t *v;
	int ret;

	lc=main;
	wtk_local_cfg_update_cfg_b(lc,cfg,load,v);
	wtk_local_cfg_update_cfg_f(lc,cfg,thresh,v);
	wtk_local_cfg_update_cfg_i(lc,cfg,skip_frame,v);
	wtk_local_cfg_update_cfg_string_v(lc,cfg,usr_dn,v);
	//wtk_debug("[%p:%s]\n",cfg->usr_dn.data,cfg->usr_dn.data);
	wtk_local_cfg_update_cfg_b(lc,cfg,use_ubin,v);
	lc=wtk_local_cfg_find_lc_s(main,"usr");
	if(lc)
	{
		ret=wtk_vdetect_cfg_update_usr(cfg,lc);
		if(ret!=0){goto end;}
	}
	if(cfg->use_ubin)
	{
		cfg->ubin=wtk_ubin_new(20);
	}
	ret=0;
end:
	return ret;
}

int wtk_vdetect_cfg_load_usr_fn(wtk_vdetect_cfg_t *cfg,wtk_string_t *fn)
{
	wtk_string_t *v;
	wtk_heap_t *heap=cfg->heap;
	wtk_vdetect_usr_t *usr;
	int ret;

	v=wtk_str_right(fn->data,fn->len,'/');
	//wtk_debug("[%.*s]\n",v->len,v->data);

	usr=wtk_vdetect_cfg_find_usr(cfg,v->data,v->len-4);
	if(!usr)
	{
		usr=(wtk_vdetect_usr_t*)wtk_heap_malloc(heap,sizeof(wtk_vdetect_usr_t));
		usr->hmm=NULL;
		usr->prob=0;
		usr->attr=0;
		wtk_heap_fill_string(heap,&(usr->name),v->data,v->len-4);
		wtk_queue_push(&(cfg->usr_q),&(usr->q_n));
	}
	usr->hmm_fn=fn->data;
	//wtk_debug("[%.*s]\n",usr->name.len,usr->name.data);
	ret=wtk_vdetect_cfg_load_usr(cfg,usr);
	if(ret!=0){goto end;}
	ret=0;
end:
	wtk_string_delete(v);
	return ret;
}

int wtk_vdetect_cfg_load_usr_fn3(wtk_vdetect_cfg_t *cfg,wtk_string_t *fn)
{
	int ret=-1;
	wtk_heap_t *heap=cfg->heap;
	wtk_vdetect_usr_t *usr;
	wtk_ubin_item_t *item;

	usr=wtk_vdetect_cfg_find_usr(cfg,fn->data,fn->len);
	if(!usr)
	{
		if(cfg->ubin->item_num==0){goto end;}
		item=wtk_ubin_find_item(cfg->ubin,fn);
		if(!item){goto end;}
		usr=(wtk_vdetect_usr_t*)wtk_heap_malloc(heap,sizeof(wtk_vdetect_usr_t));
		usr->hmm=NULL;
		usr->prob=0;
		//usr->hmm_fn=fn->data;
		usr->hmm_fn=(char*)item;
		wtk_heap_fill_string(heap,&(usr->name),item->fn->data,item->fn->len);
		wtk_queue_push(&(cfg->usr_q),&(usr->q_n));
	}
	item=(wtk_ubin_item_t*)(usr->hmm_fn);
	usr->attr=item->attr;
	//wtk_debug("[%.*s]\n",usr->name.len,usr->name.data);
	ret=wtk_vdetect_cfg_load_usr(cfg,usr);
	if(ret!=0){goto end;}

	ret=0;

end:
	return ret;
}

int wtk_vdetect_cfg_load_usr_dn_fn(wtk_vdetect_cfg_t *cfg,wtk_string_t *fn)
{
#define SUF ".bin"
	int ret=0;

	if(cfg->ubin)
	{
		ret=wtk_vdetect_cfg_load_usr_fn3(cfg,fn);
	}else
	{
	//	wtk_debug("fn=[%.*s]\n",fn->len,fn->data);
		if(fn->len>sizeof(SUF) && strcmp(fn->data+fn->len-sizeof(SUF)+1,SUF)==0)
		{
			//wtk_debug("fn=[%.*s]\n",fn->len,fn->data);
			ret=wtk_vdetect_cfg_load_usr_fn(cfg,fn);
		}
	}
	return ret;
}

#ifdef WIN32
int wtk_vdetect_cfg_load_usr_dn_fn_win(wtk_vdetect_cfg_t *cfg, char *fn) {
        wtk_string_t path;
        path.len = strlen(fn);
        path.data = fn;
        return wtk_vdetect_cfg_load_usr_dn_fn(cfg, &path);
}
#endif

int wtk_vdetect_cfg_del_usr_fn2(wtk_vdetect_cfg_t *cfg,wtk_string_t *fn)
{
	wtk_vdetect_usr_t *usr;
	wtk_ubin_item_t *item;
	int ret;

	//wtk_debug("[%.*s]\n",v->len,v->data);
	item=wtk_ubin_find_item(cfg->ubin,fn);
	usr=wtk_vdetect_cfg_find_usr(cfg,fn->data,fn->len);
	if(usr && item)
	{
//		wtk_debug("delete %s\n",fn->data);
//		wtk_queue_remove(&(cfg->usr_q),&(usr->q_n));
		item->attr=item->attr |WTK_UBIN_ATTR_INVALID;
		usr->attr=item->attr;
	}
	ret=0;
	return ret;
}

int wtk_vdetect_cfg_del_usr_fn(wtk_vdetect_cfg_t *cfg,wtk_string_t *fn)
{
	wtk_string_t *v;
	wtk_vdetect_usr_t *usr;
	int ret;

	v=wtk_str_right(fn->data,fn->len,'/');
	//wtk_debug("[%.*s]\n",v->len,v->data);

	usr=wtk_vdetect_cfg_find_usr(cfg,v->data,v->len-4);
	if(usr)
	{
		wtk_debug("delete %s\n",fn->data);
		remove(fn->data);
		wtk_queue_remove(&(cfg->usr_q),&(usr->q_n));
	}
	ret=0;
	wtk_string_delete(v);
	return ret;
}

int wtk_vdetect_cfg_del_usr_dn_fn(wtk_vdetect_cfg_t *cfg,wtk_string_t *fn)
{
#define SUF ".bin"
	int ret=0;

	if(cfg->ubin)
	{
		wtk_vdetect_cfg_del_usr_fn2(cfg,fn);
	}else
	{
		if(fn->len>sizeof(SUF) && strcmp(fn->data+fn->len-sizeof(SUF)+1,SUF)==0)
		{
			//wtk_debug("fn=[%.*s]\n",fn->len,fn->data);
			ret=wtk_vdetect_cfg_del_usr_fn(cfg,fn);
		}
	}
	return ret;
}


int wtk_vdetect_cfg_load_usr_dn(wtk_vdetect_cfg_t *cfg,char *dn)
{
	//wtk_debug("%s:%p\n",dn,dn);
#if defined __ANDROID__
	wtk_dir_walk(dn,cfg,(wtk_dir_walk_f)wtk_vdetect_cfg_load_usr_dn_fn);
#elif defined __APPLE__ || defined __IPHONE_OS__
#elif defined WIN32
    wtk_dir_walk(dn, cfg, (wtk_dir_walk_handler_t)wtk_vdetect_cfg_load_usr_dn_fn_win);
#else
	wtk_dir_walk(dn,cfg,(wtk_dir_walk_f)wtk_vdetect_cfg_load_usr_dn_fn);
#endif
	return 0;
}

int wtk_vdetect_cfg_update(wtk_vdetect_cfg_t *cfg)
{
	int ret;

	if(cfg->load && cfg->usr_dn.data)
	{
		ret=wtk_vdetect_cfg_load_usr_dn(cfg,cfg->usr_dn.data);
		if(ret!=0){goto end;}
	}
	ret=0;
end:
	return ret;
}

int wtk_vdetect_cfg_load_usr_fn2(wtk_vdetect_cfg_t *cfg,char *fn)
{
	int ret=-1;
	wtk_heap_t *heap=cfg->heap;
	wtk_vdetect_usr_t *usr;
	wtk_queue_t **s,**e,*q;
	wtk_queue_node_t *qn;
	wtk_ubin_item_t *item;
	hash_str_node_t *str_node;

	ret=wtk_ubin_read(cfg->ubin,fn,0);
	if(ret==-1){goto end;}
	if(cfg->ubin->item_num==0){goto end;}

	s=(cfg->ubin->hash->slot-1);
	e=s+cfg->ubin->hash->nslot+1;
	while(++s<e)
	{
		q=*s;
		if(!q){continue;}
		for(qn=q->pop;qn;qn=qn->next)
		{
			str_node=wtk_queue_node_data(qn,hash_str_node_t,n);
			item=str_node->value;
			usr=wtk_vdetect_cfg_find_usr(cfg,item->fn->data,item->fn->len);
			if(!usr)
			{
				usr=(wtk_vdetect_usr_t*)wtk_heap_malloc(heap,sizeof(wtk_vdetect_usr_t));
				usr->hmm=NULL;
				usr->prob=0;
				wtk_heap_fill_string(heap,&(usr->name),item->fn->data,item->fn->len);
				wtk_queue_push(&(cfg->usr_q),&(usr->q_n));
				//usr->hmm_fn=fn->data;
				usr->hmm_fn=(char*)item;
			}
			usr->attr=item->attr;
			ret=wtk_vdetect_cfg_load_usr(cfg,usr);
			if(ret!=0){goto end;}
		}
	}
	ret=0;

end:
	return ret;
}

int wtk_vdetect_cfg_update2(wtk_vdetect_cfg_t *cfg,wtk_source_loader_t *sl)
{
	int ret=0;
	wtk_rbin2_t *rb=(wtk_rbin2_t*)(sl->hook);
	wtk_string_t *rec_dir=0;
	wtk_string_t *usr_dir=0;
	wtk_string_t *usr_name=0;
	wtk_strbuf_t *buf;

	if(!cfg->ubin)
	{
		return wtk_vdetect_cfg_update(cfg);
	}
	buf=wtk_strbuf_new(64,0);
//	printf("%s %s\n",cfg->usr_dn.data,rb->fn);
	rec_dir=wtk_dirname(rb->fn,'/');
//	printf("%.*s\n",rec_dir->len,rec_dir->data);
	usr_dir=wtk_dirname(cfg->usr_dn.data,'/');
//	printf("%.*s\n",usr_dir->len,usr_dir->data);
	usr_name=wtk_basename(cfg->usr_dn.data,'/');
//	printf("%.*s\n",usr_name->len,usr_name->data);
	if(cfg->usr_dn.data[0]!='/'){
		wtk_strbuf_push(buf,rec_dir->data,rec_dir->len);
		wtk_strbuf_push_c(buf,'/');
	}
	wtk_strbuf_push(buf,usr_dir->data,usr_dir->len);
	wtk_strbuf_push_c(buf,'/');
	wtk_strbuf_push(buf,usr_name->data,usr_name->len);
	wtk_strbuf_push(buf,".bin",sizeof(".bin"));
	wtk_strbuf_push_c(buf,'\0');
//	printf("%.*s %s\n",buf->pos,buf->data,cfg->usr_dn.data);

	if(cfg->load && cfg->usr_dn.data)
	{
		wtk_vdetect_cfg_load_usr_fn2(cfg,buf->data);
	}

	wtk_strbuf_delete(buf);
	if(rec_dir){wtk_free(rec_dir);}
	if(usr_dir){wtk_free(usr_dir);}
	if(usr_name){wtk_free(usr_name);}
	return ret;
}
