#include "wtk_lm2bin.h"
void wtk_lm2bin_load_node(wtk_lm2bin_t *b,wtk_lm_node_t *node);

void wtk_lm2bin_node_to_bin(wtk_lm2bin_t *b,wtk_lm_node_t *n,wtk_strbuf_t *buf)
{
//	static int ki=0;
//
//	++ki;
//	if(n->id==9 && n->ngram==2 && n->parent->id==13512)
//	{
//		wtk_debug("ki==%d\n",ki);
//		wtk_lm_node_print_parent(n);
//	}
	switch(b->type)
	{
	case WTK_LM2BIN_a:
		wtk_lm_node_to_bin(n,buf,b->prob_scale,b->bow_scale);
		break;
	case WTK_LM2BIN_A:
		wtk_lm_node_to_bin(n,buf,b->prob_scale,b->bow_scale);
		break;
	case WTK_LM2BIN_B:
		wtk_lm_node_to_bin_small(n,buf,b->prob_scale,b->bow_scale);
		break;
	case WTK_LM2BIN_C:
		wtk_lm_node_to_bin_small2(n,buf,b->prob_scale,b->bow_scale);
		break;
	}
}

void wtk_lm2bin_node_to_bin_end(wtk_lm2bin_t *b,wtk_lm_node_t *n,wtk_strbuf_t *buf)
{
//	static int ki=0;
//
//	++ki;
//	if(n->id==9 && n->ngram==2 && n->parent->id==13512)
//	{
//		wtk_debug("ki==%d\n",ki);
//		wtk_lm_node_print_parent(n);
//	}
	switch(b->type)
	{
	case WTK_LM2BIN_a:
		wtk_lm_node_to_bin2(n,buf,b->prob_scale);
		break;
	case WTK_LM2BIN_A:
		wtk_lm_node_to_bin2(n,buf,b->prob_scale);
		break;
	case WTK_LM2BIN_B:
		wtk_lm_node_to_bin_samll_end(n,buf,b->prob_scale);
		break;
	case WTK_LM2BIN_C:
		wtk_lm_node_to_bin_samll_end2(n,buf,b->prob_scale);
		break;
	}
}


wtk_lm2bin_t* wtk_lm2bin_new()
{
	wtk_lm2bin_t *b;
	int i;

	b=(wtk_lm2bin_t*)wtk_malloc(sizeof(wtk_lm2bin_t));
	b->label=wtk_label_new(25007);
	b->olog=NULL;
	b->buf=wtk_strbuf_new(1024*1024*10,1);
	b->max_order=0;
	for(i=0;i<MAX_LM_ORDER;++i)
	{
		b->order[i]=0;
	}
	b->node_pool=wtk_vpool_new(sizeof(wtk_lm_node_t),102400);
	//b->use_small=0;
	b->type=WTK_LM2BIN_A;
	b->prob_scale=10000.0;
	b->bow_scale=10000.0;
	return b;
}

void wtk_lm2bin_delete(wtk_lm2bin_t *b)
{
	wtk_strbuf_delete(b->buf);
	wtk_label_delete(b->label);
	wtk_free(b);
}

int wtk_lm2bin_write(char *data,int bytes,int n,FILE *log)
{
	/*
	int of;
	int pos;

	pos=256277;
	of=ftell(log);
	if(of<=pos && of+bytes>pos)
	{
		wtk_lm_node_t node;
		wtk_debug("chekc bug %d:%d pos=%d\n",of,bytes,pos);
		print_hex(data,bytes);

		wtk_lm_node_from_bin2(&(node),data+4,bytes-4);
		wtk_debug("prob=%f bow=%f\n",node.prob,node.bow);
		exit(0);
	}*/
	return fwrite(data,bytes,n,log);
}



