#include "wtk_buf.h" 
#include "wtk/core/wtk_alloc.h"

wtk_buf_item_t* wtk_buf_item_new(int size)
{
	wtk_buf_item_t *item;

	//size+=sizeof(wtk_buf_item_t);
	item=(wtk_buf_item_t*)wtk_malloc(sizeof(wtk_buf_item_t));
	item->next=NULL;
	item->data=(char*)wtk_malloc(size);//((char*)item)+sizeof(wtk_buf_item_t);
	item->pos=0;
	item->len=0;
	return item;
}

void wtk_buf_item_delete(wtk_buf_item_t *item)
{
	wtk_free(item->data);
	wtk_free(item);
}

wtk_buf_t* wtk_buf_new(int buf_size)
{
	wtk_buf_t *buf;

	buf=(wtk_buf_t*)wtk_malloc(sizeof(wtk_buf_t));
	buf->buf_size=buf_size;
	buf->front=buf->end=wtk_buf_item_new(buf_size);
        buf->max_len = -1;
        return buf;
}

void wtk_buf_delete(wtk_buf_t *buf)
{
	wtk_buf_item_t *item,*next;

	//wtk_debug("==============> delete buf=%p buf_size=%d\n",buf,buf->buf_size);
	item=buf->front;
	while(item)
	{
		next=item->next;
		wtk_buf_item_delete(item);
		item=next;
	}
	wtk_free(buf);
}

void wtk_buf_reset(wtk_buf_t *buf)
{
    wtk_buf_item_t *item, *next, *n;

    item = buf->front;
    if (!item) {
        return;
    }
    next = item->next;
    while (next) {
        n = next->next;
        wtk_buf_item_delete(next);
        next = n;
    }
        item->pos=0;
	item->len=0;
        item->next = NULL;
        buf->end=item;
}

int wtk_buf_len(wtk_buf_t *buf)
{
	int len=0;
	wtk_buf_item_t *item=buf->front;

	while(item)
	{
		len+=item->len;
		item=item->next;
	}
	return len;
}

void wtk_buf_print(wtk_buf_t *buf)
{
	wtk_buf_item_t *item=buf->front;

	wtk_debug("=============== buf=%p ================= \n",buf);
	while(item)
	{
		print_hex(item->data+item->pos,item->len);
		item=item->next;
	}
}

wtk_buf_it_t wtk_buf_get_it(wtk_buf_t *buf)
{
	wtk_buf_it_t it;

	it.item=buf->front;
	it.pos=it.item->pos;
	it.buf_size=buf->buf_size;
	return it;
}

char* wtk_buf_it_next(wtk_buf_it_t *it)
{
	char *p;

	if(!it->item)
	{
		return NULL;
	}
	if(it->pos>=it->buf_size)
	{
		wtk_debug("next %d/%d....\n",it->pos,it->buf_size);
		it->item=it->item->next;
		if(it->item)
		{
			it->pos=it->item->pos;
		}else
		{
			it->pos=0;
		}
		return wtk_buf_it_next(it);
	}
	p=it->item->data+it->pos;
	++it->pos;
	if(it->pos>=it->buf_size)
	{
		wtk_debug("next %d/%d next=%p....\n",it->pos,it->buf_size,it->item->next);
		it->item=it->item->next;
		if(it->item)
		{
			wtk_debug("pos=%d len=%d\n",it->item->pos,it->item->len);
			it->pos=it->item->pos;
		}else
		{
			it->pos=0;
		}
	}
	return p;
}

void wtk_buf_push(wtk_buf_t *buf,char *data,int len)
{
	wtk_buf_item_t *item=buf->end;
	char *s,*e;
	int n;
        int cur_len, pop_len;

        if (buf->max_len > 0) {
            cur_len = wtk_buf_len(buf);
            pop_len = cur_len + len - buf->max_len;
            if (pop_len > 0) {
                wtk_buf_pop(buf, NULL, pop_len);
            }
        }

        s=data;e=s+len;
	//wtk_debug("buf=%p buf_size=%d len=%d\n",buf,buf->buf_size,len);
	while(s<e)
	{
		n=min(e-s,buf->buf_size-(item->len+item->pos));
		//wtk_debug("buf=%p buf_size=%d n=%d/%d len=%d pos=%d\n",buf,buf->buf_size,n,(int)(e-s),item->len,item->pos);
		memcpy(item->data+item->pos+item->len,s,n);
		//wtk_debug("buf=%p buf_size=%d n=%d\n",buf,buf->buf_size,n);
		item->len+=n;
		s+=n;
		//wtk_debug("pos=%d")
		if((item->pos+item->len)>=buf->buf_size)
		{
			buf->end=item->next=wtk_buf_item_new(buf->buf_size);
			item=item->next;
		}
	}
}

void wtk_buf_pop(wtk_buf_t *buf,char *data,int len)
{
	wtk_buf_item_t *item=buf->front;
	int n;

	while(len>0)
	{
		n=min(len,item->len);
		len-=n;
		if(data)
		{
			memcpy(data,item->data+item->pos,n);
			data+=n;
		}
		item->pos+=n;
		item->len-=n;
		if(item->len==0)
		{
			if(item->next)
			{
				buf->front=item->next;
				wtk_buf_item_delete(item);
				item=buf->front;
			}else
			{
				item->pos=0;
				item->len=0;
				break;
			}
		}
	}
}

void wtk_buf_read(wtk_buf_t *buf, int skip_len, int *n,
                  void (*read_f)(void *upval, char *d, int len), void *upval) {
    wtk_buf_item_t *item;
    wtk_buf_item_t sitem;
    int step;

    item = buf->front;
    if (item == NULL) {
        return;
    }

    sitem = *item;

    for (; item && skip_len > 0;) {
        sitem = *item;
        step = min(skip_len, item->len);
        skip_len -= step;
        sitem.pos += step;
        sitem.len -= step;
        if (sitem.len == 0) {
            item = item->next;
            if (item == NULL) {
                break;
            } else {
                sitem = *item;
            }
        }
    }

    if (skip_len > 0 || item == NULL) {
        return;
    }

    for (item = &sitem; item && *n > 0;) {
        step = min(*n, item->len);
        read_f(upval, item->data + item->pos, step);
        *n -= step;
        item = item->next;
    }
}
