#define USE_FOR_WVITE
#include "wtk_hmmset.h"
#include <ctype.h>

/* Symbol for <Symbol> in HMM file*/
typedef enum Symbol{   /* Only a character big !! */
	BEGINHMM, USEMAC, ENDHMM, NUMMIXES,
	NUMSTATES, STREAMINFO, VECSIZE,
	NDUR, PDUR, GDUR, RELDUR, GENDUR,
	DIAGCOV,  FULLCOV, XFORMCOV,
	STATE, TMIX, MIXTURE, STREAM, SWEIGHTS,
	MEAN, VARIANCE, INVCOVAR, XFORM, GCONST,
	DURATION, INVDIAGCOV, TRANSP, DPROB, LLTCOV, LLTCOVAR,
	PROJSIZE,
	XFORMKIND=90, PARENTXFORM, NUMXFORMS, XFORMSET,
	LINXFORM, OFFSET, BIAS, LOGDET, BLOCKINFO, BLOCK, BASECLASS,
	CLASS, XFORMWGTSET, CLASSXFORM, MMFIDMASK, PARAMETERS,
	NUMCLASSES, ADAPTKIND, PREQUAL, INPUTXFORM,
	RCLASS=110, REGTREE, NODE, TNODE,
	HMMSETID=119,
	PARMKIND=120,
	MACRO, EOFSYM, NULLSYM
} Symbol;

typedef struct {
	wtk_string_t name;	/*Symbol Name*/
	Symbol sym;/*Symbol Value*/
}wtk_symap_t;

wtk_symap_t symMap[] =
{
{ wtk_string("BEGINHMM"), BEGINHMM },
{ wtk_string("USE"), USEMAC },
{ wtk_string("ENDHMM"), ENDHMM },
{ wtk_string("NUMMIXES"), NUMMIXES },
{ wtk_string("NUMSTATES"), NUMSTATES },
{ wtk_string("STREAMINFO"), STREAMINFO },
{ wtk_string("VECSIZE"), VECSIZE },
{ wtk_string("NULLD"), NDUR },
{ wtk_string("POISSOND"), PDUR },
{ wtk_string("GAMMAD"), GDUR },
{ wtk_string("RELD"), RELDUR },
{ wtk_string("GEND"), GENDUR },
{ wtk_string("DIAGC"), DIAGCOV },
{ wtk_string("FULLC"), FULLCOV },
{ wtk_string("XFORMC"), XFORMCOV },
{ wtk_string("STATE"), STATE },
{ wtk_string("TMIX"), TMIX },
{ wtk_string("MIXTURE"), MIXTURE },
{ wtk_string("STREAM"), STREAM },
{ wtk_string("SWEIGHTS"), SWEIGHTS },
{ wtk_string("MEAN"), MEAN },
{ wtk_string("VARIANCE"), VARIANCE },
{ wtk_string("INVCOVAR"), INVCOVAR },
{ wtk_string("XFORM"), XFORM },
{ wtk_string("GCONST") , GCONST },
{ wtk_string("DURATION"), DURATION },
{ wtk_string("INVDIAGC"), INVDIAGCOV },
{ wtk_string("TRANSP"), TRANSP },
{ wtk_string("DPROB"), DPROB },
{ wtk_string("LLTC"), LLTCOV },
{ wtk_string("LLTCOVAR"), LLTCOVAR },
{ wtk_string("PROJSIZE"), PROJSIZE},
{ wtk_string("RCLASS"), RCLASS },
{ wtk_string("REGTREE"), REGTREE },
{ wtk_string("NODE"), NODE },
{ wtk_string("TNODE"), TNODE },
{ wtk_string("HMMSETID"), HMMSETID },
{ wtk_string("PARMKIND"), PARMKIND },
{ wtk_string("MACRO"), MACRO },
{ wtk_string("EOF"), EOFSYM },
/* Transformation symbols */
{wtk_string("XFORMKIND"), XFORMKIND } ,
{wtk_string("PARENTXFORM"), PARENTXFORM },
{wtk_string("NUMXFORMS"), NUMXFORMS },
{wtk_string("XFORMSET"), XFORMSET },
{wtk_string("LINXFORM"), LINXFORM },
{wtk_string("OFFSET"), OFFSET },
{wtk_string("BIAS"), BIAS },
{wtk_string("LOGDET"), LOGDET},
{wtk_string("BLOCKINFO"), BLOCKINFO },
{wtk_string("BLOCK"), BLOCK },
{wtk_string("BASECLASS"), BASECLASS },
{wtk_string("CLASS"), CLASS },
{wtk_string("XFORMWGTSET"), XFORMWGTSET },
{wtk_string("CLASSXFORM"), CLASSXFORM },
{wtk_string("MMFIDMASK"), MMFIDMASK },
{wtk_string("PARAMETERS"), PARAMETERS },
{wtk_string("NUMCLASSES"), NUMCLASSES },
{wtk_string("ADAPTKIND"), ADAPTKIND },
{wtk_string("PREQUAL"), PREQUAL },
{wtk_string("INPUTXFORM"), INPUTXFORM },
{wtk_string(""), NULLSYM }
};

typedef struct
{
	wtk_str_hash_t *hash;
	wtk_strbuf_t *buf;
	wtk_fkind_t pkind;
	int nstate;
	Symbol sym;
	char macro_type;
	unsigned char bin:1;
}wtk_hmmtoken_t;

typedef enum
{
	WTK_SHARE_VECTOR,
	WTK_SHARE_MATRIX,
	WTK_SHARE_MIXPDF,
	WTK_SHARE_LIN_XFORM,
	WTK_SHARE_INPUT_XFORM,
}wtk_hmm_share_type;

typedef struct
{
	wtk_hmm_share_type type;
	union{
		wtk_svector_t *sv;
		wtk_mixpdf_t *pdf;
		wtk_smatrix_t *sm;
		wtk_linxform_t *lin_xform;
		wtk_inputxform_t *input_xform;
	}value;
}wtk_hmm_share_struct_t;

void wtk_hmmtoken_print(wtk_hmmtoken_t* t)
{
	wtk_string_t *s;
	int i;

	for(i=0;i<sizeof(symMap)/sizeof(wtk_symap_t);++i)
	{
		if(t->sym==symMap[i].sym)
		{
			s=&(symMap[i].name);
			printf("sym: %*.*s\n",s->len,s->len,s->data);
			break;
		}
	}
	if(t->sym==MACRO)
	{
		printf("type: \"%c\"\n",t->macro_type);
	}
}

