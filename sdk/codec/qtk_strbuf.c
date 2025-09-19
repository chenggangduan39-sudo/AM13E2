#include "qtk_strbuf.h"


#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif
#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

qtk_strbuf_t* qtk_strbuf_new(int init_len,float rate)
{
    qtk_strbuf_t *b;
    char *data;

    data=(char*)malloc(init_len);
    if(!data){b=0;goto end;}
    b=(qtk_strbuf_t*)malloc(sizeof(*b));
    b->data=data;
    b->length=init_len;
    b->pos=0;
    b->rate=1.0+rate;
	
end:
    return b;
}

void qtk_strbuf_resize(qtk_strbuf_t *buf,int size)
{
	free(buf->data);
	buf->data=(char*)malloc(size);
	buf->length=size;
	buf->pos=0;
}

int qtk_strbuf_delete(qtk_strbuf_t* b)
{
    free(b->data);
    free(b);
    return 0;
}

void qtk_strbuf_expand(qtk_strbuf_t *s,int bytes)
{
    int left,alloc;
    char *p;
    int t1,t2;

    left=s->length-s->pos;
    if(bytes>left)
    {
    	t1=s->length*s->rate;
    	t2=s->pos+bytes;
        alloc=max(t1,t2);//s->length*s->rate,s->pos+bytes);
        p=s->data;
        s->data=(char*)malloc(alloc);
        s->length=alloc;
        memcpy(s->data,p,s->pos);
        free(p);
    }
    return;
}

void qtk_strbuf_push(qtk_strbuf_t *s,const char *buf,int bytes)
{
    if(bytes<=0){return;}
    if(bytes>s->length-s->pos)
    {
        qtk_strbuf_expand(s,bytes);
    }
    if(buf)
    {
    	memcpy(s->data+s->pos,buf,bytes);
    }else
    {
    	memset(s->data+s->pos,0,bytes);
    }
    s->pos+=bytes;
    return;
}

int qtk_strbuf_pop(qtk_strbuf_t *s,char* data,int bytes)
{
	int ret;

	if(s->pos<bytes)
	{
		ret=-1;goto end;
	}
	if(data)
	{
		memcpy(data,s->data,bytes);
	}
	s->pos-=bytes;
	if(s->pos>0)
	{
		memmove(s->data,&(s->data[bytes]),s->pos);
	}
	ret=0;
end:
	return ret;
}
