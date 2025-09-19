#include <ctype.h>
#include <math.h>
#include "wtk_nglm.h"

int wtk_nglm_load_uni_gram(wtk_nglm_t *nglm)
{
	wtk_lm_node_t *node;
	int *pi;
	wtk_string_t v;
	int ret;
	char *s,*e;
	int i;
	float *pf;
	int nx;

	ret=wtk_fbin_get(nglm->fbin,0,4,&v);
	if(ret!=0){goto end;}
	//wtk_debug("[%.*s]\n",v.len,v.data);
	if(wtk_string_cmp_s(&(v),"Y==A")==0)
	{
        ret=wtk_fbin_get(nglm->fbin,4,4,&v);
        nglm->type=WTK_LM2BIN_A;
		nglm->prob_scale=1.0/10000.0;
		nglm->bow_scale=1.0/10000.0;
	}else if(wtk_string_cmp_s(&(v),"Y==a")==0)
	{
		nglm->type=WTK_LM2BIN_a;
		ret=wtk_fbin_get(nglm->fbin,4,8,&v);
		if(ret!=0){goto end;}
		//print_data(v.data,v.len);
		pf=(float*)v.data;
		nglm->prob_scale=1.0/pf[0];
		nglm->bow_scale=1.0/pf[1];
		//wtk_debug("%f/%f=[%f/%f]\n",pf[0],pf[1],nglm->prob_scale,nglm->bow_scale);
		//exit(0);
		ret=wtk_fbin_get(nglm->fbin,12,4,&v);
		if(ret!=0){goto end;}
	}else if(wtk_string_cmp_s(&(v),"Y==B")==0)
	{
		nglm->type=WTK_LM2BIN_B;
		ret=wtk_fbin_get(nglm->fbin,4,8,&v);
		if(ret!=0){goto end;}
		//print_data(v.data,v.len);
		pf=(float*)v.data;
		nglm->prob_scale=1.0/pf[0];
		nglm->bow_scale=1.0/pf[1];
		//wtk_debug("%f/%f=[%f/%f]\n",pf[0],pf[1],nglm->prob_scale,nglm->bow_scale);
		ret=wtk_fbin_get(nglm->fbin,12,4,&v);
		if(ret!=0){goto end;}
	}
	else if (wtk_string_cmp_s(&(v),"Y==C")==0)
	{
		nglm->type=WTK_LM2BIN_C;
		ret=wtk_fbin_get(nglm->fbin,4,8,&v);
		if(ret!=0){goto end;}
		//print_data(v.data,v.len);
		pf=(float*)v.data;
		nglm->prob_scale=1.0/pf[0];
		nglm->bow_scale=1.0/pf[1];
		//wtk_debug("%f/%f=[%f/%f]\n",pf[0],pf[1],nglm->prob_scale,nglm->bow_scale);
		ret=wtk_fbin_get(nglm->fbin,12,4,&v);
		if(ret!=0){goto end;}
	}
	//exit(0);
	if(ret!=0){goto end;}
	pi=(int*)v.data;
	nglm->n_uni_nodes=*pi;
	nglm->uni_nodes=(wtk_lm_node_t*)wtk_calloc(nglm->n_uni_nodes,sizeof(wtk_lm_node_t));
	nx=wtk_lm2bin_type_bytes(nglm->type);
	//wtk_debug("nx=%d %d\n",nx,nglm->n_uni_nodes);
	switch(nglm->type)
	{
	case WTK_LM2BIN_A:
		ret=wtk_fbin_get(nglm->fbin,8,nglm->n_uni_nodes*nx,&v);
		break;
	case WTK_LM2BIN_a:
		ret=wtk_fbin_get(nglm->fbin,16,nglm->n_uni_nodes*nx,&v);
		break;
	case WTK_LM2BIN_B:
		ret=wtk_fbin_get(nglm->fbin,16,nglm->n_uni_nodes*nx,&v);
		break;
	case WTK_LM2BIN_C:
		ret=wtk_fbin_get(nglm->fbin,16,nglm->n_uni_nodes*nx,&v);
		break;
	}
	if(ret!=0)
	{
		wtk_debug("load unigram failed[%d]\n",nglm->n_uni_nodes);
		goto end;
	}
	s=v.data;e=s+v.len;
	i=0;
	while(s<e)
	{
		node=nglm->uni_nodes+i;
		wtk_lm_node_init(node);
		node->ngram=1;
		//node->self_offset=8+i*12;
		wtk_lm2bin_from_bin(nglm->type,node,s,nglm->prob_scale,nglm->bow_scale);
		//wtk_debug("v[%d]=%d\n",i,node->id);
//		if(i!=node->id-1)
//		{
//			wtk_debug("v[%d]=%d\n",i,node->id);
//			print_data(s,nx);
//			exit(0);
//		}
		//wtk_lm_node_print(node);
		++i;
		//wtk_debug("v[%d]=%d/%d len=%d/%d\n",i,node->id,v.len/12,nglm->n_uni_nodes*12,v.len);
		s+=nx;
	}
	ret=0;
end:
	//exit(0);
	return ret;
}