void wtk_lm2bin_write_uni_gram(wtk_lm2bin_t *b)
{
	wtk_strbuf_t *buf;
	FILE *log=b->olog;
	wtk_lm_node_t *node;
	int i;
	unsigned int n;
	int nx;

	buf=b->buf;
	//n=b->order[1];
	n=max(b->order[1],b->sym->nid);
	//wtk_debug("%d\n",n);
	wtk_strbuf_reset(buf);
	//wtk_debug("n=%d\n",n);
	wtk_strbuf_push(buf,(char*)&n,4);
	//header prob
	//nx=12;
	nx=b->type==WTK_LM2BIN_A?4:12;
	//nx=wtk_lm2bin_type_bytes(b->type);
	//wtk_debug("n=%d %d/%d\n",n,(int)(b->order[1]),(int)(b->sym->nid));
	for(i=1;i<=n;++i)
	{
		node=b->uni_nodes+i;
		//prob,bow
		//node->prob=10.0;
		//node->bow=5.0;
		//wtk_lm_node_print(node);
		node->self_offset=buf->pos+nx;
		if(node->id!=i)
		{
			node->id=i;
			//wtk_debug("%p %d/%d n=%d\n",node,node->id,i,n);
			//exit(0);
		}
		wtk_lm2bin_node_to_bin(b,node,buf);
		//wtk_lm_node_from_bin(node,buf->data+4,buf->pos-4);
		//wtk_lm_node_print(node);
		//exit(0);
	}
	//wtk_debug("n=%d\n",n);
	fseek(log,nx,SEEK_SET);
	//wtk_debug("n=%ld\n",ftell(log));
	wtk_lm2bin_write(buf->data,buf->pos,1,log);
	//exit(0);
}

int wtk_lm_node_cmp2(const void *v1,const void *v2)
{
	wtk_lm_node_t *n1,*n2;

	n1=*((wtk_lm_node_t**)v1);
	n2=*((wtk_lm_node_t**)v2);
	return n1->id-n2->id;
	//return n2->id-n1->id;
}

void wtk_lm2bin_print_nodes(wtk_lm_node_t **child,int n)
{
	int i;

	for(i=0;i<n;++i)
	{
		wtk_debug("v[%d]=%d prob=%f bow=%f node=%p\n",i,child[i]->id,
				child[i]->prob,child[i]->bow,
				child[i]);
	}
}


void wtk_lm2bin_write_ngram(wtk_lm2bin_t *b,wtk_lm_node_t *node,wtk_lm_node_t **child,int n)
{
	FILE *log=b->olog;
	wtk_strbuf_t *buf=b->buf;
	int i;
	uint64_t of;
	//wtk_string_t *v;

	//v=b->sym->ids[node->id]->str;
	//wtk_debug("write ngram=%d %.*s\n",node->ngram,v->len,v->data);
	//wtk_lm_node_print(node);
	//wtk_lm_node_print2(b->sym,node);
	//exit(0);
	if(node->child_offset>0 || node->self_offset<=0)
	{
		wtk_lm_node_print_parent(node);
		//wtk_lm_node_print(node);
		wtk_lm_node_print2(b->sym,node->parent);
		wtk_lm_node_print2(b->sym,node);
		wtk_debug("found bug child=%d self=%d nchld=%d ngram=%d n=%d\n",(int)node->child_offset,(int)node->self_offset,node->nchild,node->ngram,n);
		exit(0);
	}
	//wtk_lm_node_print(node);
	//wtk_lm2bin_print_nodes(child,n);
	qsort(child,n,sizeof(wtk_lm_node_t*),wtk_lm_node_cmp2);
	//wtk_lm2bin_print_nodes(child,n);
	//exit(0);
	wtk_strbuf_reset(buf);
	wtk_strbuf_push(buf,(char*)&n,4);
	node->nchild=n;
	node->childs=child;
	for(i=0;i<n;++i)
	{
		child[i]->parent=node;
		child[i]->ngram=node->ngram+1;
		/*
		if(0 && child[i]->ngram==3)
		{
			wtk_debug("foudn bug [%d/%d]\n",child[i]->ngram,b->max_order);
			exit(0);
		}*/
		if(child[i]->ngram==b->max_order)
		{
			wtk_lm2bin_node_to_bin_end(b,child[i],buf);
		}else
		{
			wtk_lm2bin_node_to_bin(b,child[i],buf);
		}
	}
//	if(node->id==597)
//	{
//		wtk_debug("pos=%ld\n",ftell(log));
//		print_data(buf->data,4);
//		print_data(buf->data+4,20);
//	}
	node->nchild=0;
	node->childs=NULL;
	node->child_offset=ftell(log);
	//wtk_debug("id=%d n=%d of=%ld\n",node->id,n,node->child_offset);
	//fwrite(buf->data,buf->pos,1,log);

	wtk_lm2bin_write(buf->data,buf->pos,1,log);

	//fflush(log);
	of=node->child_offset+buf->pos;

	wtk_strbuf_reset(buf);
	wtk_lm2bin_node_to_bin(b,node,buf);

	fseek(log,node->self_offset,SEEK_SET);
	wtk_lm2bin_write(buf->data,buf->pos,1,log);
	fseek(log,of,SEEK_SET);

	/*
	{
		wtk_strbuf_reset(buf);
		wtk_lm_node_tostring(b->sym,node,buf);
		if(wtk_str_equal_s(buf->data,buf->pos,"0 万"))
		{
			wtk_lm_node_print_parent(node);
			//exit(0);
		}
	}*/
	//wtk_lm_node_print(node);
	//exit(0);
}

