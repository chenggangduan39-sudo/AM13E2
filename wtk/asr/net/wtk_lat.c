#include "wtk_lat.h"
#include "wtk/core/wtk_sort.h"
typedef struct wtk_token wtk_token_t;
typedef struct wtk_path wtk_path_t;
typedef struct wtk_align wtk_align_t;
typedef struct wtk_nxtpath wtk_nxtpath_t;
typedef struct wtk_tokenset wtk_tokenset_t;
typedef struct wtk_netinst wtk_netinst_t;
typedef struct wtk_precomp wtk_precomp_t;
typedef struct wtk_hsetinfo wtk_hsetinfo_t;
typedef struct wtk_observation wtk_observation_t;
typedef struct wtk_reltoken wtk_reltoken_t;

struct wtk_token
{
	double like;		//Likelihood of token
	float lm;			//LM likelihood of token.
	wtk_path_t* path;	//route (word level) through network.
	wtk_align_t *align;	//route(state/model level) through network.
};

struct wtk_reltoken
{
	double like;		//Relative Likelihood of token
	float lm;			//LM likelihood of token.
	wtk_path_t* path;	//route (word level) through network.
	wtk_align_t *align;	//route(state/model level) through network.
};

struct wtk_path
{
	wtk_queue_node_t q_n;
	wtk_queue_t* queue;		//path linked in.
	wtk_path_t *prev;		//Previous word record
	wtk_netnode_t *node;	//word level traceback info.
	wtk_nxtpath_t *chain;	//Next of NBest Paths
	wtk_align_t *align;		//state/model traceback for this word.
	double like;		//likelihood at boundary.
	float lm;			//likelihood of current word.
	int frame;				//time of boundary(end of word)
	int usage;			//times struct refrenced by next path
	unsigned char used:1;	//reference to struct by current inst.
};

struct wtk_nxtpath
{
	wtk_path_t *prev;		//previous word record.
	wtk_align_t *align;
	wtk_nxtpath_t *chain;		//Next of NBest paths
	double like;		//likelihood at boundary.
	float	lm;			//lm likelihood of current word.
};

struct wtk_align
{
	wtk_queue_node_t q_n;
	wtk_queue_t *queue;		//align linked in.
	wtk_align_t *prev;	//previous align record
	wtk_netnode_t *node;	//node for which alignment information present.
	double like;		//likelihood upon entering state/model end
	int	frame;		//frame number upon entering state/model end
	int usage;		//times struct refrenced by align or path
	short state;		//state level traceback info.
	unsigned char used:1;	//refrence to struct by current inst.
};

struct wtk_tokenset
{
	wtk_token_t tok;	//most likely token in state.
	wtk_reltoken_t *set;	//likelihood sorted array[0 ..nToks]
	short n;	//number or valid tokens.
};


struct wtk_netinst
{
	wtk_queue_node_t q_n;
	wtk_netnode_t* node;
	wtk_tokenset_t *state;	//TokenSet[0..N-2] in state [1..N-1] for hmm
	wtk_tokenset_t *exit;	//TokenSet in exit state.
	double wdlk;				//Max likelihood of t=0 path to word end node
	double max;				//Likelihood for pruning of instance
	unsigned char pxd:1;	//external propagation done this frame.
	unsigned char ooo:1;	//instance potentially out of order.
};


static wtk_string_t null_name=wtk_string("!NULL");

wtk_lat_t* wtk_lat_new_h(wtk_heap_t *heap)
{
	wtk_lat_t *lat;

	lat=(wtk_lat_t*)wtk_heap_malloc(heap,sizeof(*lat));
	wtk_lat_init(lat,heap);
	return lat;
}

int wtk_lat_init(wtk_lat_t *lat,wtk_heap_t *heap)
{
	lat->heap=heap;
	//lat->frame_dur=frame_dur;
	lat->lmscale=1;
	lat->acscale=1;
	lat->wdpenalty=0;
	lat->prscale=1;
	lat->logbase=1;
	lat->tscale=1;
	lat->name=0;
	return 0;
}

int wtk_lat_reset(wtk_lat_t *lat)
{
	wtk_netnode_t *n;

	n=lat->chain;
	while(n)
	{
		n->net_inst=0;
		n=n->chain;
	}
	return 0;
}

wtk_netnode_t* wtk_netnode_dup(wtk_netnode_t *n,wtk_heap_t *h)
{
	wtk_netnode_t* cpy;

	cpy=(wtk_netnode_t*)wtk_heap_malloc(h,sizeof(*cpy));
	*cpy=*n;
	cpy->net_inst=0;cpy->nlinks=0;cpy->links=0;cpy->chain=0;
	return cpy;
}

void wtk_netnode_dup_link(wtk_netnode_t *n1,wtk_netnode_t *n2,wtk_heap_t *h)
{
	int i,n;

	n=n2->nlinks;
	if(n1->nlinks>0)
	{
		wtk_netnode_print(n1);
		wtk_debug("%p\n",n1);
		//exit(0);
	}
	n1->nlinks=n;
	n1->links=(wtk_netlink_t*)wtk_heap_malloc(h,sizeof(wtk_netlink_t)*n);
	for(i=0;i<n;++i)
	{
		n1->links[i].node=(wtk_netnode_t*)n2->links[i].node->net_inst;
		n1->links[i].like=n2->links[i].like;
	}
}