void wtk_nglm_print_lm(wtk_nglm_t *lm,wtk_lm_node_t *node)
{
	wtk_strbuf_t *buf=lm->buf;

	wtk_strbuf_reset(buf);
	wtk_lm_node_tostring(lm->dict_cfg->sym,node,buf);
	wtk_debug("%f %.*s %f\n",node->prob,buf->pos,buf->data,node->bow);
}

void wtk_nglm_print_child(wtk_nglm_t *lm,wtk_lm_node_t *n)
{
	wtk_string_t *v;
	int i;

	v=lm->dict_cfg->sym->ids[n->id]->str;
	for(i=1;i<n->ngram;++i)
	{
		printf("  ");
	}
	printf("%f %.*s %f [%d,child=%d,p=%p]\n",n->prob,v->len,v->data,n->bow,n->ngram,
			(int)n->child_offset,n);
	for(i=0;i<n->nchild;++i)
	{
		wtk_nglm_print_child(lm,n->childs[i]);
	}
}

void wtk_nglm_print_node(wtk_nglm_t *lm,wtk_lm_node_t *n)
{
	wtk_strbuf_t *buf=lm->buf;
	//wtk_string_t *v;
	int i;

	//v=lm->cfg->sym->ids[n->id]->str;
	for(i=1;i<n->ngram;++i)
	{
		printf("  ");
	}
	wtk_lm_node_tostring(lm->dict_cfg->sym,n,buf);
	printf("%f %.*s %f [%d,child=%d,p=%p]\n",n->prob,buf->pos,buf->data,n->bow,
			n->ngram,(int)n->child_offset,n);
}