wtk_lm_node_t* wtk_lm2bin_pop_node(wtk_lm2bin_t *b)
{
	wtk_lm_node_t *n;

	n=wtk_vpool_pop(b->node_pool);
	//wtk_queue2_init(&(n->child_q));
	n->nchild=0;
	n->childs=NULL;
	n->id=0;
	n->child_offset=0;
	n->self_offset=0;
	n->bow=0;
	n->prob=0;
	return n;
}

void wtk_lm2bin_push_node(wtk_lm2bin_t *b,wtk_lm_node_t *n)
{
	wtk_vpool_push(b->node_pool,n);
}

void wtk_lm2bin_push_node2(wtk_lm2bin_t *b,wtk_lm_node_t *n)
{
	int i;

	if(n->childs)
	{
		for(i=0;i<n->nchild;++i)
		{
			wtk_lm2bin_push_node2(b,n->childs[i]);
		}
		wtk_free(n->childs);
		n->childs=NULL;
	}
	wtk_lm2bin_push_node(b,n);
}

void wtk_lm2bin_push_child_node(wtk_lm2bin_t *b,wtk_lm_node_t *n)
{
	int i;

	if(n->childs)
	{
		for(i=0;i<n->nchild;++i)
		{
			wtk_lm2bin_push_node2(b,n->childs[i]);
		}
		wtk_free(n->childs);
		n->childs=NULL;
		n->nchild=0;
	}
}


void wtk_lm2bin_load_node(wtk_lm2bin_t *b,wtk_lm_node_t *node)
{
	FILE *log=b->olog;
	uint64_t of;
	int n;
	wtk_strbuf_t *buf=b->buf;
	int ret;
	char *s,*e;
	wtk_lm_node_t *p;
	uint64_t of1;
	int i;
	float f1,f2;
	int nx;

	switch(b->type)
	{
	case WTK_LM2BIN_A:
		f1=1.0/b->prob_scale;
		f2=1.0/b->bow_scale;
		nx=12;
		break;
	case WTK_LM2BIN_B:
		f1=1.0/b->prob_scale;
		f2=1.0/b->bow_scale;
		nx=9;
		break;
	case WTK_LM2BIN_C:
		f1=1.0/b->prob_scale;
		f2=1.0/b->bow_scale;
		nx=10;
		break;
	case WTK_LM2BIN_a:
		f1=1.0/b->prob_scale;
		f2=1.0/b->bow_scale;
		nx=12;
		break;
	default:
		return;
	}
	of=ftell(log);
	ret=fseek(log,node->child_offset,SEEK_SET);
	//wtk_debug("ret=%d offset=%d,of=%d\n",ret,(int)node->child_offset,(int)of);
//	if(node->id==597)
//	{
//		wtk_debug("pos=%ld\n",ftell(log));
//	}
	ret=fread((char*)&n,4,1,log);
	if(ret!=1)
	{
		perror(__FUNCTION__);
		exit(0);
	}
//	if(node->id==597)
//	{
//		print_data((char*)&n,4);
//	}
	//print_data((char*)&n,4);
	//wtk_debug("ret=%d id=%d n=%d of=%ld\n",ret,node->id,n,node->child_offset);
	//exit(0);
	node->nchild=n;
	node->childs=(wtk_lm_node_t**)wtk_calloc(n,sizeof(wtk_lm_node_t*));
	n=n*nx;
	ret=fread(buf->data,n,1,log);
	if(1!=ret)
	{
		wtk_debug("bug[%d,%d]\n",n,ret);
		exit(0);
	}
//	if(node->id==597)
//	{
//		print_data(buf->data,20);
//	}
	s=buf->data;
	e=s+n;
	of1=node->child_offset+4;
	i=0;
	while(s<e)
	{
		p=wtk_lm2bin_pop_node(b);
		p->ngram=node->ngram+1;
		p->parent=node;
		wtk_lm2bin_from_bin(b->type,p,s,f1,f2);
		p->self_offset=of1;
		node->childs[i++]=p;
		s+=nx;
		of1+=nx;
	}
	//wtk_debug("n=%d\n",n);
	fseek(log,of,SEEK_SET);
	//wtk_lm_node_print(node);
	//exit(0);
}