void wtk_lat_link_node(wtk_lnode_t *s,wtk_lnode_t *e,wtk_larc_t *arc)
{
	arc->start=s;
	arc->end=e;

	arc->parc=e->pred;
	e->pred=arc;

	arc->farc=s->foll;
	s->foll=arc;
}


wtk_lat_t *wtk_lat_dup2(wtk_lat_t *src,wtk_heap_t *heap)
{
	wtk_lat_t *dst;
	wtk_lnode_t *s_n,*d_n,*d_n2;
	wtk_larc_t *arc,*arc2;
	int i,idx;
	int aid;

	dst=wtk_lat_new_h(heap);
	wtk_lat_create(dst,src->nn,src->na);
	aid=0;
	for(i=0;i<src->nn;++i)
	{
		s_n=src->lnodes+i;
		d_n=dst->lnodes+i;
		/*
		*d_n=*s_n;
		d_n->foll=NULL;
		d_n->pred=NULL;
		*/
		d_n->score=s_n->score;
		d_n->time=s_n->time;
		d_n->n=s_n->n;
		d_n->v=s_n->v;
		d_n->type=s_n->type;
		d_n->info.word=s_n->info.word;
		d_n->pron_chain=s_n->pron_chain;
		d_n->hook=s_n->hook;

		//wtk_debug("v[%d]=%d wd=%p\n",i,(int)d_n->score,d_n->info.word);

		for(arc=s_n->foll;arc;arc=arc->farc)
		{
			idx=arc->end-src->lnodes;

			d_n2=dst->lnodes+idx;
			arc2=dst->larcs+aid;
			++aid;
			*arc2=*arc;

			wtk_lat_link_node(d_n,d_n2,arc2);
			//wtk_debug("idx=%d,%d->%d\n",idx,(int)(arc2->start-dst->lnodes),(int)(arc2->end-dst->lnodes));
		}

	}
	return dst;
}

wtk_lat_t* wtk_lat_dup(wtk_lat_t *lat,wtk_heap_t *h)
{
	wtk_netnode_t *n,*n2;
	wtk_lat_t *cpy;
	int i;

	cpy=wtk_lat_new_h(h);
	cpy->initial=lat->initial;
	cpy->final=lat->final;
	cpy->initial.nlinks=0;
	cpy->final.nlinks=0;
	cpy->initial.net_inst=0;
	cpy->final.net_inst=0;
	lat->initial.net_inst=(void*)&(cpy->initial);
	lat->final.net_inst=(void*)&(cpy->final);
	n=lat->chain;
	while(n)
	{
		n->net_inst=(void*)wtk_netnode_dup(n,h);
		n=n->chain;
	}
	n=lat->chain;
	wtk_netnode_dup_link(&(cpy->initial),&(lat->initial),h);
	wtk_netnode_dup_link(&(cpy->final),&(lat->final),h);
	while(n)
	{
		n2=(wtk_netnode_t*)n->net_inst;
		wtk_netnode_dup_link(n2,n,h);
		n2->chain=n->chain?((wtk_netnode_t*)n->chain->net_inst):0;
		n=n->chain;
	}
	cpy->chain=(wtk_netnode_t*)lat->chain->net_inst;
	n=lat->chain;
	while(n)
	{
		n->net_inst=0;
		n=n->chain;
	}
	lat->initial.net_inst=0;
	lat->final.net_inst=0;
	n=cpy->chain;
	while(n)
	{
		n->net_inst=0;
		for(i=0;i<n->nlinks;++i)
		{
			n->links[i].node->net_inst=0;
		}
		n=n->chain;
	}
	//wtk_netnode_print(&(lat->initial));
	//wtk_lat_print_net(cpy);
	return cpy;
}

void wtk_lat_clean_node_hook(wtk_lat_t *lat)
{
	int i;

	for(i=0;i<lat->nn;++i)
	{
		lat->lnodes[i].hook=NULL;
		lat->lnodes[i].n=0;
		lat->lnodes[i].score=0;
	}
}

wtk_lnode_t* wtk_lat_start_node(wtk_lat_t *lat)
{
	int i;

	for(i=0;i<lat->nn;++i)
	{
		if(!lat->lnodes[i].pred)
		{
			return lat->lnodes+i;
		}
	}
	return 0;
}

wtk_lnode_t* wtk_lat_end_node(wtk_lat_t *lat)
{
	int i;

	for(i=0;i<lat->nn;++i)
	{
		if(!lat->lnodes[i].foll)
		{
			return lat->lnodes+i;
			/*
			wtk_debug("found end node [i=%d,%.*s]\n",i,
					lat->lnodes[i].info.fst_wrd->len,
					lat->lnodes[i].info.fst_wrd->data);
			if(nx)
			{
				exit(0);
			}
			nx=lat->lnodes+i;
			*/
		}
	}
	return 0;
}

int wtk_lnode_next_len(wtk_lnode_t *n)
{
	wtk_larc_t *arc;
	int cnt;

	for(cnt=0,arc=n->foll;arc;arc=arc->farc,++cnt)
	{

	}
	return cnt;
}