int wtk_nglm_load_node(wtk_nglm_t *nglm,wtk_lm_node_t *node,int reverse,wtk_heap_t *heap)
{
	wtk_lm_node_t *n2;
	wtk_string_t v;
	int *pi;
	int ret;
	int is_max_order;
	int i,n;
	char *s,*e;

	//wtk_debug("type=%d\n",nglm->type);
	//wtk_nglm_print_child(nglm,node);
	if(node->ngram==1 || node->parent==nglm->s_node)
	{
		wtk_larray_push2(nglm->valid_uni_node,&(node));
	}
	if(node->child_offset<=0)
	{
		return 0;
		if(node->id==nglm->dict_cfg->snt_end_id || node->ngram==nglm->cfg->max_order)
		{
			return 0;
		}
	}
	//wtk_debug("ngram=%d node=%d %d offset=%d\n",node->ngram,node->id,node->parent?node->parent->id:-1,(int)(node->child_offset));
	is_max_order=node->ngram==(nglm->cfg->max_order-1);
	ret=wtk_fbin_get(nglm->fbin,node->child_offset,4,&v);
	if(ret!=0){goto end;}
	++nglm->cnt;
	pi=(int*)v.data;
	//wtk_debug("pi=%d\n",*pi);
	switch(nglm->type)
	{
	case WTK_LM2BIN_a:
		n=is_max_order?5:12;
		break;
	case WTK_LM2BIN_A:
		n=is_max_order?5:12;
		break;
	case WTK_LM2BIN_B:
		n=is_max_order?4:9;
		break;
	case WTK_LM2BIN_C:
		n=is_max_order?4:10;
		break;
	default:
		return -1;
	}
	node->nchild=*pi;
//	wtk_debug("pi=%d\n",*pi);
//	wtk_debug("off=%#x %d\n",(int)node->child_offset,*pi*n);
//	exit(0);
	ret=wtk_fbin_get(nglm->fbin,node->child_offset+4,*pi*n,&v);
	if(ret!=0){goto end;}
	nglm->bytes+=4+v.len;
	//wtk_debug("of[%d]=%ld\n",ki,node->child_offset+4);
	//print_hex(v.data,v.len);
	i=0;
	node->childs=(wtk_lm_node_t**)wtk_heap_malloc(heap,sizeof(wtk_lm_node_t*)*(node->nchild));
	s=v.data;e=s+v.len;
	if(nglm->cfg->load_all)
	{
		nglm->n_order[node->ngram]+=node->nchild;
	}
	while(s<e)
	{
		n2=(wtk_lm_node_t*)wtk_heap_malloc(heap,sizeof(wtk_lm_node_t));
		node->childs[i]=n2;
		wtk_lm_node_init(n2);
		n2->ngram=node->ngram+1;
		n2->parent=node;
		if(is_max_order)
		{
			wtk_lm2bin_from_bin_end(nglm->type,n2,s,nglm->prob_scale);
		}else
		{
			wtk_lm2bin_from_bin(nglm->type,n2,s,nglm->prob_scale,nglm->bow_scale);
		}
		//wtk_lm_node_print(node);
		++i;
		//wtk_debug("v[%d]=%d/%d len=%d/%d\n",i,node->id,v.len/12,nglm->n_uni_nodes*12,v.len);
		s+=n;
	}
	if(reverse && (node->ngram!=nglm->cfg->max_order-1))
	{
		for(i=0;i<node->nchild;++i)
		{
			n2=node->childs[i];
			ret=wtk_nglm_load_node(nglm,n2,reverse,heap);
			if(ret!=0){goto end;}
		}
	}
	ret=0;
end:
	//wtk_debug("ret=%d\n",ret);
	return ret;
}

int wtk_nglm_load_all_node(wtk_nglm_t *nglm)
{
	wtk_lm_node_t *node;
	int i;
	int ret;

	if(nglm->cfg->load_all)
	{
		nglm->n_order[0]=nglm->n_uni_nodes-1;
	}
	for(i=0;i<nglm->n_uni_nodes;++i)
	{
		node=nglm->uni_nodes+i;
		if(node->nchild==0)
		{
			ret=wtk_nglm_load_node(nglm,node,1,nglm->glb_heap);
			if(ret!=0){goto end;}
		}
	}
	ret=0;
end:
	return ret;
}

wtk_nglm_t* wtk_nglm_new(wtk_nglm_cfg_t *cfg,wtk_lm_dict_cfg_t *dict_cfg)
{
	wtk_nglm_t *n;

	n=(wtk_nglm_t*)wtk_malloc(sizeof(wtk_nglm_t));
	n->type=WTK_LM2BIN_A;
	n->cfg=cfg;
	n->dict_cfg=dict_cfg;
	if(cfg->rbin)
	{
		wtk_rbin2_item_t* item;

		item=wtk_rbin2_get(cfg->rbin,cfg->lm_fn,strlen(cfg->lm_fn));
		n->fbin=wtk_fbin_new(cfg->rbin->fn);
		n->fbin->of=item->pos;
	}else
	{
		n->fbin=wtk_fbin_new(cfg->lm_fn);
	}
	n->glb_heap=wtk_heap_new(4096);
	n->loc_heap=wtk_heap_new(4096);
	n->n_uni_nodes=0;
	n->uni_nodes=NULL;
	n->buf=wtk_strbuf_new(256,1);
	n->valid_uni_node=wtk_larray_new(1024,sizeof(wtk_lm_node_t*));
	wtk_nglm_load_uni_gram(n);
	if(cfg->load_all)
	{
		int i;
		for(i=0;i<4;++i)
		{
			n->n_order[i]=0;
		}
		wtk_debug("load all node\n");
		wtk_nglm_load_all_node(n);
	}
	//wtk_debug("snt=%d\n",dict_cfg->snt_start_id);
	n->s_node=wtk_nglm_get_uni_node(n,dict_cfg->snt_start_id);
	if(n->s_node->nchild==0)
	{
		wtk_nglm_load_node(n,n->s_node,0,n->glb_heap);
	}
	n->cnt=0;
	n->bytes=0;
	return n;
}

