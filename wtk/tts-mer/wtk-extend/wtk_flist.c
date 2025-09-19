#include "wtk_flist.h"
void wtk_flist_feed(wtk_flist_t *fl,char *data,int len);
void wtk_flist_process3(char *fn,void *ths,wtk_flist_notify_f2 notify,int cnt);

wtk_flist_t* wtk_mer_flist_new(char *fn)
{
	wtk_flist_t *fl=0;
	char *data;
	int len;

	int unfile = (access(fn, 0) == -1);
	if(unfile){data=fn;len=strlen(data);}
	else data=file_read_buf(fn, &len);
	if(!data){printf("-i file not found\n");goto end;}
	fl=(wtk_flist_t*)wtk_malloc(sizeof(*fl));
	fl->heap=wtk_heap_new(4096);
	fl->buf=wtk_strbuf_new(1024,1);
	wtk_queue_init(&(fl->queue));
	wtk_flist_feed(fl,data, len);
end:
	if(data && !unfile){free(data);}
	return fl;
}

void wtk_mer_flist_process3(char *fn,void *ths,wtk_flist_notify_f2 notify,int cnt)
{
	wtk_flist_t *fl;
	wtk_queue_node_t *qn;
	wtk_fitem_t *item;
	int i;
	int step=0;
	
	fl=wtk_mer_flist_new(fn);
	if(!fl)return;
	for(i=0,qn=fl->queue.pop;qn;qn=qn->next,++i)
	{
		if(i<cnt)
		{
			continue;
		}
		++step;
		item=data_offset2(qn,wtk_fitem_t,q_n);
		notify(ths,item->str->data,i,fl->queue.length);
	}
	if(step!=0)
	{
		wtk_flist_process3(fn,ths,notify,fl->queue.length);
	}
	wtk_flist_delete(fl);
}


void wtk_mer_flist_process2(char *fn,void *ths,wtk_flist_notify_f2 notify)
{
	wtk_mer_flist_process3(fn,ths,notify,0);
}