int wtk_hmmtoken_get(wtk_hmmtoken_t *t,wtk_source_t *s)
{
#define MAX_SYM_LEN 40
	wtk_fkind_t pk;
	wtk_str_hash_t* hash=t->hash;
	char buf[MAX_SYM_LEN];
	int c,ret,sym;
	int i,imax;
	wtk_symap_t *sm;

	ret=-1;i=0;
	while(isspace(c=wtk_source_get(s)));
	//check is valid token or not.(is Macro or Symbol)
	if (c != '<' && c != ':' && c != '~'  && c != '.' && c != '#')
	{
		if (c == EOF){t->sym=EOFSYM;ret=0;}
		goto end;
	}
	if(c=='~')
	{
		//is macro
		c=wtk_source_get(s);
		if(c<'a')
		{
			c+=32;//'a'-'A';
			//c=tolower(c);
		}
		if (c!='s' && c!='m' && c!='u' && c!='x' && c!='d' && c!='c' &&
			c!='r' && c!='a' && c!='b' && c!='g' && c!='f' && c!='y' && c!='j' &&
			c!='v' && c!='i' && c!='t' && c!='w' && c!='h' && c!='o')
		{
			goto end;
		}
		t->macro_type=c;t->sym=MACRO;
		ret=0;goto end;
	}
	if(c=='#')
	{
		imax=MAX_SYM_LEN-1;i=0;
		while(((c=wtk_source_get(s))!='#') && i<imax)
		{
			buf[i++]=c;
		}
		if (strncmp(buf,"!MMF!",i) != 0)
		{
			goto end;
		}
		t->sym = MACRO;
		t->macro_type = 'h';
		ret=0;goto end;
	}
	if(c=='.')
	{
		//macro h(alias)
		while(isspace(c=wtk_source_get(s)));
		if(c==EOF)
		{
			t->sym=EOFSYM;
		}else
		{
			wtk_source_unget(s,c);
			t->sym=MACRO;
			t->macro_type='h';
		}
		ret=0;goto end;
	}
	if(c=='<')
	{
		imax=MAX_SYM_LEN-1;i=0;
		while(((c=wtk_source_get(s))!='>') && i<imax)
		{
			if(c>='a')
			{
				c-=32;
				//c=toupper(c);
			}
			buf[i++]=c;//islower(c)?toupper(c):c;
		}
		if(c!='>'){goto end;}
		sm=(wtk_symap_t*)wtk_str_hash_find(hash,buf,i);
		if(sm)
		{
			t->sym=sm->sym;
			ret=0;goto end;
		}
	}else
	{
		t->bin=1;
		sym=wtk_source_get(s);
		if(sym>=BEGINHMM && sym<PARMKIND)
		{
			t->sym=(Symbol)sym;
			ret=0;goto end;
		}else
		{
			ret=-1;goto end;
		}
	}
	ret=wtk_fkind_from_string(&pk,buf,i);
	if(ret==0)
	{
		//print_data(buf,i);
		t->pkind=pk;
		t->sym=PARMKIND;
	}
end:
	//wtk_hmmtoken_print(t);
	return ret;
}

int wtk_hmmset_load_option_value(wtk_hmmset_t *hl,wtk_source_t* s,wtk_hmmtoken_t *tok)
{
	short sw[MAX_STREAM_NUMBER],nst;
	int ret;

	switch(tok->sym)
	{
	case NUMSTATES:
		//read <NUMSTATES> value.
		ret=wtk_source_read_short(s,&nst,1,tok->bin);
		if(ret!=0){goto end;}
		tok->nstate=nst;
		break;
	case PARMKIND:
		//set hmm param kind.
		hl->pkind=tok->pkind;
		break;
	case VECSIZE:
		ret=wtk_source_read_short(s,&nst,1,tok->bin);
		if(ret!=0){goto end;}
		hl->vec_size=nst;
		break;
	case STREAMINFO:
		//read <STREAMINFO>,<STREAMINFO> numOfStream.
		ret=wtk_source_read_short(s,sw,1,tok->bin);
		if(ret!=0){goto end;}
		if(sw[0]<1 || sw[0]>MAX_STREAM_NUMBER){ret=-1;goto end;}
		ret=wtk_source_read_short(s,sw+1,sw[0],tok->bin);
		if(ret!=0){goto end;}
		memcpy(hl->stream_width,sw,(sw[0]+1)*sizeof(short));
		break;
	case NDUR:
	case PDUR:
	case GDUR:
	case RELDUR:
	case GENDUR:
		hl->dkind = (DurKind) (NULLD + (tok->sym-NDUR));
		break;
	case DIAGCOV:
		hl->ckind=DIAGC;
		break;
	case FULLCOV:
		hl->ckind=FULLC;
		break;
	case XFORMCOV:
		hl->ckind=XFORMC;
		break;
	case INVDIAGCOV:
		hl->ckind=INVDIAGC;
		break;
	case LLTCOV:
		hl->ckind=LLTC;
		break;
	default:
		ret=-1;goto end;
	}
	ret=wtk_hmmtoken_get(tok,s);
end:
	return ret;
}

int wtk_hmmset_freeze_options(wtk_hmmset_t* hl)
{
	int i,ret;

	if(hl->option_set){ret=0;goto end;}
	ret=-1;
	if(hl->vec_size==0)
	{
		if(hl->stream_width[0]>0 && hl->stream_width[1]>0)
		{
			for(i=1;i<=hl->stream_width[0];++i)
			{
				hl->vec_size+=hl->stream_width[i];
			}
		}else
		{
			goto end;
		}
	}
	if(hl->stream_width[0]==0)
	{
		hl->stream_width[0]=1;
		hl->stream_width[1]=hl->vec_size;
	}
	hl->option_set=1;
	ret=0;
end:
	return ret;
}

int wtk_hmmset_load_options(wtk_hmmset_t *hl,wtk_source_t *s,wtk_hmmtoken_t* tok)
{
	int ret;

	ret=wtk_hmmtoken_get(tok,s);
	if(ret!=0){goto end;}
	while (tok->sym == PARMKIND || tok->sym == INVDIAGCOV ||
		tok->sym == HMMSETID  || tok->sym == INPUTXFORM ||
		tok->sym == PARENTXFORM || tok->sym == PROJSIZE ||
		(tok->sym >= NUMSTATES && tok->sym <= XFORMCOV))
	{
		//if option name is valid,read option value.
		ret=wtk_hmmset_load_option_value(hl,s,tok);
		if(ret!=0){goto end;}
	}
	ret=wtk_hmmset_freeze_options(hl);
end:
	return ret;
}