void wtk_lat_print_net(wtk_lat_t *lat)
{
	wtk_netnode_t *n;

	n=lat->chain;
	while(n)
	{
		wtk_netnode_print(n);
		n=n->chain;
	}
	wtk_netnode_print(&(lat->initial));
}

wtk_lnode_t* wtk_lat_new_lnode(wtk_lat_t *lat)
{
	wtk_lnode_t *ln;

	ln=(wtk_lnode_t*)wtk_heap_zalloc(lat->heap,sizeof(wtk_lnode_t));
	ln->index=(lat->nn++);
	//++lat->nn;
	return ln;
}

wtk_larc_t* wtk_lat_new_arc(wtk_lat_t *lat)
{
	wtk_larc_t *arc;

	arc=(wtk_larc_t*)wtk_heap_zalloc(lat->heap,sizeof(wtk_larc_t));
	++lat->na;
	arc->index = lat->na;
	return arc;
}

#define wtk_lnode_prev_has_s(n,s) wtk_lnode_prev_has(n,s,sizeof(s)-1)

wtk_lnode_t* wtk_lnode_prev_has(wtk_lnode_t *n,char *data,int len)
{
	wtk_larc_t *arc;

	if(n->info.word)
	{
		if(wtk_string_cmp(n->info.word->name,data,len)==0)
		{
			return n;
		}
	}
	for(arc=n->pred;arc;arc=arc->parc)
	{
		n=wtk_lnode_prev_has(arc->start,data,len);
		if(n)
		{
			return n;
		}
	}
	return NULL;
}

void wtk_lat_link_node2(wtk_lat_t *lat,wtk_lnode_t *s,wtk_lnode_t *e)
{
	wtk_larc_t *arc;

	arc=wtk_lat_new_arc(lat);
	arc->start=s;
	arc->end=e;

	arc->farc=s->foll;
	s->foll=arc;

	arc->parc=e->pred;
	e->pred=arc;

	if(0)
	{
		if(wtk_lnode_prev_has_s(s,"麟") && wtk_string_cmp_s(e->info.word->name,"里")==0)
		{
			wtk_lnode_print_prev(e);
			exit(0);
		}
	}
	/*
	if(s->info.word && e->info.word)
	{
		if((wtk_string_cmp_s(s->info.word->name,"<s>")==0) &&(wtk_string_cmp_s(e->info.word->name,"把")==0))
		{
			wtk_debug("[%.*s]=[%.*s]\n",s->info.word->name->len,s->info.word->name->data,
					e->info.word->name->len,e->info.word->name->data
					);
			exit(0);
		}
	}*/
}

typedef struct
{
	int arc_index;
}wtk_lat_link_info_t;

void wtk_lat_print_link_node(wtk_lnode_t *node,wtk_strbuf_t *buf1,wtk_strbuf_t *buf2,
		wtk_lat_link_info_t *info)
{
	wtk_larc_t *arc;

	//I=0 t=0.00 W=<s>
	if(node->hook)
	{
		return;
	}
	if(node->info.word)
	{
		wtk_strbuf_push_f(buf1,"I=%d t=%.2f W=%.*s\n",node->index,node->time,
			node->info.word->name->len,
			node->info.word->name->data);
	}else
	{
		wtk_strbuf_push_f(buf1,"I=%d t=%.2f W=!NULL\n",node->index,node->time);
	}
	for(arc=node->foll;arc;arc=arc->farc)
	{
		//J=0 S=0 E=2 a=-1791.541138 l=2.000000
		wtk_strbuf_push_f(buf2,"J=%d S=%d E=%d a=%f l=%f\n",info->arc_index++,
				arc->start->index,arc->end->index,arc->aclike,arc->lmlike);
		wtk_lat_print_link_node(arc->end,buf1,buf2,info);
	}
	node->hook=node;
}

void wtk_lat_print_link(wtk_lat_t *lat,FILE *log)
{
	wtk_strbuf_t *buf1;
	wtk_strbuf_t *buf2;
	wtk_lat_link_info_t info;

	buf1=wtk_strbuf_new(1024,1);
	buf2=wtk_strbuf_new(1024,1);
	info.arc_index=0;
	wtk_lat_print_link_node(lat->ls,buf1,buf2,&info);
	fprintf(log,"VERSION=1.0\n");
	fprintf(log,"lmscale=%.2f wdpenalty=%.2f prscale=%.2f acscale=%.2f\n",lat->lmscale,lat->wdpenalty,lat->prscale,lat->acscale);
	fprintf(log,"N=%d L=%d\n",lat->nn,lat->na);
	fprintf(log,"%.*s",buf1->pos,buf1->data);
	fprintf(log,"%.*s",buf2->pos,buf2->data);
	wtk_strbuf_delete(buf1);
	wtk_strbuf_delete(buf2);
}

