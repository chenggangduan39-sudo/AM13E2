#define _GNU_SOURCE
#include "wtk_rnn_dec.h" 
#define MAX_GRAD 15.0
//#define RNN_EXP exp
//#define RNN_EXP FAST_EXP
#define RNN_EXP wtk_fast_exp
#define wtk_rnn5_norm_float(f) ((f>MAX_GRAD)?MAX_GRAD:((f<-MAX_GRAD)?-MAX_GRAD:f))


wtk_rnn_dec_t* wtk_rnn_dec_new(wtk_rnn_dec_cfg_t *cfg)
{
	wtk_rnn_dec_t* dec;

	dec=(wtk_rnn_dec_t*)wtk_malloc(sizeof(wtk_rnn_dec_t));
	dec->cfg=cfg;
	dec->start.neu_hid.f=NULL;
	dec->start.ngram=NULL;
	dec->start.wrd=cfg->tree->eos;
	dec->use_fix=cfg->syn->fix?1:0;
	if(dec->use_fix)
	{
		wtk_rnn_dec_fix_syn_t *syn=cfg->syn->fix;

		dec->fix_r1=(syn->input_hid->scale*cfg->max_hid_value);
		dec->fix_r2=dec->fix_r1/syn->input_wrd->scale;
		dec->fix_r1=1.0/dec->fix_r1;
		dec->fix_r3=1.0/(cfg->max_hid_value*syn->output_wrd->scale);
		dec->hid_size=syn->hid_size;
	}else
	{
		dec->hid_size=cfg->syn->fsyn->hid_size;
	}
	dec->hid_bytes=dec->hid_size*sizeof(int);
	wtk_queue_init(&(dec->env_q));
	return dec;
}

void wtk_rnn_dec_delete(wtk_rnn_dec_t *dec)
{
	wtk_rnn_dec_reset(dec);
	wtk_free(dec);
}

wtk_rnn_dec_env_t* wtk_rnn_dec_pop_env(wtk_rnn_dec_t *dec,wtk_hs_tree_wrd_t *wrd)
{
	wtk_rnn_dec_env_t *env;

	env=(wtk_rnn_dec_env_t*)wtk_malloc(sizeof(wtk_rnn_dec_env_t));
	env->ngram=NULL;
	env->use_fix=dec->use_fix;
	if(dec->use_fix)
	{
		env->neu_hid.i=wtk_mati_new(1,dec->hid_size);
	}else
	{
		env->neu_hid.f=wtk_matf_new(1,dec->hid_size);
	}
	env->wrd=wrd;
	wtk_queue_push(&(dec->env_q),&(env->q_n));
	return env;
}

void wtk_rnn_dec_env_cpy(wtk_rnn_dec_env_t *src,wtk_rnn_dec_env_t *dst)
{
	dst->wrd=src->wrd;
	if(src->use_fix)
	{
		if(src->neu_hid.i)
		{
			wtk_mati_cpy(src->neu_hid.i,dst->neu_hid.i);
		}else
		{
			wtk_mati_zero(dst->neu_hid.i);
		}
	}else
	{
		if(src->neu_hid.f)
		{
			wtk_matf_cpy(src->neu_hid.f,dst->neu_hid.f);
		}else
		{
			wtk_matf_zero(dst->neu_hid.f);
		}
	}
	dst->ngram=src->ngram;
}

void wtk_rnn_dec_env_delete(wtk_rnn_dec_env_t *env)
{
	if(env->use_fix)
	{
		wtk_mati_delete(env->neu_hid.i);
	}else
	{
		wtk_matf_delete(env->neu_hid.f);
	}
	wtk_free(env);
}

void wtk_rnn_dec_reset(wtk_rnn_dec_t *dec)
{
	wtk_queue_node_t *qn;
	wtk_rnn_dec_env_t *env;

	while(1)
	{
		qn=wtk_queue_pop(&(dec->env_q));
		if(!qn){break;}
		env=data_offset2(qn,wtk_rnn_dec_env_t,q_n);
		wtk_rnn_dec_env_delete(env);
	}
}



