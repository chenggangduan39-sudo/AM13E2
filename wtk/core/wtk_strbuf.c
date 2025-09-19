#include "wtk_strbuf.h"
#include "wtk_os.h"
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>

wtk_strbuf_t* wtk_strbuf_new(int init_len,float rate)
{
    wtk_strbuf_t *b;
    char *data;

    data=(char*)wtk_malloc(init_len);
    if(!data){b=0;goto end;}
    b=(wtk_strbuf_t*)wtk_malloc(sizeof(*b));
    b->data=data;
    b->length=init_len;
    b->pos=0;
    b->rate=1.0+rate;
end:
    return b;
}

void wtk_strbuf_resize(wtk_strbuf_t *buf,int size)
{
	wtk_free(buf->data);
	buf->data=(char*)wtk_malloc(size);
	buf->length=size;
	buf->pos=0;
}

int wtk_strbuf_bytes(wtk_strbuf_t *b)
{
	return sizeof(*b)+b->length;
}

int wtk_strbuf_delete(wtk_strbuf_t* b)
{
    wtk_free(b->data);
    wtk_free(b);
    return 0;
}

wtk_strbuf_t** wtk_strbufs_new(int n)
{
	wtk_strbuf_t **bufs;
	int i;

	bufs=(wtk_strbuf_t**)wtk_calloc(n,sizeof(wtk_strbuf_t*));
	for(i=0;i<n;++i)
	{
		bufs[i]=wtk_strbuf_new(1024,1);
	}
	return bufs;
}

int wtk_strbufs_bytes(wtk_strbuf_t **bufs,int n)
{
	int bytes=0;
	int i;

	for(i=0;i<n;++i)
	{
		bytes+=wtk_strbuf_bytes(bufs[i]);
	}
	return bytes;
}

void wtk_strbufs_delete(wtk_strbuf_t **bufs,int n)
{
	int i;

	for(i=0;i<n;++i)
	{
		wtk_strbuf_delete(bufs[i]);
	}
	wtk_free(bufs);
}

void wtk_strbufs_reset(wtk_strbuf_t **bufs,int n)
{
	int i;

	for(i=0;i<n;++i)
	{
		if(bufs[i]->length>1024)
		{
			wtk_strbuf_resize(bufs[i],1024);
		}else
		{
			wtk_strbuf_reset(bufs[i]);
		}
	}
}

void wtk_strbufs_push_short(wtk_strbuf_t **bufs,int n,short **data,int len)
{
	int i;

	len=len*2;
	for(i=0;i<n;++i)
	{
		wtk_strbuf_push(bufs[i],(char*)data[i],len);
	}
}

void wtk_strbufs_push_int(wtk_strbuf_t **bufs,int n,int **data,int len)
{
	int i;

	len=len*4;
	for(i=0;i<n;++i)
	{
		wtk_strbuf_push(bufs[i],(char*)data[i],len);
	}
}

void wtk_strbufs_push_float(wtk_strbuf_t **bufs,int n,float **data,int len)
{
	int i;

	len=len*4;
	for(i=0;i<n;++i)
	{
		if(data)
		{
			wtk_strbuf_push(bufs[i],(char*)data[i],len);
		}else
		{
			wtk_strbuf_push(bufs[i],NULL,len);
		}
	}
}

void wtk_strbufs_pop(wtk_strbuf_t **bufs,int n,int len)
{
	int i;

	for(i=0;i<n;++i)
	{
		wtk_strbuf_pop(bufs[i],NULL,len);
	}
}

void wtk_strbuf_expand(wtk_strbuf_t *s,int bytes)
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
        s->data=(char*)wtk_malloc(alloc);
        s->length=alloc;
        memcpy(s->data,p,s->pos);
        wtk_free(p);
    }
    return;
}

void wtk_strbuf_push_int(wtk_strbuf_t *buf,int *p,int n)
{
	wtk_strbuf_push(buf,(char*)p,n*sizeof(int));
}

void wtk_strbuf_push_int_front(wtk_strbuf_t *buf,int *p,int n)
{
	wtk_strbuf_push_front(buf,(char*)p,n*sizeof(int));
}

void wtk_strbuf_push_float(wtk_strbuf_t *buf,float *p,int n)
{
	wtk_strbuf_push(buf,(char*)p,n*sizeof(float));
}