void wtk_nglm_delete(wtk_nglm_t *lm)
{
	wtk_larray_delete(lm->valid_uni_node);
	wtk_strbuf_delete(lm->buf);
	if(lm->uni_nodes)
	{
		wtk_free(lm->uni_nodes);
	}
	wtk_fbin_delete(lm->fbin);
	wtk_heap_delete(lm->loc_heap);
	wtk_heap_delete(lm->glb_heap);
	wtk_free(lm);
}

void wtk_nglm_reset(wtk_nglm_t *lm)
{
	wtk_lm_node_t **nodes,*n;
	int i;

	if(lm->cfg->load_all)
	{
		return;
	}
	if(lm->cfg->use_dynamic_reset)
	{
		//wtk_debug("==============> bytes=%d/%f\n",lm->bytes,lm->cfg->reset_max_bytes);
		if(lm->cfg->reset_max_bytes<0 || lm->bytes<lm->cfg->reset_max_bytes)
		{
			return;
		}
	}
	lm->cnt=0;
	lm->bytes=0;
	nodes=(wtk_lm_node_t**)lm->valid_uni_node->slot;
	for(i=0;i<lm->valid_uni_node->nslot;++i)
	{
		n=nodes[i];
		n->nchild=0;
		n->childs=NULL;
	}
	wtk_larray_reset(lm->valid_uni_node);
	wtk_heap_reset(lm->loc_heap);
}

int wtk_nglm_bytes(wtk_nglm_t *lm)
{
	int bytes=0;

	bytes+=wtk_heap_bytes(lm->glb_heap);
	bytes+=wtk_heap_bytes(lm->loc_heap);
	bytes+=wtk_larray_bytes(lm->valid_uni_node);
	return bytes;
}

wtk_lm_node_t* wtk_nglm_get_uni_node(wtk_nglm_t *lm,unsigned int id)
{
	wtk_lm_node_t *n;

	n=(lm->uni_nodes+id-1);
	//wtk_debug("%ld/%ld %f\n",n->child_offset,n->self_offset,n->prob);
	/*
	if(id!=lm->dict_cfg->snt_end_id && n->child_offset==0)
	{
		return NULL;
	}*/
	return n;
}

void wtk_nglm_touch_node(wtk_nglm_t *lm,wtk_lm_node_t *node)
{
	if(node->nchild==0)
	{
		wtk_nglm_load_node(lm,node,0,lm->loc_heap);
	}
}

wtk_lm_node_t* wtk_nglm_get_child(wtk_nglm_t *lm,wtk_lm_node_t *node,unsigned int id)
{
	//wtk_debug("of=%d\n",(int)node->child_offset);
	if(node->child_offset==0){return NULL;}
	if(node->nchild==0)
	{
		wtk_nglm_load_node(lm,node,0,lm->loc_heap);
	}
	return wtk_lm_node_find_child(node,id);
}

wtk_lm_node_t* wtk_nglm_step_wrd(wtk_nglm_t *lm,wtk_string_t *k,int nwrd,wtk_lm_node_t *node)
{
	wtk_lm_node_t *child;
	int id;

	wtk_debug("v[%d]=[%.*s]\n",nwrd,k->len,k->data);
	id=wtk_fst_insym_get_index(lm->dict_cfg->sym,k);
	child=wtk_nglm_get_child(lm,node,id);
	return child;
}

wtk_lm_node_t* wtk_nglm_get_node(wtk_nglm_t *lm,int *ids,int cnt)
{
	wtk_lm_node_t *node=NULL;
	int i;

	for(i=0;i<cnt;++i)
	{
		if(i==0)
		{
			node=wtk_nglm_get_uni_node(lm,ids[i]);
			if(!node){goto end;}
		}else
		{
			node=wtk_nglm_get_child(lm,node,ids[i]);
			if(!node){goto end;}
		}
	}
end:
	//wtk_lm_node_print2(lm->cfg->sym,node);
	return node;
}