//int wtk_hmmset_load_struct(wtk_hmmset_t *hl,wtk_source_t *s,wtk_hmmtoken_t* tok,void **v)
int wtk_hmmset_load_struct(wtk_hmmset_t *hl,wtk_source_t *s,wtk_hmmtoken_t* tok,wtk_hmm_share_struct_t *ss)
{
	void *d;
	int ret;

	ret=wtk_source_read_string(s,tok->buf);
	if(ret!=0){goto end;}
	d=wtk_hmmset_find_macro_hook(hl,tok->macro_type,tok->buf->data,tok->buf->pos);
	if(d)
	{
		switch(ss->type)
		{
		case WTK_SHARE_VECTOR:
			ss->value.sv=(wtk_svector_t*)d;
			break;
		case WTK_SHARE_MATRIX:
			ss->value.sm=(wtk_smatrix_t*)d;
			break;
		case WTK_SHARE_MIXPDF:
			ss->value.pdf=(wtk_mixpdf_t*)d;
			break;
		case WTK_SHARE_LIN_XFORM:
			ss->value.lin_xform=(wtk_linxform_t*)d;
			break;
		case WTK_SHARE_INPUT_XFORM:
			ss->value.input_xform=(wtk_inputxform_t*)d;
			break;
		}
		//*v=d;
		ret=0;
	}else
	{
		ret=-1;
	}
end:
	return ret;
}

int wtk_hmmset_load_variance(wtk_hmmset_t *hl,wtk_source_t *s,wtk_hmmtoken_t* tok,wtk_svector_t** data)
{
	wtk_hmm_share_struct_t ss;
	wtk_svector_t* sv;
	float *p,*e;
	short size;
	int ret;
	int n;
	//int cnt=0;

	switch(tok->sym)
	{
	case VARIANCE:
		ret=wtk_source_read_short(s,&size,1,tok->bin);
		if(ret!=0){goto end;}
		sv=wtk_svector_newh(hl->heap,size);
		ret=wtk_source_read_vector(s,sv,tok->bin);
		if(ret!=0){goto end;}
		n=wtk_vector_size(sv);
		p=sv+1;
		e=sv+n+1;
		while(p<e)
		{
			*(p)=1/(*p);
			++p;
			//++cnt;
		}
		//wtk_debug("cnt=%d/%d\n",cnt,n);
		//exit(0);
		break;
	case MACRO:
		if(tok->macro_type=='v')
		{
			ss.type=WTK_SHARE_VECTOR;
			ret=wtk_hmmset_load_struct(hl,s,tok,&ss);//(void**)&sv);
			if(ret!=0){goto end;}
			sv=ss.value.sv;
			wtk_inc_use((void**)sv);
			break;
		}
	default:
		ret=-1;goto end;
		break;
	}
	ret=wtk_hmmtoken_get(tok,s);
	if(ret==0)
	{
		*data=sv;
	}
end:
	return ret;
}

/**
 *	read mean vector
 *	mean =~u macro|<mean> short vector
 */
int wtk_hmmset_load_mean(wtk_hmmset_t *hl,wtk_source_t *s,wtk_hmmtoken_t* tok,wtk_svector_t **v)
{
	wtk_hmm_share_struct_t ss;
	wtk_svector_t* m=0;
	short size;
	int ret=-1;

	if(tok->sym==MEAN)
	{
		ret=wtk_source_read_short(s,&size,1,tok->bin);
		if(ret!=0){goto end;}
		m=wtk_svector_newh(hl->heap,size);
		ret=wtk_source_read_vector(s,m,tok->bin);
		if(ret!=0){goto end;}
	}else if(tok->sym==MACRO && tok->macro_type=='u')
	{
		ss.type=WTK_SHARE_VECTOR;
		ss.value.sv=0;
		ret=wtk_hmmset_load_struct(hl,s,tok,&ss);//(void**)&m);
		if(ret!=0){goto end;}
		m=ss.value.sv;
	}else
	{
		goto end;
	}
	ret=wtk_hmmtoken_get(tok,s);
end:
	if(ret==0)
	{
		*v=m;
	}
	return ret;
}

/* read  single Gaussian Mixture Component main information.
*	mixpdf= ~m macro						| mean cov [<GConst> float]
*						^read at this point.   or^read at this point.
*/
int wtk_hmmset_load_mixturepdf(wtk_hmmset_t *hl,wtk_source_t *s,wtk_hmmtoken_t* tok,wtk_mixpdf_t** data)
{
	wtk_hmm_share_struct_t ss;
	wtk_mixpdf_t *pdf;
	int ret;

	ret=-1;
	if(tok->sym==MACRO && tok->macro_type=='m')
	{
		ss.type=WTK_SHARE_MIXPDF;
		ret=wtk_hmmset_load_struct(hl,s,tok,&ss);//(void**)&pdf);
		if(ret!=0){goto end;}
		pdf=ss.value.pdf;
		++pdf->used;
		ret=wtk_hmmtoken_get(tok,s);
		if(ret!=0){goto end;}
	}else
	{
		//read mean vector
		pdf=wtk_hmmset_new_mixpdf(hl);
		ret=wtk_hmmset_load_mean(hl,s,tok,&(pdf->mean));
		if(ret!=0){goto end;}
		//read diagonal variance vector.
		if(tok->sym==VARIANCE || (tok->sym==MACRO && tok->macro_type=='v'))
		{
			ret=wtk_hmmset_load_variance(hl,s,tok,&(pdf->variance));
			if(ret!=0){goto end;}
		}
		//read const value.
		if(tok->sym==GCONST)
		{
			ret=wtk_source_read_float(s,&(pdf->fGconst),1,tok->bin);
			if(ret!=0){goto end;}
			ret=wtk_hmmtoken_get(tok,s);
			if(ret!=0){goto end;}
		}
		//pre * -0.5f to variance and fGconst
		wtk_mixpdf_post_process(hl,pdf);
	}
	*data=pdf;
end:
	return ret;
}

int wtk_hmmset_load_sweights(wtk_hmmset_t *hl,wtk_source_t *s,wtk_hmmtoken_t* tok,wtk_svector_t **v)
{
	wtk_hmm_share_struct_t ss;
	wtk_svector_t *sv;
	int ret;
	short size;

	if(tok->sym==SWEIGHTS)
	{
		ret=wtk_source_read_short(s,&size,1,tok->bin);
		if(ret!=0){goto end;}
		sv=wtk_svector_newh(hl->heap,size);
		ret=wtk_source_read_vector(s,sv,tok->bin);
		if(ret!=0){goto end;}
	}else if(tok->sym==MACRO && tok->macro_type=='w')
	{
		ss.type=WTK_SHARE_VECTOR;
		ret=wtk_hmmset_load_struct(hl,s,tok,&ss);//(void**)&sv);
		if(ret!=0){goto end;}
		sv=ss.value.sv;
		wtk_inc_use((void**)sv);
	}else
	{
		ret=-1;goto end;
	}
	*v=sv;
end:
	return ret;
}