void wtk_strbuf_push_float_front(wtk_strbuf_t *buf,float *p,int n)
{
	wtk_strbuf_push_front(buf,(char*)p,n*sizeof(float));
}

void wtk_strbuf_push(wtk_strbuf_t *s,const char *buf,int bytes)
{
    if(bytes<=0){return;}
    if(bytes>s->length-s->pos)
    {
        wtk_strbuf_expand(s,bytes);
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

void wtk_strbuf_push_24bit(wtk_strbuf_t *buf,char *data,int len)
{
	int i,n;
	int *pv;
	int v;

	n=len/3;
	wtk_strbuf_expand(buf,n<<2);
	pv=(int*)(buf->pos+buf->data);
	for(i=0;i<n;++i)
	{
		if(data[2]&0x80)
		{
			v=((*(int*)(data)) & 0x00ffffff)|0xff000000;
		}else
		{
			v=(*(int*)(data))&0x00ffffff;
		}
		pv[i]=v;///256;
		data+=3;
	}
	buf->pos+=n<<2;
}

void wtk_strbuf_push_word(wtk_strbuf_t *buf,char *data,int bytes)
{
	if(buf->pos>0 && (bytes==1 || wtk_utf8_bytes(data[0])==1) && (isalpha(data[0])))
	{
		wtk_strbuf_push_s(buf," ");
	}
	wtk_strbuf_push(buf,data,bytes);
}

void wtk_strbuf_push_front(wtk_strbuf_t *s,const char *buf,int bytes)
{
    int left;

    if(bytes<0){return;}
    left=s->length-s->pos;
    if(bytes>left)
    {
        wtk_strbuf_expand(s,bytes);
    }
    memmove(s->data+bytes,s->data,s->pos);
    if(buf)
    {
    	memcpy(s->data,buf,bytes);
    }else
    {
    	memset(s->data,0,bytes);
    }
    s->pos+=bytes;
    return;
}

/*
void wtk_strbuf_push_c(wtk_strbuf_t *s,char b)
{
	//wtk_strbuf_push(s,&b,1);
    if(s->length<=s->pos)
    {
        wtk_strbuf_expand(s,1);
    }
    s->data[s->pos++]=b;
}
*/

void wtk_strbuf_push_f(wtk_strbuf_t *b,const char *fmt,...)
{
	char buf[4096]={0};
	va_list ap;
	int n;

	va_start(ap,fmt);
	n=vsprintf(buf,fmt,ap);
	wtk_strbuf_push(b,buf,n);
	va_end(ap);
}

int wtk_strbuf_pop(wtk_strbuf_t *s,char* data,int bytes)
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

char* wtk_strbuf_to_str(wtk_strbuf_t *s)
{
	char *p=0;

	if(s->pos<=0){goto end;}
	p=(char*)wtk_malloc(s->pos+1);
	memcpy(p,s->data,s->pos);
	p[s->pos]=0;
end:
	return p;
}

void wtk_strbuf_merge(wtk_strbuf_t *buf,wtk_string_t *p1,...)
{
    va_list ap;

    va_start(ap,p1);
    while(p1)
    {
    	wtk_strbuf_push(buf,p1->data,p1->len);
        p1=va_arg(ap,wtk_string_t*);
    }
    va_end(ap);
}

void wtk_strbuf_merge2(wtk_strbuf_t *buf,char *p1,...)
{
    va_list ap;

    va_start(ap,p1);
    while(p1)
    {
    	wtk_strbuf_push(buf,p1,strlen(p1));
        p1=va_arg(ap,char*);
    }
    va_end(ap);
}

/*
void wtk_strbuf_reset(wtk_strbuf_t *s)
{
    s->pos=0;
}
*/

void wtk_strbuf_print(wtk_strbuf_t *s)
{
    //print_data(s->data,s->pos);
    printf("%*s\n",s->pos,s->data);
}

int wtk_strbuf_read2(wtk_strbuf_t *buf,char *fn)
{
	FILE *f;
	int ret=-1;
	char tmp[4096];
	int n;

	f=fopen(fn,"rb");
	if(!f){goto end;}
	wtk_strbuf_reset(buf);
	while(1)
	{
		n=fread(tmp,1,sizeof(tmp),f);
		if(n>0)
		{
			wtk_strbuf_push(buf,tmp,n);
		}
		if(n<sizeof(tmp)){break;}
	}
	ret=0;
end:
	if(f){fclose(f);}
	return ret;
}

void wtk_strbuf_strip(wtk_strbuf_t *buf)
{
	int i,n;

	for(i=0,n=0;i<buf->pos;++i)
	{
#ifdef WIN32
		if(isspace((unsigned char)buf->data[i]))
#else
		if(isspace(buf->data[i]))
#endif
		{
			++n;
		}else
		{
			break;
		}
	}
	//wtk_debug("[%d]\n",n);
	if(n>0)
	{
		wtk_strbuf_pop(buf,0,n);
	}
	for(i=buf->pos-1;i>=0;--i)
	{
#ifdef WIN32
		if(isspace((unsigned char)buf->data[i]))
#else
		if(isspace(buf->data[i]))
#endif
		{
			--buf->pos;
		}else
		{
			break;
		}
	}
}

void wtk_strbuf_pad0(wtk_strbuf_t *buf,int bytes)
{
	wtk_strbuf_expand(buf,bytes);
	memset(buf->data+buf->pos,0,bytes);
	buf->pos+=bytes;
}

void wtk_strbuf_replace(wtk_strbuf_t *buf,char *data,int data_len,char *src,int src_len,char *dst,int dst_len)
{
	char *s,*e;
	int i;

	wtk_strbuf_reset(buf);
	s=data;e=s+data_len;
	while(s<e)
	{
		i=wtk_str_str(s,e-s,src,src_len);
		if(i<0)
		{
			wtk_strbuf_push(buf,s,e-s);
			break;
		}
		if(i>0)
		{
			wtk_strbuf_push(buf,s,i);
		}
		wtk_strbuf_push(buf,dst,dst_len);
		s+=i+src_len;
	}
}

#ifndef BUF_USE_MACRO
void wtk_strbuf_push_c(wtk_strbuf_t *buf,char b)
{
    if(buf->length<=buf->pos)
    {
        wtk_strbuf_expand(buf,1);
    }
    buf->data[buf->pos++]=b;
}
#endif

void wtk_strbuf_push_skip_ws(wtk_strbuf_t *buf,char *data,int len)
{
	char *s,*e;
	char c;

	s=data;e=s+len;
	while(s<e)
	{
		c=*s;
		if(c==' '||c=='\t')
		{

		}else
		{
			wtk_strbuf_push_c(buf,c);
		}
		++s;
	}
}

void wtk_strbuf_push_skip_utf8_ws(wtk_strbuf_t *buf,char *data,int len)
{
	char *s,*e;
	char c;
	int n;
	int prev_n=-1;
	int space=0;

	s=data;e=s+len;
	while(s<e)
	{
		c=*s;
		n=wtk_utf8_bytes(c);
		if(n==1)
		{
			if(prev_n>1 && (c==' '||c=='\t'))
			{

			}else
			{
				if(c==' ' || c=='\t')
				{
					space=1;
				}else
				{
					if(space)
					{
						wtk_strbuf_push_c(buf,' ');
					}
					space=0;
					wtk_strbuf_push_c(buf,c);
				}
			}
		}else
		{
			space=0;
			wtk_strbuf_push(buf,s,n);
		}
		prev_n=n;
		s+=n;
	}
}

void wtk_strbuf_push_add_escape_str(wtk_strbuf_t *buf,char *data,int bytes)
{
	char *s,*e;
	char c;

	s=data;e=s+bytes;
	while(s<e)
	{
		c=*s;
#ifdef WIN32
		if(c=='\"' ||c=='\\')
#else
		if(c=='\'' || c=='\"' ||c=='\\')
#endif
		{
			wtk_strbuf_push_c(buf,'\\');
		}
		wtk_strbuf_push_c(buf,c);
		++s;
	}
}

void wtk_strbuf_string_to_str(wtk_strbuf_t *buf,char *data,int bytes)
{
	wtk_strbuf_reset(buf);
	wtk_strbuf_push(buf,data,bytes);
	wtk_strbuf_push_c(buf,0);
}

void wtk_strbuf_parse_quote(wtk_strbuf_t *buf,char *data,int bytes)
{
	char quote;
	char *s,*e;
	char c;
	int esc;

	s=data;e=s+bytes;
	wtk_strbuf_reset(buf);
	quote=*s;
	if(quote!='\'' && quote !='"')
	{
		wtk_strbuf_push(buf,data,bytes);
		return;
	}
	++s;
	esc=0;
	while(s<e)
	{
		c=*s;

		if(esc)
		{
			wtk_strbuf_push_c(buf,c);
			esc=0;
		}else
		{
			if(c=='\\')
			{
				esc=1;
			}else if(c==quote)
			{
				break;
			}else
			{
				wtk_strbuf_push_c(buf,c);
			}
		}
		++s;
	}

}

static wtk_string_t num[]={
		wtk_string("\xe9\x9b\xb6"), // 0
		wtk_string("\xe4\xb8\x80"), // 1
		wtk_string("\xe4\xba\x8c"), // 2
		wtk_string("\xe4\xb8\x89"), // 3
		wtk_string("\xe5\x9b\x9b"), // 4
		wtk_string("\xe4\xba\x94"), // 5
		wtk_string("\xe5\x85\xad"), // 6
		wtk_string("\xe4\xb8\x83"), // 7
		wtk_string("\xe5\x85\xab"), // 8
		wtk_string("\xe4\xb9\x9d"), // 9
};

static wtk_string_t num_en[]={
		wtk_string("zero"),
		wtk_string("one"),
		wtk_string("two"),
		wtk_string("three"),
		wtk_string("four"),
		wtk_string("five"),
		wtk_string("six"),
		wtk_string("seven"),
		wtk_string("eight"),
		wtk_string("nine"),
		wtk_string("ten"),
		wtk_string("eleven"),
		wtk_string("twelve"),
		wtk_string("thirteen"),
		wtk_string("fourteen"),
		wtk_string("fifteen"),
		wtk_string("sixteen"),
		wtk_string("seventeen"),
		wtk_string("eighteen"),
		wtk_string("nineteen"),
};

void wtk_stochn(wtk_strbuf_t *buf,char *data,int len)
{
	char *s,*e;
	int v;

	wtk_strbuf_reset(buf);
	s=data;e=s+len;
	while(s<e)
	{
		v=*s-'0';
		if(v<10)
		{
			wtk_strbuf_push(buf,num[v].data,num[v].len);
		}
		++s;
	}
}

void wtk_stoen(wtk_strbuf_t *buf,char *data,int len)
{
	char *s,*e;
	int v;

	wtk_strbuf_reset(buf);
	s=data;e=s+len;
	while(s<e)
	{
		v=*s-'0';
		if(v<10)
		{
			wtk_strbuf_push(buf,num_en[v].data,num_en[v].len);
			wtk_strbuf_push_c(buf,' ');
		}
		++s;
	}
}


static wtk_string_t num2[]={
		wtk_string("\xe9\x9b\xb6"), // 0
		wtk_string("\xe5\xb9\xba"), // 1(yao)
		wtk_string("\xe4\xba\x8c"), // 2
		wtk_string("\xe4\xb8\x89"), // 3
		wtk_string("\xe5\x9b\x9b"), // 4
		wtk_string("\xe4\xba\x94"), // 5
		wtk_string("\xe5\x85\xad"), // 6
		wtk_string("\xe4\xb8\x83"), // 7
		wtk_string("\xe5\x85\xab"), // 8
		wtk_string("\xe4\xb9\x9d"), // 9
};

void wtk_stotel(wtk_strbuf_t *buf,char *data,int len)
{
	char *s,*e;
	int v;

	wtk_strbuf_reset(buf);
	s=data;e=s+len;
	while(s<e)
	{
		v=*s-'0';
		if(v<10)
		{
			wtk_strbuf_push(buf,num2[v].data,num2[v].len);
		}
		++s;
	}
}

typedef struct
{
	int v;
	wtk_string_t s;
}wtk_iunit_t;

void wtk_itochn2(wtk_strbuf_t *buf,int v,int last_u)
{
	static wtk_iunit_t unit1[]={
			{100000000,wtk_string("\xe4\xba\xbf")},
			{10000,wtk_string("\xe4\xb8\x87")},
	};
	static wtk_iunit_t unit2[]={
			{1000,wtk_string("\xe5\x8d\x83")},
			{100,wtk_string("\xe7\x99\xbe")},
			{10,wtk_string("\xe5\x8d\x81")},
	};
	int x;
	int i;

	for(i=0;i<sizeof(unit1)/sizeof(wtk_iunit_t);++i)
	{
		x=v/(unit1[i].v);
		if(x>0)
		{
			//wtk_debug("x=%d\n",x);
			wtk_itochn2(buf,x,last_u);
			wtk_strbuf_push(buf,unit1[i].s.data,unit1[i].s.len);
			v=v%(unit1[i].v);
			//last_u=unit1[i].v;
			wtk_itochn2(buf,v,unit1[i].v);
			return;
		}
	}
	for(i=0;i<sizeof(unit2)/sizeof(wtk_iunit_t);++i)
	{
		x=v/(unit2[i].v);
		if(x>0 && x<10)
		{
			//wtk_debug("x=%d\n",x);
			//wtk_debug("last=%d/%d\n",last_u,unit2[i].v);
			if(last_u/unit2[i].v>10)
			{
				wtk_strbuf_push(buf,num[0].data,num[0].len);
			}
			if(x==1 && buf->pos==0 && unit2[i].v==10)
			{

			}else
			{
				wtk_strbuf_push(buf,num[x].data,num[x].len);
			}
			wtk_strbuf_push(buf,unit2[i].s.data,unit2[i].s.len);
			v=v%(unit2[i].v);
			last_u=unit2[i].v;
			//wtk_debug("[%.*s]\n",buf->pos,buf->data);
		}
	}
	//wtk_debug("v=%d last_u=%d\n",v,last_u);
	if(v>0 && v<10)
	{
		if(last_u>10)
		{
			wtk_strbuf_push(buf,num[0].data,num[0].len);
		}
		wtk_strbuf_push(buf,num[v].data,num[v].len);
	}else if(v==0 && buf->pos==0)
	{
		wtk_strbuf_push(buf,num[v].data,num[v].len);
	}
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	//wtk_debug("v=%d\n",v);
	//exit(0);
}

void wtk_itoen2(wtk_strbuf_t *buf,int v,int last_u)
{
	static wtk_iunit_t unit1[]={
			{1000000000,wtk_string("billion")},	//十亿
			{1000000,wtk_string("million")},	//百万
			{1000,wtk_string("thousand")},	//千
	};
	static wtk_iunit_t en_hundred = {100,wtk_string("hundred")};

	static wtk_iunit_t unit2[]={
			{20,wtk_string("twenty")},
			{30,wtk_string("thirty")},
			{40,wtk_string("forty")},
			{50,wtk_string("fifty")},
			{60,wtk_string("sixty")},
			{70,wtk_string("seventy")},
			{80,wtk_string("eighty")},
			{90,wtk_string("ninety")},
	};

	char *and = " and";
	int x;
	int i;

	for(i=0;i<sizeof(unit1)/sizeof(wtk_iunit_t);++i)
	{
		x=v/(unit1[i].v);
		if(x>0)
		{
			//wtk_debug("x=%d\n",x);
			wtk_itoen2(buf,x,last_u);
			wtk_strbuf_push_c(buf,' ');
			wtk_strbuf_push(buf,unit1[i].s.data,unit1[i].s.len);
			v=v%(unit1[i].v);
			wtk_itoen2(buf,v,unit1[i].v);
			return;
		}
	}
	//hundred 百
	x = v/en_hundred.v;
	if(x > 0){
		if(last_u){
			wtk_strbuf_push(buf,",",1);
		}
		wtk_strbuf_push(buf,num_en[x].data,num_en[x].len);
		wtk_strbuf_push_c(buf,' ');
		wtk_strbuf_push(buf,en_hundred.s.data,en_hundred.s.len);
		last_u = en_hundred.v;
		v = v%en_hundred.v;
	}
	//百位以后
	x = v/10;
	if(x > 1){
		if(last_u > 100){
			wtk_strbuf_push(buf,",",1);
		}else{
			wtk_strbuf_push_c(buf,' ');
		}
		wtk_strbuf_push(buf,unit2[x-2].s.data,unit2[x-2].s.len);
		last_u = unit2[x-2].v;
		v = v%unit2[x-2].v;
	}
	//二十位以内
	if(last_u/10>=10)
	{
		if(v == 10 && last_u/10 >= 100){
			wtk_strbuf_push(buf,",",1);
		}else{
			wtk_strbuf_push(buf,and,strlen(and));
		}
	}
	if(v > 0){
		wtk_strbuf_push_c(buf,' ');
		wtk_strbuf_push(buf,num_en[v].data,num_en[v].len);
	}
	return;
}

void wtk_itochn(wtk_strbuf_t *buf,int v)
{
	wtk_strbuf_reset(buf);
	wtk_itochn2(buf,v,0);
}

void wtk_itoen(wtk_strbuf_t *buf,int v)
{
	wtk_strbuf_reset(buf);
	wtk_itoen2(buf,v,0);
}

#include "wtk/core/wtk_larray.h"

void wtk_strbuf_real_path(wtk_strbuf_t *buf,char *name,int len)
{
enum
{
	WTK_RP_INIT,
};
	char *s,*e;
	int state;
	int n;
	char c;
	wtk_larray_t *a;
	wtk_heap_t *heap;
	wtk_string_t *v;
	wtk_string_t **strs;
	int i;

	//wtk_debug("[%.*s]\n",len,name);
	heap=wtk_heap_new(4096);
	a=wtk_larray_new(len/3,sizeof(void*));
	wtk_strbuf_reset(buf);
	s=name;
	e=s+len;
	state=WTK_RP_INIT;
	while(s<e)
	{
		c=*s;
		n=wtk_utf8_bytes(c);
		switch(state)
		{
		case WTK_RP_INIT:
			if(n==1 && *s=='/')
			{
				//wtk_debug("[%.*s]\n",buf->pos,buf->data);
				if(buf->pos>0)
				{
					if(wtk_str_equal_s(buf->data,buf->pos,".."))
					{
						if(a->nslot>0)
						{
							--a->nslot;
						}
					}else if(wtk_str_equal_s(buf->data,buf->pos,"."))
					{
						if(a->nslot==0)
						{
							v=wtk_heap_dup_string(heap,buf->data,buf->pos);
							wtk_larray_push2(a,&v);
						}
					}else
					{
						v=wtk_heap_dup_string(heap,buf->data,buf->pos);
						wtk_larray_push2(a,&v);
					}
					wtk_strbuf_reset(buf);
				}
			}else
			{
				wtk_strbuf_push(buf,s,n);
			}
			break;
		}
		s+=n;
	}
	if(buf->pos>0)
	{
		if(wtk_str_equal_s(buf->data,buf->pos,".."))
		{
			if(a->nslot>0)
			{
				--a->nslot;
			}
		}else if(wtk_str_equal_s(buf->data,buf->pos,"."))
		{
			if(a->nslot==0)
			{
				v=wtk_heap_dup_string(heap,buf->data,buf->pos);
				wtk_larray_push2(a,&v);
			}
		}else
		{
			v=wtk_heap_dup_string(heap,buf->data,buf->pos);
			wtk_larray_push2(a,&v);
		}
	}
	strs=(wtk_string_t**)(a->slot);
	n=a->nslot;
	//wtk_debug("n=%d\n",a->nslot);
	wtk_strbuf_reset(buf);
	for(i=0;i<n;++i)
	{
		//wtk_debug("[%.*s]\n",strs[i]->len,strs[i]->data);
		if(buf->pos>0)
		{
			wtk_strbuf_push_s(buf,"/");
		}
		wtk_strbuf_push(buf,strs[i]->data,strs[i]->len);
	}
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	wtk_heap_delete(heap);
	wtk_larray_delete(a);
	//exit(0);
}

void wtk_strbuf_control_cache(wtk_strbuf_t *buf,int max_cache)
{
	char *data;

	if(buf->length>max_cache && buf->pos<=max_cache)
	{
		data=wtk_malloc(max_cache);
		memcpy(data,buf->data,buf->pos);
		wtk_free(buf->data);
		buf->data=data;
		buf->length=max_cache;
	}
}

#include <math.h>
void qtk_chn2num(wtk_strbuf_t *buf, char *chnnum, int len)
{
	char *s = chnnum;
	char *e = s + len;
	int step;
	int state;
	double number = 0.0;
	char *strnum;
	int ret = 0, i = 0;
	int pre_unit=0;
	wtk_strbuf_t *temp = wtk_strbuf_new(512, 1);
	//char *strgbk;
	enum qtk_chn2num_state_t
	{
		qtk_chn2num_start,
		qtk_chn2num_feed_wait_eq,
		qtk_chn2num_feed_value,
	};
	state = qtk_chn2num_start;

	typedef struct
	{
		int num;
		wtk_string_t str;
	}qtk_iunit_t;
	static qtk_iunit_t unit[] = {
		{ 100000000, wtk_string("亿") },
		{ 10000000, wtk_string("千万") },
		{ 1000000, wtk_string("百万") },
		{ 10000, wtk_string("万") },
		{ 1000, wtk_string("千") },
		{ 100, wtk_string("百") },
		{ 10, wtk_string("十") },
		{ 1, wtk_string("个") },
		{ -1, wtk_string("浮点") },
	};

	while (s<e)
	{
		step = wtk_utf8_bytes(*s);

		//strgbk = wtk_utf8_to_gbk(s,step);
		//wtk_debug("[%s]\n", strgbk);

		switch (state)
		{
		case qtk_chn2num_start:
			ret = (s[0] == '(') ? 0 : -1;
			if (ret == 0)
			{
				wtk_strbuf_reset(temp);
				state = qtk_chn2num_feed_wait_eq;
			}
			break;

		case qtk_chn2num_feed_wait_eq:
			if (s[0] == '=')
			{
				for (i = 0; i < sizeof(unit) / sizeof(qtk_iunit_t); i++)
				{
					//printf("============>%s\n", gbk_to_utf8(unit[i].s.data));
					//printf("============> %d %s\n", i, unit[i].s.data);
					//char *strutf8 = gbk_to_utf8(unit[i].str.data);//unit是gbk类型
					char *strutf8 = unit[i].str.data;
					//int len = strlen(strutf8);
					if (wtk_str_equal(strutf8, strlen(strutf8), temp->data, temp->pos))//喂入的是utf8类型temp
					{
						//wtk_debug("i=%d get key name:%s  : %d\n", i, unit[i].str.data, unit[i].num);

						//wtk_debug("[%.*s :len=%d][%.*s :len=%d]", strlen(strutf8), strutf8, strlen(strutf8), temp->pos, temp->data, temp->pos);
						pre_unit = unit[i].num;
						wtk_strbuf_reset(temp);
						state = qtk_chn2num_feed_value;
						//wtk_free(strutf8);
						break;
					}
					//wtk_free(strutf8);
				}
			}
			else
			{
				wtk_strbuf_push(temp, s, step);
			}
			break;
		case qtk_chn2num_feed_value:
			if ((s[0] == ',') || (s[0] == ')'))
			{
				double x = 0.0;
				double p = 0.0;
				//wtk_debug("get->%.*s\n", temp->pos, temp->data);
				if (temp->pos > 0)
				{
					x = wtk_str_atoi(temp->data, temp->pos);
					if (pre_unit == -1)
					{
						p = pow(10, temp->pos);
						//printf("%lf  %d\n",p, temp->pos);
						number += x / p;
					}
					else
					{
						number += x*pre_unit;
					}
				}

				if (s[0] == ')')
				{
					//wtk_debug("get num =%lf\n", number);
					strnum = wtk_dtoa(number);
					wtk_strbuf_reset(buf);
					wtk_strbuf_push(buf, strnum, strlen(strnum));
					wtk_free(strnum);
				}
				else
				{
					wtk_strbuf_reset(temp);
					state = qtk_chn2num_feed_wait_eq;
				}
			}
			else
			{
				wtk_strbuf_push(temp, s, step);
			}

			break;

		default:
			ret = -1;
			break;
		}


		if (ret != 0)
		{
			wtk_debug("err at %.*s", (int)(e - s), s);
			break;
		}
		s = s + step;
	}
	//wtk_debug("%.*s\n", len, chnnum);
	wtk_strbuf_delete(temp);
}