int wtk_lat_create(wtk_lat_t *lat,int nodes,int arcs)
{
	//wtk_heap_t *heap=lat->heap;
	wtk_lnode_t *ln;
	wtk_larc_t *la;
	int i;

	lat->nn=nodes;lat->na=arcs;
	lat->lnodes=wtk_calloc(nodes,sizeof(wtk_lnode_t));
	lat->larcs=wtk_calloc(arcs,sizeof(wtk_larc_t));
	//lat->lnodes=(wtk_lnode_t*)wtk_heap_zalloc(heap,nodes*sizeof(wtk_lnode_t));
	//lat->larcs=(wtk_larc_t*)wtk_heap_zalloc(heap,arcs*sizeof(wtk_larc_t));
	for(i=0,ln=lat->lnodes;i<nodes;++i,++ln)
	{
		ln->score=0;
	}
	for(i=0,la=lat->larcs;i<arcs;++i,++la)
	{
		la->lmlike=la->aclike=la->prlike=la->score=0;
	}
	return 0;
}

int wtk_lat_clean(wtk_lat_t *lat)
{
	if(lat->lnodes)
	{
		wtk_free(lat->lnodes);
		lat->lnodes=0;
	}
	if(lat->larcs)
	{
		wtk_free(lat->larcs);
		lat->larcs=0;
	}
	return 0;
}

wtk_netnode_t* wtk_netnode_wn(wtk_netnode_t *n)
{
	wtk_netnode_t *w=0;
	int i;

	if(n->type==n_word){w=n;goto end;}
	for(i=0;i<n->nlinks;++i)
	{
		w=n->links[i].node;
		if(w){break;}
	}
	if(!w){goto end;}
	if(w->type==n_word){goto end;}
	w=wtk_netnode_wn(w);
end:
	return w;
}

wtk_string_t* wtk_netnode_name(wtk_netnode_t *n)
{
	static wtk_string_t null=wtk_string("NULL");
	wtk_string_t *name;

	if(n->type==n_word)
	{
		if(n->info.pron)
		{
			name=n->info.pron->outsym;
		}else
		{
			name=&null;
		}
	}else
	{
		name=n->info.hmm->name;
	}
	return name;
}

char* wtk_netnode_name2(wtk_netnode_t *n)
{
	wtk_string_t *name;

	name=wtk_netnode_name(n);
#ifdef WIN32
    return name->data;
#else
	return	gbk_to_utf8_3(name->data,name->len);
#endif
}

/*
 * lattice will sorted like this:
 *
 *           o(2)  - o(3)
 *          /            \
 *        o(1)            o(6)
 *       /  \            / \
 *      /    o(4)  - o(5)   \
 *    o(0)                   o(13)
 *      \    o(8)  - o(9)   /
 *       \  /            \ /
 *        o(7)            o(12)
 *          \            /
 *           o(10) - o(11)
 *
 */
void wtk_lnode_mark_back(wtk_lnode_t *ln,int *nn)
{
	wtk_larc_t *la;

	ln->n=-2;
	for(la=ln->pred;la;la=la->parc)
	{
		if(la->start->n==-1)
		{
			wtk_lnode_mark_back(la->start,nn);
		}
	}
	ln->n=(*nn)++;
	//wtk_lnode_print(ln);
}

void wtk_lat_disable_lmlike(wtk_lat_t *lat)
{
	int j;

	for(j=0;j<lat->na;++j)
	{
		lat->larcs[j].lmlike=0;
	}
}

void wtk_netnode_print3(wtk_netnode_t *n)
{
	char *name;
	char *type;
	static int x=0;

	wtk_debug("################ %p,x=%d ##############\n",n,++x);
	name=wtk_netnode_name2(n);
	if(n->type & n_hmm)
	{
		type="hmm";
	}else
	{
		type="word";
	}
	printf("type: %s\n",type?type:"null");
	if(n->type==n_word)
	{
		type="outsym";
	}else
	{
		type="hmm";
		printf("state:\t %d\n",n->info.hmm->num_state);
	}
	printf("type:\t%d\n",n->type);
	printf("nlinks:\t%d\n",n->nlinks);
	name=wtk_netnode_name2(n);
	printf("%s:\t%s\n",type,name);
	if(n->net_inst)
	{
		printf("wdlik:\t%f\n",n->net_inst->wdlk);
		printf("max:\t%f\n",n->net_inst->max);
		printf("exitlike:\t%f\n",n->net_inst->exit->tok.like);
		printf("exitlm:\t%f\n",n->net_inst->exit->tok.lm);
	}
	printf("#############################################\n");
}

void wtk_netnode_print(wtk_netnode_t *n)
{
	char *name;
	char *type;
	int i;
	static int j=0;

	wtk_debug("################  %d: %p ##############\n",++j,n);
	//printf("############ NetNode ( %p:%d ) #############\n",n,n->type);
	//fflush(stdout);
	name=wtk_netnode_name2(n);
	if(n->type==n_word)
	{
		type="outsym";
	}else
	{
		type="hmm";
		printf("state:\t %d\n",n->info.hmm->num_state);
	}
	printf("type:\t%d\n",n->type);
	printf("nliks:\t%d\n",n->nlinks);
	printf("%s:\t%s\n",type,name);
	if(n->nlinks>0)
	{
		for(i=0;i<n->nlinks;++i)
		{
			name=wtk_netnode_name2(n->links[i].node);
			if(name)
			{
				printf("link[%d/%d]:\t%s(%p:%d,%f)\n",i,n->nlinks,name,n->links[i].node,n->links[i].node->type,n->links[i].like);
			}else
			{
				printf("link[%d/%d]:\t!NULL(%p:%d)\n",i,n->nlinks,n->links[i].node,n->links[i].node->type);
			}
		}
	}
	if(n->net_inst)
	{
		printf("wdlik:\t%f\n",n->net_inst->wdlk);
		printf("max:\t%f\n",n->net_inst->max);
		printf("exitlike:\t%f\n",n->net_inst->exit->tok.like);
		printf("exitlm:\t%f\n",n->net_inst->exit->tok.lm);
	}
	printf("#############################################\n");
}