/* read one component mixture.
*mixture=[<Mixture> short float] mixpdf
*								^read at this point.
*/
int wtk_hmmset_load_mixture(wtk_hmmset_t *hl,wtk_source_t *s,wtk_hmmtoken_t* tok,
		int M,wtk_mixture_t *mixture)
{
	float w=1.0;
	short m=1;
	int ret;

	if(tok->sym==MIXTURE)
	{
		ret=wtk_source_read_short(s,&m,1,tok->bin);
		if(ret!=0){goto end;}
		if(m<1||m>M){ret=-1;goto end;}
		ret=wtk_source_read_float(s,&w,1,tok->bin);
		if(ret!=0){goto end;}
		ret=wtk_hmmtoken_get(tok,s);
		if(ret!=0){goto end;}
	}
	//mixture->fWeight=w;
	mixture->fWeight=log(w);
	ret=wtk_hmmset_load_mixturepdf(hl,s,tok,&(mixture->pdf));
end:
	return ret;
}

/*
*read ~s segment or single state main info.
*stateInfo= ~s macro						 | [mixes] [weights] stream { stream }
*						^ read at this point			^ read at this point.
*/
int wtk_hmmset_load_stateinfo(wtk_hmmset_t *hl,wtk_source_t *s,wtk_hmmtoken_t* tok,wtk_state_t** data)
{
	int streams=hl->stream_width[0];
	short nMix[MAX_STREAM_NUMBER];
	int i;
	short sindex;
	short nmix;
	int j;
	wtk_state_t *st=0;
	int ret=-1;

	if(tok->sym==MACRO && tok->macro_type=='s')
	{
		ret=wtk_source_read_string(s,tok->buf);
		if(ret!=0){goto end;}
		st=(wtk_state_t*)wtk_hmmset_find_macro_hook(hl,tok->macro_type,tok->buf->data,tok->buf->pos);
		if(!st){goto end;}
		++st->used;
		ret=wtk_hmmtoken_get(tok,s);
	}else
	{
		st=wtk_hmmset_new_state(hl);
		if(tok->sym==NUMMIXES)
		{
			ret=wtk_source_read_short(s,nMix+1,streams,tok->bin);
			if(ret!=0){goto end;}
			ret=wtk_hmmtoken_get(tok,s);
			if(ret!=0){goto end;}
		}else
		{
			for(i=1;i<=streams;++i)
			{
				nMix[i]=1;
			}
		}
		//read stream weights if exist.
		if(tok->sym==SWEIGHTS || (tok->sym==MACRO && tok->macro_type=='w'))
		{
			ret=wtk_hmmset_load_sweights(hl,s,tok,&st->pfStreamWeight);
			if(ret!=0){goto end;}
			if(wtk_vector_size(st->pfStreamWeight) != streams){ret=-1;goto end;}
		}
		st->pStream=wtk_hmmset_new_streams(hl,streams);
		for(i=0;i<streams;++i)
		{
			sindex=i;
			if(tok->sym==STREAM)
			{
				ret=wtk_source_read_short(s,&sindex,1,tok->bin);
				if(ret!=0){goto end;}
				if(sindex<1 || sindex>streams){ret=-1;goto end;}
				sindex-=1;
				ret=wtk_hmmtoken_get(tok,s);
				if(ret!=0){goto end;}
			}
			nmix=nMix[i+1];
			if(tok->sym==NUMMIXES)
			{
				ret=wtk_source_read_short(s,&nmix,1,tok->bin);
				if(ret!=0){goto end;}
				ret=wtk_hmmtoken_get(tok,s);
				if(ret!=0){goto end;}
			}
			st->pStream[sindex].nMixture=nmix;
			st->pStream[sindex].pmixture=wtk_hmmset_new_mixtures(hl,nmix);
			for(j=0;j<nmix;++j)
			{
				//read <MIXTURE>
				ret=wtk_hmmset_load_mixture(hl,s,tok,nmix,&(st->pStream[sindex].pmixture[j]));
				if(ret!=0){goto end;}
			}
		}
		if(!st->pfStreamWeight)
		{
			st->pfStreamWeight=wtk_vector_new_h(hl->heap,streams);
			for(i=1;i<=streams;++i)
			{
				st->pfStreamWeight[i]=1.0;
			}
		}
	}
	*data=st;
end:
	//wtk_debug("ret=%d,data=%p\n",ret,st);
	return ret;
}

/* read trans matrix.
*	transp = ~t macro|<TransP> short matrix
*/
int wtk_hmmset_load_transmat(wtk_hmmset_t *hl,wtk_source_t *s,wtk_hmmtoken_t *tok,wtk_smatrix_t** sm)
{
	wtk_hmm_share_struct_t ss;
	wtk_smatrix_t *m;
	wtk_vector_t *v;
	short size;
	int ret,i,j;
	float rsum;

	if(tok->sym==TRANSP)
	{
		ret=wtk_source_read_short(s,&size,1,tok->bin);
		if(ret!=0){goto end;}
		if(size<1){ret=-1;goto end;}
		m=wtk_smatrix_newh(hl->heap,size,size);
		ret=wtk_source_read_matrix(s,m,tok->bin);
		if(ret!=0){goto end;}
		if(!hl->allow_tmods && m[1][size]>0.0)
		{
			 // kill teeModel
			rsum=0.0;v=m[1];v[size]=0.0;
			for(j=1;j<size;++j){rsum+=v[j];}
			for(j=1;j<size;++j){v[j]/=rsum;}
		}
		// convert to logs
		for(i=1;i<size;++i)
		{
			v=m[i];rsum=0.0;
			for(j=1;j<=size;++j)
			{
				rsum+=v[j];
				v[j] = (float)((v[j]<= MINLARG)?LZERO:log(v[j]));
			}
			if(rsum<0.99 || rsum>1.01){ret=-1;goto end;}
		}
		v=m[size];
		for(j=1;j<=size;++j)
		{
				v[j]=LZERO;
		}
		/*
		{
			static wtk_matrix_t *tmp=0;

			if(!tmp)
			{
				tmp=wtk_matrix_new(size,size);
			}
			wtk_matrix_cpy(m,tmp);
			wtk_matrix_transpose(m,tmp);
		}*/
	}else
	{
		ss.type=WTK_SHARE_MATRIX;
		ret=wtk_hmmset_load_struct(hl,s,tok,&ss);//(void**)&(m));
		if(ret!=0){goto end;}
		m=ss.value.sm;
		wtk_inc_use((void**)m);
	}
	ret=wtk_hmmtoken_get(tok,s);
	if(ret!=0){goto end;}
	*sm=m;
end:
	return ret;
}

