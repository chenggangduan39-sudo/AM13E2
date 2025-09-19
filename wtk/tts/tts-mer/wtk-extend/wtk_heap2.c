#include "wtk_heap2.h"

wtk_strbuf_t* wtk_strbuf_heap_new( wtk_heap_t *heap, int init_len,float rate)
{
    wtk_strbuf_t *b;
    char *data;

    data=(char*)wtk_heap_malloc( heap, init_len);
    if(!data){b=0;goto end;}
    b=(wtk_strbuf_t*)wtk_heap_malloc(heap, sizeof(*b));
    b->data=data;
    b->length=init_len;
    b->pos=0;
    b->rate=1.0+rate;
end:
    return b;
}

wtk_mati_t* wtk_mati_heap_new(wtk_heap_t *heap, int row,int col)
{
	wtk_mati_t *m;
	char *p;
	
	p=wtk_heap_malloc(heap, sizeof(wtk_mati_t)+row*col*sizeof(int)+16);
	m=(wtk_mati_t*)p;
	m->row=row;
	m->col=col;
	m->p=wtk_align_ptr(p+sizeof(wtk_mati_t),16);
	return m;
}

wtk_matf_t* wtk_matf_heap_new(wtk_heap_t *heap, int row,int col)
{
	wtk_matf_t *m;
	char *p;
	
	p=wtk_heap_zalloc(heap, sizeof(wtk_matf_t)+row*col*sizeof(float)+16);
	m=(wtk_matf_t*)p;
	m->row=row;
	m->col=col;
	m->p=wtk_align_ptr(p+sizeof(wtk_matf_t),16);
	return m;
}

wtk_matdf_t* wtk_matdf_heap_new(wtk_heap_t *heap, int row,int col)
{
	wtk_matdf_t *m;
	char *p;
	
	p=wtk_heap_malloc(heap, sizeof(wtk_matdf_t)+row*col*sizeof(double)+16);
	m=(wtk_matdf_t*)p;
	m->row=row;
	m->col=col;
	m->p=wtk_align_ptr(p+sizeof(wtk_matdf_t),16);
	return m;
}

wtk_veci_t* wtk_veci_heap_new( wtk_heap_t *heap, int len)
{
	wtk_veci_t *v;
	char *p;
	
	p=wtk_heap_malloc( heap, sizeof(wtk_veci_t)+len*sizeof(float)+16);
	v=(wtk_veci_t*)p;
	v->len=len;
	v->p=wtk_align_ptr(p+sizeof(wtk_veci_t),16);
	wtk_veci_zero(v);
	return v;
}