void wtk_netnode_print_nxtpath(wtk_netnode_t *n)
{
	char *name;
	char *type;
	wtk_nxtpath_t *nxt_path;
	wtk_path_t *path;
	static int x=0;

	wtk_debug("################  %p(np=%d) ##############\n",n,++x);
	name=wtk_netnode_name2(n);
	if(n->type==n_word)
	{
		type="word";
	}else
	{
		type="hmm";
		printf("state:\t %d\n",n->info.hmm->num_state);
	}
	printf("%s:\t%s\n",type,name);
	printf("nliks:\t%d\n",n->nlinks);
	path=n->net_inst->exit->tok.path;
	if(path)
	{
		name=wtk_netnode_name2(path->node);
		printf("path: %s\n",name);
		for(nxt_path=path->chain;nxt_path;nxt_path=nxt_path->chain)
		{
			name=wtk_netnode_name2(nxt_path->prev->node);
			printf("nxt_path: %s\n",name);
		}
	}
	if(n->net_inst)
	{
		printf("wdlik:\t%f\n",n->net_inst->wdlk);
		printf("max:\t%f\n",n->net_inst->max);
		printf("exitlike:\t%f\n",n->net_inst->exit->tok.like);
		printf("exitlm:\t%f\n",n->net_inst->exit->tok.lm);
	}
	printf("#############################################\n");
}



void wtk_larc_print(wtk_larc_t *a,int i)
{
	wtk_string_t *s,*e;

	s=wtk_lnode_name(a->start);
	e=wtk_lnode_name(a->end);
	printf("arc(%d,%p):\t %*.*s(%p) -> %*.*s(%p)\n",i,a,s->len,s->len,s->data,a->start,e->len,e->len,e->data,a->end);
}

void wtk_proninst_print(wtk_proninst_t *inst)
{
	int i;

	printf("pron:\t%*.*s\n",inst->pron->outsym->len,inst->pron->outsym->len,inst->pron->outsym->data);
	printf("(%d):",inst->nphones);
	for(i=0;i<inst->nphones;++i)
	{
		printf(" %*.*s",inst->phones[i]->name->len,inst->phones[i]->name->len,inst->phones[i]->name->data);
	}
	printf("\n");
}

void wtk_lat_print(wtk_lat_t *lat)
{
#define wtk_lat_print_flt(lat,field) printf("%s:\t%.3f\n",STR(field),lat->field)
#define wtk_lat_print_int(lat,field) printf("%s:\t%d\n",STR(field),lat->field)
	char *p;
	int n;

	if(lat->name)
	{
		p=lat->name->data;n=lat->name->len;
	}else
	{
		p="main";n=4;
	}
	printf("############## lattice (%*.*s) ###############\n",n,n,p);
	wtk_lat_print_flt(lat,lmscale);
	wtk_lat_print_flt(lat,acscale);
	wtk_lat_print_flt(lat,wdpenalty);
	wtk_lat_print_flt(lat,prscale);
	wtk_lat_print_flt(lat,logbase);
	wtk_lat_print_flt(lat,tscale);
	wtk_lat_print_int(lat,nn);
	wtk_lat_print_int(lat,na);
	for(n=0;n<lat->nn;++n)
	{
		wtk_lnode_print(lat->lnodes+n);
	}
	for(n=0;n<lat->na;++n)
	{
		wtk_larc_print(lat->larcs+n,n);
	}
}

void wtk_lat_print2(wtk_lat_t *lat,FILE* file)
{
	//wtk_string_t null=wtk_string("!NULL");
	//wtk_string_t *name;
	int i,j,k;
	int *v;

	v=(int*)malloc(sizeof(int)*lat->na);
	fprintf(file,"VERSION=1.0\n");
	fprintf(file,"N=%-4d L=%-5d\n",lat->nn,lat->na);
	for(i=0;i<lat->nn;++i)
	{
#ifdef WIN32
        fprintf(file,"I=%d W=%s\n",i,wtk_lnode_name(lat->lnodes+i));
#else
        wtk_string_t *v;

        v=wtk_lnode_name(lat->lnodes+i);
        if(str_is_utf8((const unsigned char*)v->data,v->len))
        {
        	fprintf(file,"I=%d W=%.*s\n",i,v->len,v->data);
        }else
        {
        	fprintf(file,"I=%d W=%s\n",i,wtk_lnode_name2(lat->lnodes+i));
        }
#endif
	}
	i=k=0;
	while(k<lat->na)
	{
		for(j=0;j<lat->na;++j)
		{
			if((lat->larcs[j].end-lat->lnodes)==i)
			{
				v[k++]=j;
			}
		}
		++i;
	}
	for(j=0;j<lat->na;++j)
	{
		i=v[j];
		fprintf(file,"J=%d S=%ld E=%ld l=%.2f a=%.2f\n",j,(long)(lat->larcs[i].start-lat->lnodes),
				(long)(lat->larcs[i].end-lat->lnodes),lat->larcs[i].lmlike,lat->larcs[i].score);
		//printf("J=%d S=%ld E=%ld l=%.2f\n",i,(long)(lat->larcs[i].start-lat->lnodes),(long)(lat->larcs[i].end-lat->lnodes),lat->larcs[i].lmlike);
	}
	free(v);
}