int wtk_lm2bin_dump_fn(wtk_lm2bin_t *b,wtk_source_t *src)
{
typedef enum
{
	WTK_LM2BIN_INIT,
	WTK_LM2BIN_HDR,
	WTK_LM2BIN_WAIT_NGRAM,
	WTK_LM2BIN_READ_NGRAM,
}wtk_lm2bin_state_t;
	wtk_strbuf_t *buf;
	int ret,eof;
	wtk_lm2bin_state_t state;
	wtk_heap_t *heap;
	wtk_array_t *a;
	wtk_string_t **strs,*v;
	int order;
	uint64_t iv;
	float prob,bow;
	unsigned int id;
	wtk_lm_node_t *node;
	wtk_lm_node_t *pnode[MAX_LM_ORDER];
	wtk_lm_node_t *last_key_node=NULL;
	int i;
	wtk_heap_t *glb_heap;
	wtk_larray_t *node_array;
	uint64_t cnt=0;
	uint64_t tot=0;

	glb_heap=wtk_heap_new(4096);
	node_array=wtk_larray_new(10240,sizeof(wtk_lm_node_t*));
	order=-1;
	heap=wtk_heap_new(4096);
	state=WTK_LM2BIN_INIT;
	buf=wtk_strbuf_new(256,1);
	while(1)
	{
		ret=wtk_source_read_line2(src,buf,&eof);
		if(ret!=0 || eof)
		{
			goto end;
		}
		++cnt;
		if(cnt%100000==0)
		{
			wtk_debug("v[%d]:%d/%d node=%d/%d heap=%fM/%fM\n",
					order,(int)cnt,(int)tot,
					b->node_pool->hoard.use_length,b->node_pool->hoard.cur_free,
					wtk_heap_bytes(glb_heap)*1.0/(1024*1024),
					wtk_heap_bytes(heap)*1.0/(1024*1024));
		}
		//wtk_debug("[%.*s]\n",buf->pos,buf->data);
		wtk_strbuf_strip(buf);
		switch(state)
		{
		case WTK_LM2BIN_INIT:
			if(buf->pos>0)
			{
				if(wtk_str_equal_s(buf->data,buf->pos,"\\data\\")==1)
				{
					state=WTK_LM2BIN_HDR;
				}
			}
			break;
		case WTK_LM2BIN_HDR:
			if(buf->pos>0)
			{
				a=wtk_str_to_array(heap,buf->data,buf->pos,' ');
				strs=(wtk_string_t**)a->slot;
				v=strs[1];
				a=wtk_str_to_array(heap,v->data,v->len,'=');
				strs=(wtk_string_t**)a->slot;
				v=strs[0];
				order=wtk_str_atoi(v->data,v->len);
				v=strs[1];
				iv=wtk_str_atoi(v->data,v->len);
				b->order[order]=iv;
				tot+=iv;
				if(order>b->max_order)
				{
					b->max_order=order;
				}
				wtk_heap_reset(heap);
				wtk_debug("%d=%d/%d\n",order,(int)iv,b->max_order);
			}else
			{
				state=WTK_LM2BIN_WAIT_NGRAM;
			}
			break;
		case WTK_LM2BIN_WAIT_NGRAM:
			if(buf->pos>0 && buf->data[0]=='\\')
			{
				a=wtk_str_to_array(heap,buf->data+1,buf->pos-1,'-');
				strs=(wtk_string_t**)a->slot;
				v=strs[0];
				//wtk_debug("[%.*s]\n",v->len,v->data);
				order=wtk_str_atoi(v->data,v->len);
				wtk_heap_reset(heap);
				state=WTK_LM2BIN_READ_NGRAM;
				if(order==1)
				{
					int nodes;

					nodes=max(b->order[order],b->sym->nid)+1;
					b->uni_nodes=(wtk_lm_node_t*)wtk_calloc(nodes,sizeof(wtk_lm_node_t));
				}
				last_key_node=NULL;
				memset(pnode,0,sizeof(wtk_lm_node_t*)*MAX_LM_ORDER);
			}
			break;
		case WTK_LM2BIN_READ_NGRAM:
			if(buf->pos==0)
			{
				if(order==1)
				{
					//wtk_debug("write uni\n");
					wtk_lm2bin_write_uni_gram(b);
				}else
				{
					if(last_key_node)
					{
						wtk_lm2bin_write_ngram(b,last_key_node,(wtk_lm_node_t**)node_array->slot,
								node_array->nslot);
						wtk_heap_reset(glb_heap);
						wtk_larray_reset(node_array);
					}
				}
				state=WTK_LM2BIN_WAIT_NGRAM;
				continue;
			}
			a=wtk_str_to_array2(heap,buf->data,buf->pos);
			strs=(wtk_string_t**)a->slot;
			prob=wtk_str_atof(strs[0]->data,strs[0]->len);
			if(order==1)
			{
				id=wtk_fst_insym_get_index(b->sym,strs[1]);
				if(id<=0 || id>429496729)
				{
					wtk_debug("invalid id=[%.*s]\n",strs[1]->len,strs[1]->data);
					exit(0);
					goto end;
				}
				//wtk_debug("n=%d\n",a->nslot);
				if(a->nslot>2)
				{
					bow=wtk_str_atof(strs[2]->data,strs[2]->len);
				}else
				{
					bow=0;
				}
				//wtk_debug("id=%d prob=%f bow=%f\n",id,prob,bow);
				node=b->uni_nodes+id;
				node->id=id;
				node->prob=prob;
				node->bow=bow;
				node->child_offset=0;
				node->ngram=1;
				node->childs=0;
				node->nchild=0;
				node->parent=NULL;
				node->self_offset=0;
			}else
			{
				//wtk_debug("v[%ld]=[%.*s]\n",cnt,buf->pos,buf->data);
				//wtk_debug("nslot=%d\n",a->nslot);
				for(i=1;i<order;++i)
				{
					id=wtk_fst_insym_get_index(b->sym,strs[i]);
					if(i==1)
					{
						node=b->uni_nodes+id;
					}else
					{
						node=pnode[i-1];
						if(node->nchild==0)
						{
							wtk_lm2bin_load_node(b,node);
						}
						node=wtk_lm_node_find_child(node,id);
						if(!node)
						{
							int k;

							wtk_lm_node_print2(b->sym,pnode[i-1]);
							wtk_debug("nchild=%d id=%d offset=%ld/%ld id=%d\n",pnode[i-1]->nchild,id,
									(long)pnode[i-1]->self_offset,(long)pnode[i-1]->child_offset,pnode[i-1]->id);
							for(k=0;k<pnode[i-1]->nchild;++k)
							{
								wtk_debug("v[%d/%d]=%d\n",k,pnode[i-1]->nchild,pnode[i-1]->childs[k]->id);
								if(k>0 && pnode[i-1]->childs[k]->id<pnode[i-1]->childs[k-1]->id)
								{
									wtk_lm_node_print2(b->sym,pnode[i-1]->childs[k]);
									exit(0);
								}
							}
							wtk_debug("id=%d\n",id);
							wtk_lm_node_print2(b->sym,pnode[i-1]);
							exit(0);
						}
					}
					//wtk_debug("v[%d/%d]=%p last_key=%p\n",i,node_array->nslot,node,last_key_node);
					if(node!=pnode[i])
					{
						if(pnode[i])
						{
							int j;

							if(last_key_node)
							{
								wtk_lm2bin_write_ngram(b,last_key_node,
										(wtk_lm_node_t**)node_array->slot,
									node_array->nslot);
//								if(last_key_node->id==597)// && node_array->nslot==269)
//								{
//									int ki;
//
//									for(ki=0;ki<3;++ki)
//									{
//										wtk_debug("v[%d]=%d\n",ki,((wtk_lm_node_t**)node_array->slot)[ki]->id);
//									}
//									wtk_debug("id=%d n=%d offset=%ld/%ld nslot=%d\n",last_key_node->id,last_key_node->nchild,
//											last_key_node->self_offset,last_key_node->child_offset,node_array->nslot);
//
//									wtk_lm_node_print2(b->sym,last_key_node);
//								}
								wtk_heap_reset(glb_heap);
								wtk_larray_reset(node_array);
								last_key_node=NULL;
							}
							wtk_lm2bin_push_child_node(b,pnode[i]);
							for(j=i;j<=b->max_order+2;++j)
							{
								pnode[j]=NULL;
							}
						}
						pnode[i]=node;
						if(i==order-1 && (last_key_node!=node))
						{
							if(last_key_node)
							{
								wtk_lm_node_print(last_key_node);
								exit(0);
							}
							last_key_node=node;
						}
					}
				}
				if(last_key_node==NULL)
				{
					wtk_debug("found bug\n");
					exit(0);
				}
				id=wtk_fst_insym_get_index(b->sym,strs[order]);
				if(a->nslot==(order+2))
				{
					bow=wtk_str_atof(strs[order+1]->data,strs[order+1]->len);
				}else
				{
					bow=0;
				}
//				{
//					int ki;
//
//					for(ki=0;ki<node_array->nslot;++ki)
//					{
//						if(((wtk_lm_node_t**)(node_array->slot))[ki]->id==id)
//						{
//							wtk_debug("found bug\n");
//							exit(0);
//						}
//					}
//				}
				node=(wtk_lm_node_t*)wtk_heap_malloc(glb_heap,sizeof(wtk_lm_node_t));
				node->id=id;
				node->prob=prob;
				node->bow=bow;
				node->child_offset=0;
				node->self_offset=0;
				node->nchild=0;
				node->childs=NULL;
				node->parent=last_key_node;
				wtk_larray_push2(node_array,&node);
			}
			wtk_heap_reset(heap);
			break;
		default:
			break;
		}
	}
	ret=0;
end:
	wtk_strbuf_delete(buf);
	return ret;
}

