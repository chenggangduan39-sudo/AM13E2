#include "wtk_mer_source_file.h"
#include "wtk_file.h"
#include "wtk_type.h"
#include "wtk_fix.h"
#include "wtk_blas.h"
#define SING_QUOTE '\''
#define DBL_QUOTE '"'
#define ESCAPE_CHAR '\\'

/* source read */
int wtk_mer_source_read_line(wtk_source_t *s,wtk_strbuf_t *b)
{
	int ret;
	int c;

	wtk_strbuf_reset(b);
	while(1)
	{
		c=wtk_source_get(s);
		if(c=='\n'||c==EOF){goto end;}
		wtk_strbuf_push_c(b,c);
	}
end:
	//ret=b->pos<=0?-1:0;
    ret= c==EOF ? -1: 0;
	return ret;
}
int wtk_mer_source_read_string(wtk_source_t *s,wtk_strbuf_t *b)
{/* not process escape char
不处理转义字符
 */
	int isq,q=0,c,ret;
	//char t;

	wtk_strbuf_reset(b);
	if(s->read_str)
	{
		return s->read_str(s->data,b);
	}
	ret=-1;
	while(isspace(c=wtk_source_get(s)));
	if(c==EOF){goto end;}
	if(c==DBL_QUOTE||c==SING_QUOTE)
	{
		isq=1;q=c;
		c=wtk_source_get(s);
	}else
	{
		isq=0;
	}
	while(1)
	{
		//wtk_debug("%d:%c\n",c,c);
		// if(c==92) {printf("发现单斜杠==%c==\n",c);}
		if(c==EOF){ret=0;goto end;}
		if(isq)
		{
			if(c==q){break;}
		}else
		{
			if(c==EOF||isspace(c)||c=='\"' ||c=='\'')
			{
				wtk_source_unget(s,c);
				break;
			}
		}
		wtk_strbuf_push_c(b,c);
		c=wtk_source_get(s);
	}
	ret=0;
end:
	return ret;
}
int wtk_mer_source_read_float(wtk_source_t *s,float *f,int n,int bin)
{
	int ret=0;
	float *p,*e;
	double d;

	e=f+n;
	if(bin)
	{
		ret=wtk_source_fill(s,(char*)f,n*sizeof(float));
		if(ret!=0 || !s->swap){goto end;}
	}else
	{
		for(p=f;p<e;++p)
		{
			ret=wtk_source_atof(s,&d);
			if(ret!=0)
			{
				goto end;
			}
			*p=d;
		}
	}
end:
	return ret;
}

int wtk_mer_source_read_double(wtk_source_t *s,double *f,int n,int bin)
{
	int ret=0;
	double *p,*e;
	double d;

	e=f+n;
	if(bin)
	{
		ret=wtk_source_fill(s,(char*)f,n*sizeof(double));
		if(ret!=0 || !s->swap){goto end;}
	}else
	{
		for(p=f;p<e;++p)
		{
			ret=wtk_source_atof(s,&d);
			if(ret!=0)
			{
				goto end;
			}
			*p=d;
		}
	}
end:
	return ret;
}


/* source loader */
/* wtk_rbin2_get2 会先将item加载到buf, 如果再copy到矩阵, 会产生双份内存 */
void wtk_mer_source_loader_init(wtk_source_loader_t *sl, wtk_source_t *src, char *fn)
{
    int use_rbin = !(sl->hook==NULL);
    int ret=-1;
    if (use_rbin)
    {
        wtk_rbin2_item_t *item;
        wtk_rbin2_t *rb = sl->hook;
        item=wtk_rbin2_get2(rb, fn, strlen(fn));
        // item=wtk_rbin2_get(rb, fn, strlen(fn));
        if (!item)
        {goto end;}
        wtk_source_init_rbin2(src, item);
        ret=0;
    } else {
        ret=wtk_source_init_file(src, fn);
    }
end:
    if (ret!=0)
    { wtk_exit_debug("[%s] load failed \n", fn);}
    wtk_debug("%s load file ---> %s \n", use_rbin?"rbin":"", fn);
}
void wtk_mer_source_loader_load_vecf(wtk_source_loader_t *sl, wtk_source_t *src, char *fn, wtk_vecf_t *vf)
{
    wtk_mer_source_loader_load_float(sl, src, fn, vf->p, vf->len);
};

void wtk_mer_source_loader_load_vecdf(wtk_source_loader_t *sl, wtk_source_t *src, char *fn, wtk_vecdf_t *vf)
{
    wtk_mer_source_loader_load_double(sl, src, fn, vf->p, vf->len);
};

// #pragma message( "Need to do 3D collision testing \n")

void wtk_mer_source_loader_load_matf(wtk_source_loader_t *sl, wtk_source_t *src, char *fn, wtk_matf_t *mf)
{
    int len = mf->row*mf->col;
    wtk_mer_source_loader_load_float(sl, src, fn, mf->p, len);
	// wtk_exit(1);
};
void wtk_mer_source_loader_load_matf2( wtk_heap_t *heap, wtk_source_loader_t *sl, wtk_source_t *src, char *fn, wtk_matf_t *mf)
{/* 允许
	矩阵定点
	MKL SGEMM PACK
	等额外操作
*/
	wtk_mer_source_loader_load_matf(sl, src, fn, mf);
	#if defined(USE_MAT_FIX) || defined(USE_NEON)
    FIX_TYPE *pi = (FIX_TYPE*)mf->p;
    int j, len=mf->row*mf->col;
    float *p = mf->p;
    for (j=0; j<len; ++j)
    {
        pi[j] = (FIX_TYPE)(p[j]*FIX);
    }
	#elif defined(USE_MKL) && defined(USE_MKL_PACK)
	int pack_size
	  , m=0
	  , n=mf->row
	  , k=mf->col;
	float *packb;
	n=mf->row;
	k=mf->col;
	pack_size=cblas_sgemm_pack_get_size(CblasBMatrix, m, n, k);
	packb=wtk_heap_malloc( heap, pack_size);
	cblas_sgemm_pack( CblasRowMajor, CblasBMatrix, CblasTrans, m, n, k, 1, mf->p, k, packb);
	mf->p=packb;
    #endif
}
void wtk_mer_source_loader_load_float(wtk_source_loader_t *sl, wtk_source_t *src, char *fn, float *f, int len)
{
    wtk_mer_source_loader_with_as(sl, src, fn, 
    {
        wtk_mer_source_read_float(src, f, len, 1);
    });
	// ssy {wtk_mer_source_loader_init(sl,src,fn);{ wtk_mer_source_read_float(src, f, len, 1); };if(sl->hook==NULL){wtk_source_clean_file(src);}};
};
void wtk_mer_source_loader_load_double(wtk_source_loader_t *sl, wtk_source_t *src, char *fn, double *f, int len)
{
    wtk_mer_source_loader_with_as(sl, src, fn, 
    {
        wtk_mer_source_read_double(src, f, len, 1);
    });
};