int wtk_lnode_is_prev(wtk_lnode_t *n1,wtk_lnode_t *n2)
{
	wtk_larc_t *arc;
	int b;

	for(arc=n1->foll;arc;arc=arc->farc)
	{
		if(arc->end==n2)
		{
			return 1;
		}
		b=wtk_lnode_is_prev(arc->end,n2);
		if(b)
		{
			return 1;
		}
	}
	return 0;
}

float wtk_lat_cmp_node(wtk_lat_t *lat,int *src,int *dst)
{
	double t,s;
	int s1,s2;

	s1=*src;s2=*dst;
	t=lat->lnodes[s1].time-lat->lnodes[s2].time;
	s=lat->lnodes[s1].score-lat->lnodes[s2].score;
	if(t==0)
	{
		//char *p;

		//p=wtk_lnode_name2(lat->lnodes+s1);
		//wtk_debug("%d:%s\n",s1,p);
		//p=wtk_lnode_name2(lat->lnodes+s2);
		//wtk_debug("%d:%s\n",s2,p);
		//wtk_debug("s=%f s1=%d s2=%d\n",s,s1,s2);
		if(s==0)
		{
			if(wtk_lnode_is_prev(lat->lnodes+s1,lat->lnodes+s2))
			{
				return -1;
			}
			if(wtk_lnode_is_prev(lat->lnodes+s2,lat->lnodes+s1))
			{
				return 1;
			}
			return (s1-s2);
		}else if(s>0)
		{
			return 1;
		}else
		{
			return -1;
		}
	}else if(t>0)
	{
		return 1;
	}else
	{
		return -1;
	}
}

float wtk_lat_cmp_arc(wtk_lat_t *lat,int *src,int *dst)
{
	int s1,s2,j,k;

	s1=*src;s2=*dst;
	j=lat->larcs[s1].end->n-lat->larcs[s2].end->n;
	k=lat->larcs[s1].start->n-lat->larcs[s2].start->n;
	if(k==0 && j==0)
	{
		return (s1-s2);
	}else if(j==0)
	{
		return k;
	}else
	{
		return j;
	}
}

#ifndef WIN32

void wtk_lat_write(wtk_lat_t *lat,char *fn)
{
	FILE* f;

	wtk_debug("write: %s\n",fn);
	f=fopen(fn,"w");
	wtk_lat_print3(lat,f);
	fclose(f);
}


void wtk_lat_print3(wtk_lat_t *lat,FILE* file)
{
	wtk_string_t null=wtk_string("!NULL");
	int i,j;
	int *order;
	int *rorder;
	wtk_lnode_t *ln;
	wtk_larc_t *la;
	int s,e;
	wtk_string_t *name;

	fprintf(file,"VERSION=1.0\n");
	fprintf(file,"N=%-4d L=%-5d\n",lat->nn,lat->na);
	rorder=(int*)malloc(sizeof(int)*lat->nn);
	order=(int*)malloc(sizeof(int)*(lat->nn<lat->na ? (lat->na+1):(lat->nn+1)));
	for(i=0;i<lat->nn;++i)
	{
		order[i]=i;
	}
	wtk_qsort2(order,lat->nn,sizeof(int),(wtk_qsort_cmp_f)wtk_lat_cmp_node,lat);
	for(i=0;i<lat->nn;++i)
	{
		j=order[i];
		rorder[j]=i;
		ln=lat->lnodes+j;
		ln->n=i;

		if(ln->info.word)
		{
			name=ln->info.word->name;
		}else
		{
			name=&null;
		}
		if(str_is_utf8((unsigned char*)name->data,name->len))
		{
			fprintf(file,"I=%d W=%*.*s\n",i,name->len,name->len,name->data);
		}else
		{
			fprintf(file,"I=%d t=%.2f W=%s\n",i,ln->time,wtk_lnode_name2(ln));
		}
	}
	for(i=0;i<lat->na;++i)
	{
		order[i]=i;
	}
	wtk_qsort2(order,lat->na,sizeof(int),(wtk_qsort_cmp_f)wtk_lat_cmp_arc,lat);
	for(i=0;i<lat->na;++i)
	{
		la=lat->larcs+order[i];
		s=rorder[la->start-lat->lnodes];
		e=rorder[la->end-lat->lnodes];
		fprintf(file,"J=%d S=%d E=%d a=%.2f l=%.3f\n",i,s,e,la->aclike,la->lmlike);
	}
	free(order);
	free(rorder);
}
#endif