int wtk_hmmset_load_transfrom(wtk_hmmset_t *hl,wtk_source_t *s,wtk_hmmtoken_t *tok,wtk_smatrix_t** sm)
{
	wtk_hmm_share_struct_t ss;
	wtk_smatrix_t *m = NULL;
	short rows,cols;
	int ret;

	if(tok->sym==XFORM)
	{
		ret=wtk_source_read_short(s,&rows,1,tok->bin);
		if(ret!=0){goto end;}
		ret=wtk_source_read_short(s,&cols,1,tok->bin);
		if(ret!=0){goto end;}
		m=wtk_smatrix_newh(hl->heap,rows,cols);
		ret=wtk_source_read_matrix(s,m,tok->bin);
	}else if(tok->sym==MACRO && tok->macro_type=='x')
	{
		ss.type=WTK_SHARE_MATRIX;
		ret=wtk_hmmset_load_struct(hl,s,tok,&ss);//(void**)&(m));
		if(ret!=0){goto end;}
		m=ss.value.sm;
		wtk_inc_use((void**)m);
	}else
	{
		ret=-1;
	}
	if(ret==0)
	{
		*sm=m;
	}
end:
	return ret;
}

int wtk_hmmset_load_linxform(wtk_hmmset_t *hl,wtk_source_t *s,wtk_hmmtoken_t *tok,wtk_inputxform_t* ixf)
{
	wtk_hmm_share_struct_t ss;
	wtk_heap_t *heap=hl->heap;
	wtk_linxform_t *xf = NULL;
	int ret,num_block,i,b;

	if(tok->sym==VECSIZE)
	{
		xf=(wtk_linxform_t *)wtk_heap_malloc(heap,sizeof(*xf));
		ret=wtk_source_read_int(s,&(xf->vec_size),1,tok->bin);
		if(ret!=0){goto end;}
		ret=wtk_hmmtoken_get(tok,s);
		if(ret!=0){goto end;}
		if(tok->sym==OFFSET)
		{
			ret=wtk_hmmtoken_get(tok,s);
			wtk_debug("dummy read bias\n");
		}else
		{
			xf->bias=0;
		}
		if(tok->sym==LOGDET)
		{
			ret=wtk_source_read_float(s,&(xf->det),1,tok->bin);
			if(ret!=0){goto end;}
			ret=wtk_hmmtoken_get(tok,s);
			if(ret!=0){goto end;}
		}else
		{
			xf->det=0;
		}
		if(tok->sym!=BLOCKINFO){ret=-1;goto end;}
		ret=wtk_source_read_int(s,&(num_block),1,tok->bin);
		if(ret!=0){goto end;}
		xf->block_size=wtk_int_vector_new_h(heap,num_block);
		ret=wtk_source_read_int(s,xf->block_size+1,num_block,tok->bin);
		if(ret!=0){goto end;}
		xf->xform=(wtk_smatrix_t**)wtk_heap_malloc(heap,(num_block+1)*sizeof(wtk_smatrix_t*));
		ret=wtk_hmmtoken_get(tok,s);
		if(ret!=0){goto end;}
		for(i=1;i<=num_block;++i)
		{
			if(tok->sym!=BLOCK){ret=-1;goto end;}
			ret=wtk_source_read_int(s,&b,1,tok->bin);
			if(b!=i){ret=-1;goto end;}
			ret=wtk_hmmtoken_get(tok,s);
			if(ret!=0){goto end;}
			ret=wtk_hmmset_load_transfrom(hl,s,tok,&(xf->xform[i]));
			//wtk_debug("%d,%p\n",ret,xf->xform[i]);
			if(ret!=0){goto end;}
		}
		if(tok->sym==VARIANCE)
		{
			ret=wtk_hmmset_load_variance(hl,s,tok,&(xf->vFloor));
		}else
		{
			xf->vFloor=0;
		}
		xf->nUse=0;
	}else if(tok->sym==MACRO && tok->macro_type=='f')
	{
		ss.type=WTK_SHARE_LIN_XFORM;
		ret=wtk_hmmset_load_struct(hl,s,tok,&ss);//(void**)&(xf));
		if(ret!=0){goto end;}
		xf=ss.value.lin_xform;
		++xf->nUse;
		ret=wtk_hmmtoken_get(tok,s);
	}else
	{
		ret=-1;
	}
	if(ret==0){ixf->xform=xf;}
end:
	return ret;
}

int wtk_hmmset_load_inputxform(wtk_hmmset_t *hl, wtk_source_t *s,
		wtk_hmmtoken_t* tok, void** data)
{
	wtk_hmm_share_struct_t ss;
	wtk_inputxform_t* xf = NULL; 
	wtk_heap_t *heap = hl->heap;
	int ret;

	ret = -1;
	//wtk_hmmtoken_print(tok);
	if (tok->sym == MMFIDMASK)
	{
		xf = (wtk_inputxform_t*) wtk_heap_zalloc(heap, sizeof(*xf));
		ret = wtk_source_read_string(s, tok->buf);
		if (ret != 0)
		{
			goto end;
		}
		xf->mmfIdMask
				= wtk_heap_dup_string(heap, tok->buf->data, tok->buf->pos);
		//print_data(tok->buf->data,tok->buf->pos);
		//read param kind.
		ret = wtk_hmmtoken_get(tok, s);
		if (ret != 0)
		{
			goto end;
		}
		if (tok->sym != PARMKIND)
		{
			ret = -1;
			goto end;
		}
		xf->pkind = tok->pkind;
		ret = wtk_hmmtoken_get(tok, s);
		if (ret != 0)
		{
			goto end;
		}
		if (tok->sym == PREQUAL)
		{
			xf->preQual = 1;
			ret = wtk_hmmtoken_get(tok, s);
		}
		else
		{
			xf->preQual = 0;
			if (tok->sym != LINXFORM)
			{
				ret = -1;
				goto end;
			}
			ret = wtk_hmmtoken_get(tok, s);
			if (ret != 0)
			{
				goto end;
			}
			ret = wtk_hmmset_load_linxform(hl, s, tok, xf);
			if (ret != 0)
			{
				goto end;
			}
			xf->nUse = 0;
		}
	}
	else if (tok->sym == MACRO && tok->macro_type == 'j')
	{
		ss.type=WTK_SHARE_INPUT_XFORM;
		ret = wtk_hmmset_load_struct(hl, s, tok, &ss);//(void**) &xf);
		if (ret != 0)
		{
			goto end;
		}
		xf=ss.value.input_xform;
		++xf->nUse;
		ret = wtk_hmmtoken_get(tok, s);
	}
	else
	{
		ret = -1;
	}
	if (ret == 0)
	{
		hl->xform=xf;
		*data = xf;
	}
end:
	return ret;
}