int wtk_lm2bin_dump(wtk_lm2bin_t *b,char *wl_fn,char *lm_fn,char *bin_fn)
{
	int ret=-1;

	wtk_debug("load wrd [%s]\n",wl_fn);
	b->sym=wtk_fst_insym_new(b->label,wl_fn,1);
	if(!b->sym)
	{
		wtk_debug("load wrd file[%s] failed.\n",wl_fn);
		goto end;
	}
	wtk_debug("bin_fn %s type=%d\n",bin_fn,b->type);
	b->olog=fopen(bin_fn,"w+rb");
	switch(b->type)
	{
	case WTK_LM2BIN_A:
		fprintf(b->olog,"Y==A");
		//print_data((char*)&(b->prob_scale),4);
		//print_data((char*)&(b->bow_scale),4);
		//wtk_debug("%f/%f\n",b->prob_scale,b->bow_scale);
		break;
	case WTK_LM2BIN_B:
		fprintf(b->olog,"Y==B");
		fwrite(&(b->prob_scale),4,1,b->olog);
		fwrite(&(b->bow_scale),4,1,b->olog);
		break;
	case WTK_LM2BIN_C:
		fprintf(b->olog,"Y==C");
		fwrite(&(b->prob_scale),4,1,b->olog);
		fwrite(&(b->bow_scale),4,1,b->olog);
		break;
	case WTK_LM2BIN_a:
		fprintf(b->olog,"Y==a");
		fwrite(&(b->prob_scale),4,1,b->olog);
		fwrite(&(b->bow_scale),4,1,b->olog);
		//print_data((char*)&(b->prob_scale),4);
		//print_data((char*)&(b->bow_scale),4);
		//wtk_debug("%f/%f\n",b->prob_scale,b->bow_scale);
		break;
	}
	//print_data((char*)&(b->prob_scale),4);
	//print_data((char*)&(b->bow_scale),4);
	fflush(b->olog);
	ret=wtk_source_load_file(b,(wtk_source_load_handler_t)wtk_lm2bin_dump_fn,lm_fn);
	if(ret!=0){goto end;}
	ret=0;
end:
	if(b->olog)
	{
		fclose(b->olog);
		b->olog=NULL;
	}
	return ret;
}