void wtk_nglm_print_ids(wtk_nglm_t *lm,int *ids,int cnt)
{
	wtk_string_t *v;
	int i;

	for(i=0;i<cnt;++i)
	{
		v=lm->dict_cfg->sym->ids[ids[i]]->str;
		if(i>0)
		{
			printf(" ");
		}
		printf("%.*s",v->len,v->data);
	}
	if(cnt>0)
	{
		printf("\n");
	}
}

void wtk_nglm_debug_node(wtk_nglm_t *lm,wtk_lm_node_t *child,float bow)
{
	wtk_string_t *v;
	wtk_string_t oov=wtk_string("OOV");
	int id;

	id=child->id;
	if(id>=0)
	{
		v=lm->dict_cfg->sym->ids[id]->str;
	}else
	{
		v=&oov;
	}
	wtk_debug("%.*s prob=%f/%f ngram=%d prob=%f/%f\n",
			v->len,v->data,
			child?(child->prob+bow):0,
			child?((child->prob+bow)*14.0*log(10)):0,
			child?child->ngram:0,
			lm->prob,
			lm->prob*15.0*log(10)
			);
}

wtk_lm_node_t* wtk_nglm_get_bow_node(wtk_nglm_t *lm,wtk_lm_node_t *node)
{
	wtk_lm_node_t *t;
	int *pi;
	int ids[30];
	int cnt;

	cnt=wtk_lm_node_trace_id(node,ids);
	//wtk_nglm_print_ids(lm,ids+1,cnt-1);
	pi=ids;
	do{
		++pi;
		--cnt;
		t=wtk_nglm_get_node(lm,pi,cnt);
	}while(!t);
	return t;
}

wtk_lm_node_t* wtk_nglm_get_node_by_id(wtk_nglm_t *lm,int *ids,int cnt)
{
	wtk_lm_node_t *t;

	do{
		t=wtk_nglm_get_node(lm,ids,cnt);
		--cnt;
	}while(!t && cnt>0);
	return t;
}

wtk_lm_node_t* wtk_nglm_get_child_prob(wtk_nglm_t *lm,wtk_lm_node_t *node,int id,double *pbow)
{
	wtk_lm_node_t *child,*t;
	double bow;
	int *pi;
	int ids[30];
	int cnt;

	//wtk_debug("id=%d\n",id);
	bow=0;
	if(id<0)
	{
		child=NULL;
		goto end;
	}
	if(!node)
	{
		child=wtk_nglm_get_uni_node(lm,id);
		if(!child)
		{
			wtk_debug("uni=%d\n",id);
			goto end;
		}
		//lm->prob+=child->prob;
		goto end;
	}
	if(node->ngram==lm->cfg->max_order)
	{
		cnt=wtk_lm_node_trace_id(node,ids);
		//wtk_nglm_print_ids(lm,ids+1,cnt-1);
		pi=ids;
		do{
			++pi;
			--cnt;
			t=wtk_nglm_get_node(lm,pi,cnt);
		}while(!t);
		node=t;
		//wtk_debug("node=%p\n",node);
	}
	//wtk_debug("node=%d\n",node->ngram);
	//id=wtk_fst_insym_get_index(lm->cfg->sym,&k);
	child=wtk_nglm_get_child(lm,node,id);
	//wtk_debug("child=%p\n",child);
	if(!child)
	{
		//wtk_lm_node_print_parent2(lm->cfg->sym,node);
		//wtk_lm_node_print2(lm->cfg->sym,node);
		cnt=wtk_lm_node_trace_id(node,ids);
		pi=ids;
		while(!child && cnt>1)
		{
			bow+=node->bow;
			//wtk_debug("bow=%f cnt=%d\n",bow,cnt);
			do
			{
				--cnt;++pi;
				node=wtk_nglm_get_node(lm,pi,cnt);
			}while(!node && cnt>1);
			//wtk_lm_node_print2(lm->cfg->sym,node);
			if(!node)
			{
				break;
			}
			child=wtk_nglm_get_child(lm,node,id);
		}
		if(!child)
		{
			if(node)
			{
				bow+=node->bow;
				//wtk_debug("bow=%f\n",bow);
			}
			child=wtk_nglm_get_uni_node(lm,id);
			//wtk_debug("child=%p bow=%f %f/%f\n",child,node->bow,child->prob,child->bow);
		}
	}
	if(!child)
	{
		goto end;
	}
	//wtk_lm_node_print(child);
end:
	if(pbow)
	{
		*pbow=bow;
	}
//	if(lm->cfg->debug)
//	{
//		wtk_nglm_debug_node(lm,child,bow);
//		//lm->prob+=child->prob+bow;
//	}
	return child;

}