/* read share structure. ~m,~v,~u,~s,~t
*1)if type is not j(linear transform),and option is not set,return false.
*2)read share structure,and put it in AIHmm macro Hash table.
*/
int wtk_hmmset_load_share_data(wtk_hmmset_t *hl,wtk_source_t *s,wtk_hmmtoken_t* tok)
{
	wtk_string_t*name;
	int ret;
	char type;
	void *hook;

	ret=wtk_source_read_string(s,tok->buf);
	if(ret!=0){goto end;}
	name=wtk_hmmset_find_name(hl,tok->buf->data,tok->buf->pos);
	type=tok->macro_type;
	if(type!='j' && !hl->option_set){goto end;}
	//wtk_hmmtoken_print(tok);
	ret=wtk_hmmtoken_get(tok,s);
	if(ret!=0){goto end;}
	//wtk_hmmtoken_print(tok);
	switch(type)
	{
	case 'v':
		//read share diagonal variance vector.
		ret=wtk_hmmset_load_variance(hl,s,tok,(wtk_svector_t**)&hook);
		if(ret!=0){goto end;}
		break;
	case 'm':
		ret=wtk_hmmset_load_mixturepdf(hl,s,tok,(wtk_mixpdf_t**)&hook);
		if(ret!=0){goto end;}
		break;
	case 's':
		//read share state distribution.
		ret=wtk_hmmset_load_stateinfo(hl,s,tok,(wtk_state_t**)&hook);
		if(ret!=0){goto end;}
		//wtk_debug("[%.*s]=%p\n",name->len,name->data,*((wtk_state_t**)hook));
		if(hook)
		{
			(((wtk_state_t*)hook))->name=name;
		}
		break;
	case 'u':
		ret=wtk_hmmset_load_mean(hl,s,tok,(wtk_svector_t**)&hook);
		if(ret!=0){goto end;}
		break;
	case 't':
		ret=wtk_hmmset_load_transmat(hl,s,tok,(wtk_smatrix_t**)&hook);
		if(ret!=0){goto end;}
		break;
	case 'j':
		ret=wtk_hmmset_load_inputxform(hl,s,tok,&hook);
		if(ret!=0){goto end;}
		break;
	default:
		goto end;
	}
	wtk_hmmset_add_macro(hl,type,name->data,name->len,hook);
	ret=0;
end:
	return ret;
}

int wtk_hmmset_load_hmmdef(wtk_hmmset_t *hl,wtk_source_t *s,wtk_hmmtoken_t *tok,wtk_hmm_t *hmm)
{
	wtk_state_t **pState;
	short state;
	int nstate=0;
	int ret=-1;

	if(tok->sym!=BEGINHMM)
	{
		wtk_debug("begin hmm failed\n");
		goto end;
	}
	ret=wtk_hmmtoken_get(tok,s);
	if(ret!=0){goto end;}
	while(tok->sym!=STATE)
	{
		tok->nstate=0;
		ret=wtk_hmmset_load_option_value(hl,s,tok);
		if(ret!=0)
		{
			wtk_debug("begin hmm failed\n");
			goto end;
		}
		if(tok->nstate>nstate){nstate=tok->nstate;}
	}
	ret=wtk_hmmset_freeze_options(hl);
	if(ret!=0){goto end;}
	if(nstate==0||nstate<3)
	{
		wtk_debug("begin hmm failed\n");
		ret=-1;goto end;
	}
	hmm->num_state=nstate;
	if(hmm->num_state>hl->max_hmm_state)
	{
		hl->max_hmm_state=hmm->num_state;
	}
	pState=(wtk_state_t**)wtk_heap_zalloc(hl->heap,(nstate-2)*sizeof(wtk_state_t*));
	hmm->pState=pState-2;
	while(tok->sym==STATE)
	{
		ret=wtk_source_read_short(s,&state,1,tok->bin);
		if(ret!=0)
		{
			wtk_debug("begin hmm failed\n");
			goto end;
		}
		if(state<2||state>=nstate)
		{
			wtk_debug("begin hmm failed\n");
			goto end;
		}
		ret=wtk_hmmtoken_get(tok,s);
		if(ret!=0)
		{
			wtk_debug("begin hmm failed\n");
			goto end;
		}
		ret=wtk_hmmset_load_stateinfo(hl,s,tok,&(hmm->pState[state]));
		if(ret!=0)
		{
			wtk_debug("begin hmm failed\n");
			goto end;
		}
	}
	//read transp
	if(tok->sym==TRANSP || (tok->sym==MACRO && tok->macro_type=='t'))
	{
		ret=wtk_hmmset_load_transmat(hl,s,tok,&(hmm->transP));
		if(ret!=0)
		{
			wtk_debug("begin hmm failed\n");
			goto end;
		}
		if(wtk_matrix_rows(hmm->transP) != nstate || wtk_matrix_cols(hmm->transP)!=nstate)
		{
			wtk_debug("trans failed\n");
			ret=-1;goto end;
		}
	}else
	{
		wtk_debug("fojnd bug %d/%d\n",tok->sym,TRANSP);
		ret=-1;goto end;
	}
	ret=wtk_hmmtoken_get(tok,s);
end:
	//wtk_debug("ret=%d\n",ret);
	return ret;
}

void wtk_hmmset_transpose_trans_matrix(wtk_hmmset_t *hl)
{
	wtk_hmmset_transpose_trans_matrix2(hl,1.0);
}

void wtk_hmmset_transpose_trans_matrix2(wtk_hmmset_t *hl,float trans_scale)
{
	wtk_larray_t *array;
	wtk_hmm_t **ph;
	wtk_matrix_t *trP;
	wtk_matrix_t *tmp=0;
	wtk_hmm_t *hmm;
	void *p;
	int i;

	array=hl->hmm_array;
	ph=(wtk_hmm_t**)array->slot;
	for(i=0;i<array->nslot;++i)
	{
		hmm=ph[i];
		trP=hmm->transP;
		if(!trP)
		{
			continue;
		}
		if(!tmp||(wtk_matrix_rows(tmp)!=wtk_matrix_rows(trP)))
		{
			tmp=wtk_matrix_new(wtk_matrix_rows(trP),wtk_matrix_cols(trP));
		}
		//wtk_debug("[%.*s]=%p\n",hmm->name->len,hmm->name->data,hmm->transP);
		p=wtk_get_hook((void**)trP);
		if(p==NULL)
		{
			//wtk_matrix_print(trP);
			wtk_matrix_cpy(trP,tmp);
			wtk_matrix_transpose(trP,tmp);
			if(trans_scale!=1.0)
			{
				wtk_matrix_scale(trP,trans_scale);
			}
			p=(void*)1;
			wtk_set_hook((void**)trP,p);
		}
	}
	if(tmp)
	{
		wtk_matrix_delete(tmp);
	}
}