void wtk_rnn_dec_float_sigmoid(register float *f1,int n)
{
	register float f;
	register float *fe;

	fe=f1+n;
	while(f1<fe)
	{
		f=*(f1);
		*(f1++)=1/(1+RNN_EXP(-wtk_rnn5_norm_float(f)));
	}
}

float wtk_rnn_dec_calc_float(wtk_rnn_dec_t *r,wtk_rnn_dec_env_t *last,wtk_rnn_dec_env_t *cur)
{
	wtk_hs_tree_wrd_t *wrd;
	wtk_rnn_dec_fsyn_t *syn=r->cfg->syn->fsyn;
	int hid=syn->hid_size;
	int last_word;
	float *neu_hid,*last_neu_hid;
	float *pf;
	int j,k;
	float logp=0;
	int pos;
	float *fwrd;
	double f;
	int idx;

	last_word=last->wrd->wrd_index;
	if(last->neu_hid.f)
	{
		last_neu_hid=last->neu_hid.f->p;
	}else
	{
		last_neu_hid=NULL;
	}
	neu_hid=cur->neu_hid.f->p;
	if(last_neu_hid)
	{
		wtk_matf_vm_hid(neu_hid,last_neu_hid,syn->input_hid,0);
	}else
	{
		memset(neu_hid,0,hid*sizeof(float));
	}
	if(last_word>-1)
	{
		pf=syn->input_wrd->p+hid*last_word;
		for(j=0;j<hid;++j)
		{
			neu_hid[j]+=pf[j];
		}
	}
	wtk_rnn_dec_float_sigmoid(neu_hid,hid);
	wrd=cur->wrd;
	for(j=0;j<wrd->codelen;++j)
	{
		pos=wrd->point[j];
		fwrd=syn->output_wrd->p+pos*hid;
		//calc prob
		f=0;
		for(k=0;k<hid;++k)
		{
			f+=neu_hid[k]*fwrd[k];
		}
		if(r->cfg->use_tree_bin)
		{
			idx=(wrd->code[j/8]>>(7-j%8))&0x1;
		}else
		{
			idx=wrd->code[j];
		}
		//wtk_debug("v[%d]=%d\n",j,idx);
		f=log10(1+(idx==1?RNN_EXP(f):RNN_EXP(-f)));
		logp+=f;
	}
	//wtk_debug("[%.*s]\n",wrd->name->len,wrd->name->data);	//10010111001111
	//exit(0);
	return -logp;
}


void wtk_mati_vm_hid_x(int *dst,int *src,wtk_matb_t *m,int add)
{
	int i,j;
	int xi;

	if(add==0)
	{
		memset(dst,0,m->row*sizeof(int));
	}
	for(i=0;i<m->row;++i)
	{
		xi=i*m->col;
		for(j=0;j<m->col;++j)
		{
			dst[j]+=(src[i]*m->p[xi+j]);
		}
	}
}


void wtk_mati_vm_hid(int *dst,int *src,wtk_matb_t *m,int add)
{
	int i;
	signed char *fp,*fp2,*fp3,*fp4;
	int f,g,h,t;
	int row;
	int row2;
	int col=m->col;
	int *f1,*fe;

	fp=m->p;
	row=m->row;
	row2=(row>>2)<<2;
	if(add==0)
	{
		memset(dst,0,sizeof(int)*row);
	}
	fe=dst+col;
	for(i=0;i<row2;i+=4)
	{
		f=src[i];
		g=src[i+1];
		h=src[i+2];
		t=src[i+3];
		fp2=fp+col;
		fp3=fp2+col;
		fp4=fp3+col;
		f1=dst;
		while(f1<fe)
		{
			*(f1++)+=f* (*(fp++)) + g* (*(fp2++)) + h * (*(fp3++)) + t * (*(fp4++));
		}
		fp=fp4;
	}
	for(;i<row;++i)
	{
		f=src[i];
		f1=dst;
		while(f1<fe)
		{
			*(f1++)+=f* (*(fp++));
		}
	}
}