void wtk_lat_print_addr(wtk_lat_t *lat)
{
	int i;

	for(i=0;i<lat->nn;++i)
	{
		wtk_debug("v[%d]: node=%p,pred=%p,foll=%p\n",i,&(lat->lnodes[i]),
				lat->lnodes[i].pred,lat->lnodes[i].foll);
	}
	for(i=0;i<lat->na;++i)
	{
		wtk_debug("v[%d]: arc=%p,start=%p,end=%p\n",i,&(lat->larcs[i]),
				lat->larcs[i].start,lat->larcs[i].end);
	}
}



void wtk_lat_print4(wtk_lat_t *lat,FILE* file)
{
	int i,j;
	int *order;
	int *rorder;
	wtk_lnode_t *ln;
	wtk_larc_t *la;
	int s,e;
#ifdef DEBUG_X
	wtk_lnode_t **top_order;

	top_order=(wtk_lnode_t**)wtk_malloc(lat->nn*sizeof(wtk_lnode_t*));
	wtk_lat_top_sort(lat,top_order);
#endif
	fprintf(file,"VERSION=1.0\n");
	fprintf(file,"lmscale=%.2f wdpenalty=%.2f prscale=%.2f acscale=%.2f\n",
			lat->lmscale,lat->wdpenalty,lat->prscale,lat->acscale);
	fprintf(file,"N=%-4d L=%-5d\n",lat->nn,lat->na);
	rorder=(int*)malloc(sizeof(int)*lat->nn);
	order=(int*)malloc(sizeof(int)*(lat->nn<lat->na ? (lat->na+1):(lat->nn+1)));
	for(i=0;i<lat->nn;++i)
	{
		order[i]=i;
	}
	wtk_qsort2(order,lat->nn,sizeof(int),(wtk_qsort_cmp_f)wtk_lat_cmp_node,lat);
	for(i=0;i<lat->nn;++i)
	{
		j=order[i];
		rorder[j]=i;
		ln=lat->lnodes+j;
		ln->n=i;
		/*
		if(ln->info.word)
		{
			name=ln->info.word->name;
		}else
		{
			name=&null;
		}
		fprintf(file,"I=%d W=%*.*s\n",i,name->len,name->len,name->data);
		*/
		if(ln->info.fst_wrd )
		{
			fprintf(file,"I=%d t=%.2f W=%.*s v=1\n",i,ln->time,ln->info.fst_wrd->len,ln->info.fst_wrd->data);
		}else
		{
			fprintf(file,"I=%d t=%.2f W=!NULL\n",i,ln->time>=0?ln->time:0);
		}
	}
	for(i=0;i<lat->na;++i)
	{
		order[i]=i;
	}
	wtk_qsort2(order,lat->na,sizeof(int),(wtk_qsort_cmp_f)wtk_lat_cmp_arc,lat);
	for(i=0;i<lat->na;++i)
	{
		la=lat->larcs+order[i];
		s=rorder[la->start-lat->lnodes];
		e=rorder[la->end-lat->lnodes];
		//fprintf(file,"J=%d S=%d E=%d\n",i,s,e);
		fprintf(file,"J=%d S=%d E=%d a=%.2f l=%.3f\n",i,s,e,la->aclike,la->lmlike);
		//fprintf(file,"J=%d S=%d E=%d a=%.2f l=%.3f s=%.3f\n",i,s,e,la->aclike,la->lmlike,la->score);
		//fprintf(file,"J=%d S=%d E=%d a=%.2f\n",i,s,e,la->aclike);
	}
	free(order);
	free(rorder);
}


void wtk_lat_print5(wtk_lat_t *lat,FILE* file)
{
	//wtk_string_t null=wtk_string("!NULL");
	//wtk_string_t *name;
	int i,j,k;
	int *v;

	v=(int*)malloc(sizeof(int)*lat->na);
	fprintf(file,"VERSION=1.0\n");
	fprintf(file,"N=%-4d L=%-5d\n",lat->nn,lat->na);
	for(i=0;i<lat->nn;++i)
	{
#ifdef WIN32
        fprintf(file,"I=%d W=%s\n",i,wtk_lnode_name(lat->lnodes+i));
#else
        wtk_string_t *v;

        v=(lat->lnodes+i)->info.fst_wrd;
        fprintf(file,"I=%d W=%.*s\n",i,v->len,v->data);
#endif
	}
	i=k=0;
	while(k<lat->na)
	{
		for(j=0;j<lat->na;++j)
		{
			if((lat->larcs[j].end-lat->lnodes)==i)
			{
				v[k++]=j;
			}
		}
		++i;
	}
	for(j=0;j<lat->na;++j)
	{
		i=v[j];
		fprintf(file,"J=%d S=%ld E=%ld l=%.2f a=%.2f\n",j,(long)(lat->larcs[i].start-lat->lnodes),
				(long)(lat->larcs[i].end-lat->lnodes),lat->larcs[i].lmlike,lat->larcs[i].aclike);
		//printf("J=%d S=%ld E=%ld l=%.2f\n",i,(long)(lat->larcs[i].start-lat->lnodes),(long)(lat->larcs[i].end-lat->lnodes),lat->larcs[i].lmlike);
	}
	free(v);
}