int wtk_hmmset_load_hmm(wtk_hmmset_t *hl,wtk_source_t *s,wtk_hmmtoken_t *tok)
{
	wtk_hmm_t tmm;
	wtk_hmm_t *hmm;
	int ret;

	ret=wtk_source_read_string(s,tok->buf);
	if(ret!=0)
	{
		wtk_debug("read hmm name failed.\n");
		goto end;
	}
	//print_data(tok->buf->data,tok->buf->pos);
	if(!hl->load_hmm_from_hmmlist)
	{
		wtk_hmmset_add_hmm(hl,tok->buf->data,tok->buf->pos,tok->buf->data,tok->buf->pos);
	}
	//hmm=(wtk_hmm_t*)wtk_hmmset_find_macro_hook(hl,'l',tok->buf->data,tok->buf->pos);
	hmm=wtk_hmmset_find_hmm(hl,tok->buf->data,tok->buf->pos);
	if(hmm)
	{
		//wtk_debug("found hmm=%p s=%p [%.*s]\n",hmm,hmm->pState,tok->buf->pos,tok->buf->data);
		//exit(0);
		if(hmm->pState)
		{
			wtk_hmmtoken_print(tok);
			wtk_debug("logic err[%.*s/%d].\n",tok->buf->pos,tok->buf->data,hl->load_hmm_from_hmmlist);
			wtk_debug("%.*s\n",hmm->name->len,hmm->name->data);
			ret=-1;goto end;
		}
	}else
	{
		hmm=&tmm;
	}
	ret=wtk_hmmtoken_get(tok,s);
	if(ret!=0)
	{
		wtk_debug("read tok failed.\n");
		goto end;
	}
	ret=wtk_hmmset_load_hmmdef(hl,s,tok,hmm);
	if(ret!=0)
	{
		wtk_debug("read hmmdef failed.\n");
		goto end;
	}
end:
	return ret;
}

int wtk_hmmset_load_xform(wtk_hmmset_t *hl,wtk_source_t *s)
{
	wtk_str_hash_t* sym_hash;
	wtk_symap_t *sym;
	wtk_hmmtoken_t tok;
	wtk_hmmtoken_t *t=&tok;
	int i,symc,ret;
	char type;

	symc=sizeof(symMap)/sizeof(wtk_symap_t);
	sym_hash=wtk_str_hash_new(symc);
	for(i=0;i<symc;++i)
	{
		sym=&(symMap[i]);
		wtk_str_hash_add(sym_hash,sym->name.data,sym->name.len,sym);
	}
	tok.bin=0;tok.hash=sym_hash;tok.nstate=0;
	tok.buf=wtk_strbuf_new(64,1);
	ret=wtk_hmmtoken_get(t,s);
	if(ret!=0){goto end;}
	//wtk_hmmtoken_print(t);
	while(tok.sym!=EOFSYM)
	{
		if(tok.sym!=MACRO){goto end;}
		type=tok.macro_type;
		//wtk_hmmtoken_print(t);
		switch(type)
		{
		case 'o':
			//if ~o, read options.
			ret=wtk_hmmset_load_options(hl,s,t);
			if(ret!=0){goto end;}
			break;
		case 'h':
			ret=wtk_hmmset_load_hmm(hl,s,t);
			if(ret!=0){goto end;}
			break;
		//case 'v':
		default:
			//load share Structure like ~v etc.
			ret=wtk_hmmset_load_share_data(hl,s,t);
			if(ret!=0){goto end;}
			break;
		}
	}
	ret=0;
end:
	wtk_str_hash_delete(sym_hash);
	wtk_strbuf_delete(tok.buf);
	return ret;
}

void wtk_hmmset_set_state_index(wtk_hmmset_t *hl)
{
	wtk_hmm_t *hmm;
	wtk_hmm_t **ph;
	wtk_larray_t *array;
	int i,j,N;
	int state_index;

	array=hl->hmm_array;
	ph=(wtk_hmm_t**)array->slot;
	state_index=0;
	for(i=0;i<array->nslot;++i)
	{
		hmm=ph[i];
		N=hmm->num_state;
		for(j=2;j<N;++j)
		{
			if(hmm->pState[j]->index<0)
			{
				hmm->pState[j]->index=++state_index;
			}
		}
	}
	hl->num_states=state_index;
}

void wtk_hmmset_set_seindex(wtk_hmmset_t *hl)
{
	wtk_larray_t *array;
	wtk_hmm_t **ph;
	wtk_hmm_t *hmm;
	wtk_heap_t *heap=hl->heap;
	wtk_matrix_t *trP;
	short **se;
	int i,n,j,N,min,max;
	int max_state;

	array=hl->hmm_array;
	n=hl->num_phy_hmm;
	hl->seIndexes=(short***)wtk_heap_malloc(heap,sizeof(short**)*n);
	ph=(wtk_hmm_t**)array->slot;
	max_state=0;
	for(i=0;i<array->nslot;++i)
	{
		hmm=ph[i];
		trP=hmm->transP;
		if(!trP){continue;}
		N=hmm->num_state;
		if(N>max_state){max_state=N;}
		se=(short**)wtk_heap_malloc(heap,sizeof(short*)*(N-1));
		se-=2;hl->seIndexes[i]=se;
		//hmm->seIndex=se;
		//hmm->max_trans=wtk_matrix_max(trP);
		for(j=2;j<=N;++j)
		{
			se[j]=(short*)wtk_heap_malloc(heap,2*sizeof(short));
			for(min=(j==N)?2:1;min<N;++min)
			{
				if(trP[min][j]>LSMALL){break;}
			}
			for(max=N-1;max>1;--max)
			{
				if(trP[max][j]>LSMALL){break;}
			}
			se[j][0]=min;
			se[j][1]=max;
		}
	}
}

void wtk_hmmset_update(wtk_hmmset_t *hl)
{
	wtk_hmmset_set_state_index(hl);
	wtk_hmmset_set_seindex(hl);
}