void wtk_rnn_dec_int_sigmoid(register int *vs,int n,int max_v,float scale)
{
	register int *ve;
	register float f;

	ve=vs+n;
	while(vs<ve)
	{
		f=*vs * scale;
		*(vs++)=max_v/(1+RNN_EXP(-wtk_rnn5_norm_float(f)));
	}
}

float wtk_rnn_dec_calc_fix(wtk_rnn_dec_t *r,wtk_rnn_dec_env_t *last,wtk_rnn_dec_env_t *cur)
{
	wtk_hs_tree_wrd_t *wrd;
	wtk_rnn_dec_fix_syn_t *syn=r->cfg->syn->fix;
	int hid=syn->hid_size;
	int last_word;
	int *neu_hid,*last_neu_hid;
	signed char *pf;
	int j,k;
	float logp=0;
	int pos;
	signed char *fwrd;
	double f;
	int idx;
	int i,l;
	float r2=r->fix_r2;

	last_word=last->wrd->wrd_index;
	if(last->neu_hid.i)
	{
		last_neu_hid=last->neu_hid.i->p;
	}else
	{
		last_neu_hid=NULL;
	}
	//wtk_debug("last_neu_hid=%p last=%p cur=%p\n",last_neu_hid,last,cur);
	neu_hid=cur->neu_hid.i->p;
	if(last_neu_hid)
	{
		wtk_mati_vm_hid(neu_hid,last_neu_hid,syn->input_hid,0);
	}else
	{
		memset(neu_hid,0,r->hid_bytes);
	}
	if(last_word>-1)
	{
		pf=syn->input_wrd->p+hid*last_word;
		for(j=0;j<hid;++j)
		{
			neu_hid[j]+=pf[j]*r2;
		}
	}
	wtk_rnn_dec_int_sigmoid(neu_hid,hid,r->cfg->max_hid_value,r->fix_r1);
	wrd=cur->wrd;
	for(j=0,i=0,l=0;j<wrd->codelen;++j)
	{
		pos=wrd->point[j];
		fwrd=syn->output_wrd->p+pos*hid;
		//calc prob
		f=0;
		for(k=0;k<hid;++k)
		{
			f+=neu_hid[k]*fwrd[k];
		}
		f=f*r->fix_r3;
		idx=(wrd->code[i]>>(7-l))&0x1;
		//wtk_debug("idx1=%d j=%d i=%d l=%d\n",idx,j,i,l);
		++l;
		if(l>7)
		{
			l=0;
			++i;
		}
		//idx=(wrd->code[j/8]>>(7-j%8))&0x1;
		//wtk_debug("idx2=%d j=%d i=%d l=%d\n",idx,j,j/8,j%8);
		//wtk_debug("v[%d]=%d\n",j,idx);
		f=log10(1+(idx==1?RNN_EXP(f):RNN_EXP(-f)));
		logp+=f;
	}
	//wtk_debug("[%.*s]\n",wrd->name->len,wrd->name->data);	//001111000
//	{
//		static int ki=0;
//
//		++ki;
//		wtk_debug("[%.*s => %.*s]=%f\n",last->wrd->name->len,last->wrd->name->data,cur->wrd->name->len,cur->wrd->name->data,-logp);
//		if(ki==2)
//		{
//			exit(0);
//		}
//	}
	return -logp;
}


float wtk_rnn_dec_calc(wtk_rnn_dec_t *r,wtk_rnn_dec_env_t *last,wtk_rnn_dec_env_t *cur)
{
	if(r->use_fix)
	{
		return wtk_rnn_dec_calc_fix(r,last,cur);
	}else
	{
		return  wtk_rnn_dec_calc_float(r,last,cur);
	}
}