/*
wtk_lm_node_t* wtk_nglm_feed_child(wtk_nglm_t *lm,wtk_lm_node_t *node,int id)
{
	wtk_lm_node_t *child,*t;
	float bow;
	int *pi;
	int ids[30];
	int cnt;

	//wtk_debug("id=%d,[%.*s]\n",id,lm->cfg->sym->ids[id]->str->len,lm->cfg->sym->ids[id]->str->data);
	bow=0;
	if(id<0)
	{
		child=NULL;
		goto end;
	}
	if(!node)
	{
		child=wtk_nglm_get_uni_node(lm,id);
		if(!child){goto end;}
		lm->prob+=child->prob;
		goto end;
	}
	//wtk_debug("max=%d\n",lm->cfg->max_order);
	if(node->ngram==lm->cfg->max_order)
	{
		cnt=wtk_lm_node_trace_id(node,ids);
		//wtk_nglm_print_ids(lm,ids+1,cnt-1);
		pi=ids;
		do{
			++pi;
			--cnt;
			t=wtk_nglm_get_node(lm,pi,cnt);
		}while(!t);
		node=t;
		//wtk_debug("node=%p\n",node);
	}
	//id=wtk_fst_insym_get_index(lm->cfg->sym,&k);
	//wtk_lm_node_print2
	child=wtk_nglm_get_child(lm,node,id);
	if(!child)
	{
		//wtk_lm_node_print_parent2(lm->cfg->sym,node);
		//wtk_lm_node_print2(lm->cfg->sym,node);
		cnt=wtk_lm_node_trace_id(node,ids);
		pi=ids;
		while(!child && cnt>1)
		{
			bow+=node->bow;
			do
			{
				--cnt;++pi;
				node=wtk_nglm_get_node(lm,pi,cnt);
			}while(!node && cnt>1);
			//wtk_lm_node_print2(lm->cfg->sym,node);
			if(!node)
			{
				break;
			}
			child=wtk_nglm_get_child(lm,node,id);
		}
		if(!child)
		{
			if(node)
			{
				bow+=node->bow;
			}
			child=wtk_nglm_get_uni_node(lm,id);
			if(!child)
			{
				goto end;
			}
		}
	}
	if(!child)
	{
		goto end;
	}
	//wtk_lm_node_print(child);
	lm->prob+=child->prob+bow;
end:
	if(lm->cfg->debug)
	{
		wtk_string_t *v;
		wtk_string_t oov=wtk_string("OOV");

		if(id>=0)
		{
			v=lm->dict_cfg->sym->ids[id]->str;
		}else
		{
			v=&oov;
		}
		wtk_debug("%.*s prob=%f/%f ngram=%d prob=%f/%f\n",
				v->len,v->data,
				child?(child->prob+bow):0,
				child?((child->prob+bow)*14.0*log(10)):0,
				child?child->ngram:0,
				lm->prob,
				lm->prob*15.0*log(10)
				);

	}
	return child;
}
*/

typedef enum
{
	WTK_NGLM_STR_INIT,
	WTK_NGLM_STR,
}wtk_nglm_str_state_t;