int wtk_hmmset_load_model(wtk_hmmset_t *hl,wtk_source_t *s)
{
	wtk_str_hash_t* sym_hash;
	wtk_symap_t *sym;
	wtk_hmmtoken_t tok;
	wtk_hmmtoken_t *t=&tok;
	int i,symc,ret;
	char type;

	s->swap=hl->use_le?0:1;
	symc=sizeof(symMap)/sizeof(wtk_symap_t);
	sym_hash=wtk_str_hash_new(symc*2+1);
	for(i=0;i<symc;++i)
	{
		sym=&(symMap[i]);
		wtk_str_hash_add(sym_hash,sym->name.data,sym->name.len,sym);
	}
	tok.bin=0;tok.hash=sym_hash;tok.nstate=0;
	tok.buf=wtk_strbuf_new(64,1);
	ret=wtk_hmmtoken_get(t,s);
	if(ret!=0){goto end;}
	//wtk_hmmtoken_print(t);
	while(tok.sym!=EOFSYM)
	{
		if(tok.sym!=MACRO){goto end;}
		type=tok.macro_type;
		//wtk_hmmtoken_print(t);
		switch(type)
		{
		case 'o':
			//if ~o, read options.
			ret=wtk_hmmset_load_options(hl,s,t);
			if(ret!=0)
			{
				wtk_debug("read option failed.\n");
				goto end;
			}
			break;
		case 'h':
			ret=wtk_hmmset_load_hmm(hl,s,t);
			if(ret!=0)
			{
				//wtk_hmmtoken_print(t);
				wtk_debug("read hmm failed.\n");
				goto end;
			}
			break;
		//case 'v':
		default:
			//load share Structure like ~v etc.
			ret=wtk_hmmset_load_share_data(hl,s,t);
			if(ret!=0)
			{
				wtk_debug("read share data failed.\n");
				goto end;
			}
			break;
		}
	}
	wtk_hmmset_update(hl);
	ret=0;
end:
	//wtk_debug("ret=%d\n",ret);
	wtk_str_hash_delete(sym_hash);
	wtk_strbuf_delete(tok.buf);
	return ret;
}

int wtk_hmmset_load_list(wtk_hmmset_t *hl,wtk_source_t *s)
{
	int ret,nl;
	wtk_strbuf_t *lb,*pb;

	lb=wtk_strbuf_new(32,1);
	pb=wtk_strbuf_new(32,1);
	while(1)
	{
		ret=wtk_source_skip_sp(s,&nl);
		if(ret!=0){goto end;}
		ret=wtk_source_read_string(s,lb);
		if(ret!=0)
		{
			ret=0;//FOR EOF
			goto end;
		}
		ret=wtk_source_skip_sp(s,&nl);
		if(ret!=0){goto end;}
		if(!nl)
		{
			ret=wtk_source_read_string(s,pb);
			if(ret!=0){goto end;}
			ret=wtk_hmmset_add_hmm(hl,lb->data,lb->pos,pb->data,pb->pos);
		}else
		{
			ret=wtk_hmmset_add_hmm(hl,lb->data,lb->pos,lb->data,lb->pos);
		}
		if(ret!=0){goto end;}
	}
	ret=0;
end:
	wtk_strbuf_delete(lb);
	wtk_strbuf_delete(pb);
	return ret;
}

int wtk_hmmset_load_list3(wtk_hmmset_t *hl,wtk_source_t *src)
{
	int vi;
	char bi;
	int ret;
	int i,n;
	wtk_strbuf_t *buf;

	buf=wtk_strbuf_new(256,1);
	ret=wtk_source_fill(src,(char*)&vi,4);
	if(ret!=0){goto end;}
	//print_hex((char*)&(vi),4);
	n=vi;
	for(i=0;i<n;++i)
	{
		ret=wtk_source_fill(src,(char*)&bi,1);
		//print_data(&(bi),1);
		if(ret!=0)
		{
			wtk_debug("read k len failed\n");
			goto end;
		}
		ret=wtk_source_fill(src,buf->data,(int)bi);
		if(ret!=0)
		{
			wtk_debug("read k v failed(%d)\n",bi);
			goto end;
		}
		//wtk_debug("v[%d/%d]=[%.*s]\n",i,n,bi,buf->data);
		//exit(0);
		ret=wtk_hmmset_add_hmm2(hl,buf->data,bi);
		if(ret!=0){goto end;}
	}
	ret=0;
end:
	wtk_strbuf_delete(buf);
	return ret;
}

int wtk_hmmset_load_list4(wtk_hmmset_t *hl,wtk_source_t *src)
{
	wtk_string_t *data;
	char *s;
	char bi;
	int i,n;
	wtk_hmm_t *hmm,*h;
	wtk_hmm_t **phmm;

	//wtk_debug("nslot=%d\n",hl->hmm_hash->nslot);
	data=src->get_file(src->data);
	s=data->data;
	n=*((int*)s);
	s+=4;
	hmm=(wtk_hmm_t*)wtk_calloc(n,sizeof(wtk_hmm_t));
	wtk_heap_add_large(hl->heap,(char*)hmm,sizeof(wtk_hmm_t)*n);
	//hl->num_phy_hmm=n;
	phmm=(wtk_hmm_t**)wtk_larray_push_n(hl->hmm_array,n);
	for(i=0;i<n;++i)
	{
		bi=*s;
		++s;
		h=hmm+i;
		h->tIdx=hl->num_phy_hmm++;
		//wtk_debug("v[%d/%d]=[%.*s]\n",i,n,bi,s);
		//exit(0);
		h->name=wtk_heap_dup_string(hl->heap,s,bi);
		//h->sil=wtk_string_cmp_s(hmm->name,"sil")==0?1:0;
		phmm[i]=h;
		wtk_str_hash_add(hl->hmm_hash,h->name->data,h->name->len,h);
		s+=bi;
		//wtk_debug("v[%d/%d]=[%.*s]\n",i,n,h->name->len,h->name->data);
	}
	//wtk_debug("hl->n=%d hmm=%p a=%p/%p\n",hl->hmm_array->nslot,hmm,phmm,hl->hmm_array->slot);
	return 0;
}


int wtk_hmmset_load_list5(wtk_hmmset_t *hl,wtk_source_t *src)
{
	wtk_string_t *data;
	char *s;
	char bi;
	int i,n;

	//wtk_debug("nslot=%d\n",hl->hmm_hash->nslot);
	data=src->get_file(src->data);
	s=data->data;
	n=*((int*)s);
	s+=4;
	for(i=0;i<n;++i)
	{
		bi=*s;
		++s;
		wtk_hmmset_add_hmm2(hl,s,bi);
		s+=bi;
	}
	//wtk_debug("hl->n=%d\n",hl->hmm_array->nslot);
	return 0;
}

int wtk_hmmset_load_list2(wtk_hmmset_t *hl,wtk_source_t *src)
{
	if(src->get_file)
	{
		return wtk_hmmset_load_list4(hl,src);
	}else
	{
		return wtk_hmmset_load_list3(hl,src);
	}
}