wtk_hs_tree_wrd_t* wtk_rnn_dec_next_word(wtk_rnn_dec_t *dec,wtk_source_t *src,wtk_strbuf_t *buf)
{
	wtk_hs_tree_t *tree=dec->cfg->tree;
	wtk_hs_tree_wrd_t *wrd=NULL;
	int nl,eof;
	int ret;

	eof=0;
	ret=wtk_source_skip_sp2(src,&nl,&eof);
	//wtk_debug("ret=%d nl=%d\n",ret,nl);
	if(eof)
	{
		goto end;
	}
	if(ret!=0)
	{
		ret=0;
		goto end;
	}
	if(nl)
	{
		return tree->eos;
	}
	ret=wtk_source_read_normal_string(src,buf);
	if(ret!=0)
	{
		ret=0;
		goto end;
	}

	if(buf->data[0]=='/' || (wtk_str_equal_s(buf->data,buf->pos,"<s>")) ||wtk_str_equal_s(buf->data,buf->pos,"</s>"))
	{
		return wtk_rnn_dec_next_word(dec,src,buf);
	}
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	wrd=wtk_hs_tree_get_word(tree,buf->data,buf->pos);
	if(!wrd)
	{
		wrd=tree->oov;
	}
end:
	return wrd;
}


int wtk_rnn_dec_eval(wtk_rnn_dec_t *r,wtk_source_t *src)
{
	wtk_hs_tree_t *tree=r->cfg->tree;
	wtk_strbuf_t *buf;
	wtk_hs_tree_wrd_t *wrd;
	int run=1;
	int good=1;
	wtk_rnn_dec_env_t *last=&(r->start);
	wtk_rnn_dec_env_t *cur;
	int wrds;
	float f,tmp,logp;
	unsigned int counter=0;
	float entropy,ppl;
	int snt=0;

	wrds=0;
	logp=0;
	tmp=0;
	buf=wtk_strbuf_new(256,1.0);
	while(run)
	{
		wrd=wtk_rnn_dec_next_word(r,src,buf);
		if(!wrd)
		{
			run=0;
			wrd=tree->eos;
		}
		if(wrd==tree->oov)
		{
			good=0;
		}
		if(good)
		{
			++wrds;
			cur=wtk_rnn_dec_pop_env(r,wrd);
			f=wtk_rnn_dec_calc(r,last,cur);
			//wtk_debug("[%.*s]=%f\n",cur->wrd->name->len,cur->wrd->name->data,f);
			tmp+=f;
			last=cur;
		}
		if(wrd==tree->eos)
		{
			++snt;
			if(snt%10000==0)
			{
				wtk_debug("snt=%d\n",snt);
//				entropy=-logp/log10(2)/counter;
//				ppl=exp10(-logp/(counter));
//				wtk_debug("counter=%d logp=%f entropy=%f ppl=%f\n",counter,logp,entropy,ppl);
//				exit(0);
			}
			wtk_rnn_dec_reset(r);
			if(good)
			{
//				ppl=exp10(-tmp/(wrds));
//				wtk_debug("ppl=%f\n",ppl);
				logp+=tmp;
				counter+=wrds;
			}
			tmp=0;
			wrds=0;
			last=&(r->start);
			good=1;
		}
	}
	wtk_strbuf_delete(buf);
#if defined __UNIX__
	entropy=-logp/log10(2)/counter;
	ppl=exp10(-logp/(counter));
	wtk_debug("counter=%d logp=%f entropy=%f ppl=%f\n",counter,logp,entropy,ppl);
#else
	entropy=0;
	ppl=0;
	wtk_debug("counter=%d logp=%f entropy=%f ppl=%f\n",counter,logp,entropy,ppl);
#endif
	return 0;
}


void wtk_rnn_dec_test(wtk_rnn_dec_t *dec,char *fn)
{
	 wtk_source_load_file(dec,(wtk_source_load_handler_t)wtk_rnn_dec_eval,fn);
}
