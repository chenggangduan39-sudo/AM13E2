#include "qtk_extract.h" 

#define QTK_EXTRACT_INT_MASK 0x00000f00
#define QTK_EXTRACT_SHORT_MASK 0x0f00
#define QTK_EXTRACT_OFFSET 8

static int qtk_extract_align(qtk_extract_t *ex);
static int qtk_extract_get(qtk_extract_t *ex,char *buffer,int len);

qtk_extract_t* qtk_extract_new(int buf_size)
{
	qtk_extract_t *ex;

	ex = (qtk_extract_t*)wtk_malloc(sizeof(*ex));
	if(!ex) {
		return NULL;
	}

	ex->buf_size = buf_size;
	ex->buf = wtk_strbuf_new(ex->buf_size,1);
	ex->align = 0;

	return ex;
}

void qtk_extract_delete(qtk_extract_t *ex)
{
	wtk_strbuf_delete(ex->buf);
	wtk_free(ex);
};

void qtk_extract_start(qtk_extract_t *ex)
{
	ex->align = 0;
	wtk_strbuf_reset(ex->buf);
}

void qtk_extract_reset(qtk_extract_t *ex)
{
	//wtk_debug("buf pos = %d\n",ex->buf->pos);
}

int qtk_extract_proc(qtk_extract_t *ex,char *data,int bytes,char *buffer,int len)
{
	wtk_strbuf_t *buf = ex->buf;
	int ret;

	wtk_strbuf_push(buf,data,bytes);
	if(ex->align == 0) {
		ret = qtk_extract_align(ex);
		printf("align = %d\n",ret);
		if(ret == -1) {
			goto end;
		} else {
			wtk_strbuf_pop(buf,NULL,ret);
			ex->align = 1;
		}
	}

	ret = qtk_extract_get(ex,buffer,len);

end:
	return ret;
}

static int qtk_extract_align(qtk_extract_t *ex)
{
	wtk_strbuf_t *buf = ex->buf;
	int *ps,*pe;
	int expect = 0;
	int flag;
	int offset = -1;

	ps = (int*)buf->data;
	pe = ps + buf->pos / 4;
	while(ps < pe) {
		flag = ((*ps) & QTK_EXTRACT_INT_MASK) >> QTK_EXTRACT_OFFSET;
		//wtk_debug("flag = %d\n",flag);
		if(flag == expect) {
			++expect;
			if(expect == 8) {
				offset = ((char*)ps) - buf->data - 28;
				break;
			}
		} else if(flag == 0) {
			expect = 1;
		} else {
			expect = 0;
		}
		++ps;
	}

	return offset;
}

#if 0
static int qtk_extract_get(qtk_extract_t *ex,char *buffer,int len)
{
	wtk_strbuf_t *buf = ex->buf;
	short *ps,*pe;
	short *px;
	int win;


	win = min(len,buf->pos >> 1);
	win = win - (win % 16);
	//wtk_debug("win = %d\n",win);
	px = (short*)buffer;
	ps = (short*)buf->data;
	pe = ps + win;
	while(ps < pe) {
		*px++ = *(++ps);
		++ps;
	}
	wtk_strbuf_pop(buf,NULL,win*2);

	return win;
}
#else
static int qtk_extract_get(qtk_extract_t *ex,char *buffer,int len)
{
	wtk_strbuf_t *buf = ex->buf;
	short *pv;
	short *px;
	int win;
	int i,j,k;
	int flag;

	win = min(len,buf->pos >> 1);
	win = win - (win % 16);
	//wtk_debug("win = %d\n",win);
	px = (short*)buffer;
	pv = (short*)buf->data;
	for(i=0,j=0;j<win;i+=8,j+=16) {
		for(k=0;k<8;++k) {
			flag = (pv[j+k*2] & QTK_EXTRACT_SHORT_MASK ) >> QTK_EXTRACT_OFFSET;
			px[i+flag] = pv[j+k*2+1];
		}
	}

	wtk_strbuf_pop(buf,NULL,win*2);
	return win;
}
#endif