double wtk_nglm_get_prob(wtk_nglm_t *lm,char *data,int bytes)
{
	char *s,*e;
	int n;
	wtk_string_t k;
	wtk_nglm_str_state_t state;
	wtk_lm_node_t *node;
	int id;
	double bow;

	//wtk_debug("[%.*s]\n",bytes,data);
	lm->prob=0;
	node=lm->s_node;
	//wtk_nglm_print_node(lm,node);
	s=data;e=s+bytes;
	wtk_string_set(&(k),0,0);
	state=WTK_NGLM_STR_INIT;
	while(s<e)
	{
		n=wtk_utf8_bytes(*s);
		//wtk_debug("state=%d %.*s\n",state,n,s);
		switch(state)
		{
		case WTK_NGLM_STR_INIT:
			if(n>1 || !isspace(*s))
			{
				//wtk_debug("[%.*s]\n",n,s);
				k.data=s;
				state=WTK_NGLM_STR;
				if(s+n>=e)
				{
					k.len=n;
					id=wtk_fst_insym_get_index(lm->dict_cfg->sym,&k);
					//wtk_debug("[%.*s]=%f id=%d\n",k.len,k.data,lm->prob,id);
					if(id!=lm->dict_cfg->snt_start_id && id!=lm->dict_cfg->snt_end_id)
					{
						node=wtk_nglm_get_child_prob(lm,node,id,&bow);
						wtk_nglm_print_node(lm,node);
						if(node)
						{
							lm->prob+=node->prob+bow;
							//wtk_debug("prob=%f/%f %f\n",node->prob,bow,node->prob+bow);
						}
						//node=wtk_nglm_feed_child(lm,node,id);
					}
				}
			}
			break;
		case WTK_NGLM_STR:
			if((n==1)&&isspace(*s))
			{
				k.len=s-k.data;
				state=WTK_NGLM_STR_INIT;
			}else if(s+n>=e)
			{
				k.len=e-k.data;
				state=WTK_NGLM_STR_INIT;
			}
			if(state==WTK_NGLM_STR_INIT)
			{
				id=wtk_fst_insym_get_index(lm->dict_cfg->sym,&k);
				//wtk_debug("[%.*s]=%f id=%d\n",k.len,k.data,lm->prob,id);
				if(id!=lm->dict_cfg->snt_start_id && id!=lm->dict_cfg->snt_end_id)
				{
					//node=wtk_nglm_feed_child(lm,node,id);
					node=wtk_nglm_get_child_prob(lm,node,id,&bow);
					//wtk_nglm_print_node(lm,node);
					if(node)
					{
						lm->prob+=node->prob+bow;
						//wtk_debug("prob=%f/%f %f\n",node->prob,bow,node->prob+bow);
					}
				}
				//wtk_debug("[%.*s]=%f\n",k.len,k.data,lm->prob);
			}
			break;
		}
		s+=n;
	}
	///wtk_nglm_feed_child(lm,node,lm->dict_cfg->snt_end_id);
	node=wtk_nglm_get_child_prob(lm,node,lm->dict_cfg->snt_end_id,&bow);
	//wtk_debug("prob=%f\n",lm->prob);
	//wtk_debug("prob=%f/%f %f\n",node->prob,bow,node->prob+bow);
	if(node)
	{
		lm->prob+=node->prob+bow;
	}
	//wtk_debug("prob=%f\n",lm->prob);
	wtk_nglm_reset(lm);
	//wtk_debug("prob=%f\n",lm->prob);
	//exit(0);
	return lm->prob;
}