void wtk_lnode_print(wtk_lnode_t *n)
{
	char *type="word";
	wtk_string_t *name;

	switch(n->type)
	{
	case WTK_LNODE_WORD:
		type="word";
		if(n->info.word)
		{
			name=n->info.word->name;
		}else
		{
			name=&null_name;
		}
		break;
	case WTK_LNODE_SUBLAT:
		type="sublat";
		name=n->info.lat->name;
		break;
	default:
		name=0;
		wtk_debug("unexpected node type(%d)\n",n->type);
		//exit(0);
		break;
	}
	//printf("############ LNODE #####################\n");
	printf("%s(%d):\t%*.*s\tscore=%.3f\ttime=%f\n",type,n->n,name->len,name->len,name->data,n->score,n->time);
	//printf("nodes:\t%d\n",n->info.word);
}

wtk_string_t* wtk_lnode_name(wtk_lnode_t *n)
{
	wtk_string_t *name=0;

	switch(n->type)
	{
	case WTK_LNODE_WORD:
		if(n->info.word)
		{
			name=n->info.word->name;
		}
		break;
	case WTK_LNODE_SUBLAT:
		name=n->info.lat->name;
		break;
	default:
		break;
	}
	if(!name)
	{
		name=&null_name;
	}
	return name;
}

#ifndef WIN32
char* wtk_lnode_name2(wtk_lnode_t *n)
{
	if(n->info.word)
	{
		return gbk_to_utf8_3(n->info.word->name->data,n->info.word->name->len);
	}else
	{
		return "!NULL";
	}
}
#else
char* wtk_lnode_name2(wtk_lnode_t *n)
{
	return "!NULL";
}
#endif

void wtk_lnode_print_prev2(wtk_lnode_t *n,int depth)
{
	wtk_larc_t *arc;
	int i;

	if(n->info.word)
	{
		for(i=0;i<depth;++i)
		{
			printf("  ");
		}
		printf("%.*s[%p n=%d index=%d]\n",n->info.word->name->len,n->info.word->name->data,n,n->info.word->npron,
				n->index);
	}
	for(arc=n->pred;arc;arc=arc->parc)
	{
		wtk_lnode_print_prev2(arc->start,depth+1);
	}
}

void wtk_lnode_print_prev(wtk_lnode_t *n)
{
	wtk_debug("========== prev ==========\n");
	wtk_lnode_print_prev2(n,0);
}


void wtk_lnode_print_next2(wtk_lnode_t *n,int depth)
{
	wtk_larc_t *arc;
	int i;

	wtk_debug("next \n");
	if(n->info.word)
	{
		for(i=0;i<depth;++i)
		{
			printf("  ");
		}
		printf("%.*s[%p n=%d index=%d]\n",n->info.word->name->len,n->info.word->name->data,n,n->info.word->npron,
				n->index);
	}
	for(arc=n->foll;arc;arc=arc->farc)
	{
		wtk_lnode_print_next2(arc->end,depth+1);
	}
}

void wtk_lnode_print_next(wtk_lnode_t *n)
{
	wtk_debug("============= next ==========\n");
	wtk_lnode_print_next2(n,0);
}

int wtk_lnode_input_arcs(wtk_lnode_t *n)
{
	wtk_larc_t *arc;
	int cnt;

	for(cnt=0,arc=n->pred;arc;arc=arc->parc,++cnt);
	return cnt;
}

int wtk_lnode_output_arcs(wtk_lnode_t *n)
{
	wtk_larc_t *arc;
	int cnt;

	for(cnt=0,arc=n->foll;arc;arc=arc->farc,++cnt);
	return cnt;
}

int wtk_lnode_has_same_output(wtk_lnode_t *n1,wtk_lnode_t *n2)
{
	wtk_larc_t *arc1,*arc2;
	int b=0;
	int find;

	//wtk_debug("%d-%d\n",n1->n,n2->n);
	if(n1->score==0)
	{
		n1->score=wtk_lnode_input_arcs(n1);
	}
	if(n2->score==0)
	{
		n2->score=wtk_lnode_input_arcs(n2);
	}
	if(n1->score!=n2->score)
	{
		goto end;
	}
	for(arc1=n1->foll;arc1;arc1=arc1->farc)
	{
		find=0;
		for(arc2=n2->foll;arc2;arc2=arc2->farc)
		{
			if(arc2->end==arc1->end)
			{
				find=1;
				break;
			}
		}
		if(find==0)
		{
			goto end;
		}
	}
	b=1;
end:
	return b;
}

int wtk_lnode_has_same_input(wtk_lnode_t *n1,wtk_lnode_t *n2)
{
	wtk_larc_t *arc1,*arc2;
	int b=0;
	int find;

	//wtk_debug("%d-%d\n",n1->n,n2->n);
	if(n1->n==0)
	{
		n1->n=wtk_lnode_input_arcs(n1);
	}
	if(n2->n==0)
	{
		n2->n=wtk_lnode_input_arcs(n2);
	}
	if(n1->n!=n2->n)
	{
		goto end;
	}
	for(arc1=n1->pred;arc1;arc1=arc1->parc)
	{
		find=0;
		for(arc2=n2->pred;arc2;arc2=arc2->parc)
		{
			if(arc2->start==arc1->start)
			{
				find=1;
				break;
			}
		}
		if(find==0)
		{
			goto end;
		}
	}
	b=1;
end:
	return b;
}