wtk_lm_node_t *wtk_nglm_get_node2(wtk_nglm_t *lm,char *data,int bytes)
{
	wtk_nglm_str_state_t state;
	char *s,*e;
	wtk_string_t k;
	int n;
	int cnt=0;
	int ids[100];
	int id;
	wtk_lm_node_t *node;

	s=data;e=s+bytes;
	wtk_string_set(&(k),0,0);
	state=WTK_NGLM_STR_INIT;
	while(s<e)
	{
		n=wtk_utf8_bytes(*s);
		//wtk_debug("state=%d %.*s\n",state,n,s);
		switch(state)
		{
		case WTK_NGLM_STR_INIT:
			if(n>1 || !isspace(*s))
			{
				//wtk_debug("[%.*s]\n",n,s);
				k.data=s;
				state=WTK_NGLM_STR;
				if(s+n>=e)
				{
					k.len=n;
					id=wtk_fst_insym_get_index(lm->dict_cfg->sym,&k);
					ids[cnt++]=id;
					//wtk_debug("[%.*s]\n",k.len,k.data);
				}
			}
			break;
		case WTK_NGLM_STR:
			if((n==1)&&isspace(*s))
			{
				k.len=s-k.data;
				state=WTK_NGLM_STR_INIT;
			}else if(s+n>=e)
			{
				k.len=e-k.data;
				state=WTK_NGLM_STR_INIT;
			}
			if(state==WTK_NGLM_STR_INIT)
			{
				id=wtk_fst_insym_get_index(lm->dict_cfg->sym,&k);
				ids[cnt++]=id;
				//wtk_debug("[%.*s]\n",k.len,k.data);
			}
			break;
		}
		s+=n;
	}
	node=wtk_nglm_get_node(lm,ids,cnt);
	return node;
}

void wtk_nglm_dump_nodes2(wtk_nglm_t *lm,wtk_lm_node_t *parent,int order,FILE *f)
{
	wtk_fst_insym_t *in=lm->dict_cfg->sym;
	wtk_string_t *v;
	wtk_lm_node_t *node;
	int i,j;
	wtk_string_t *vx[5];

	node=parent;
	while(node)
	{
		v=in->ids[node->id]->str;
		//wtk_debug("v[%d]=%.*s\n",node->ngram,v->len,v->data);
		vx[node->ngram-1]=v;
		node=node->parent;
	}
	//exit(0);
	for(i=0;i<parent->nchild;++i)
	{
		node=parent->childs[i];
		if(node->ngram==order)
		{
			fprintf(f,"%f",node->prob);
			for(j=0;j<parent->ngram;++j)
			{
				//wtk_debug("[%.*s]\n",vx[j]->len,vx[j]->data);
				fprintf(f," %.*s",vx[j]->len,vx[j]->data);
			}
			//exit(0);
			v=in->ids[node->id]->str;
			if(node->bow==0.0)
			{
				fprintf(f," %.*s\n",v->len,v->data);
			}else
			{
				fprintf(f," %.*s %f\n",v->len,v->data,node->bow);
			}
			//exit(0);
		}else if(node->nchild>0)
		{
			wtk_nglm_dump_nodes2(lm,node,order,f);
		}
	}
}

void wtk_nglm_dump_nodes(wtk_nglm_t *lm,int order,FILE *f)
{
	wtk_lm_node_t *node;
	wtk_fst_insym_t *in=lm->dict_cfg->sym;
	wtk_string_t *v;
	int i;

	for(i=0;i<lm->n_uni_nodes;++i)
	{
		node=lm->uni_nodes+i;
		if(node->ngram==order)
		{
			v=in->ids[node->id]->str;
			if(node->prob==0)
			{
				wtk_debug("%f %.*s\n",node->prob,v->len,v->data);
				continue;
			}
			if(node->bow==0.0)
			{
				fprintf(f,"%f %.*s\n",node->prob,v->len,v->data);
			}else
			{
				fprintf(f,"%f %.*s %f\n",node->prob,v->len,v->data,node->bow);
			}
		}else if(node->nchild>0)
		{
			wtk_nglm_dump_nodes2(lm,node,order,f);
		}
	}
}

void wtk_nglm_dump(wtk_nglm_t *lm,char *fn)
{
	FILE *f;
	int i;

	//wtk_debug("%s\n",fn);
	//f=stdout;
	f=fopen(fn,"w");
	fprintf(f,"\n\\data\\\n");
	for(i=0;i<lm->cfg->max_order;++i)
	{
		fprintf(f,"ngram %d=%d\n",i+1,lm->n_order[i]);
	}
	for(i=0;i<lm->cfg->max_order;++i)
	{
		fprintf(f,"\n\\%d-grams:\n",i+1);
		wtk_debug("write order=%d\n",i+1);
		wtk_nglm_dump_nodes(lm,i+1,f);
	}
	fprintf(f,"\n\\end\\\n");
	fclose(f);
}
