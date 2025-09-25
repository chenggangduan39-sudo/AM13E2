#include "wtk_short_buffer.h"
#include "wtk/core/wtk_alloc.h"

wtk_short_buffer_t* wtk_short_buffer_new(int size)
{
	wtk_short_buffer_t *b;
	int t;

	t=wtk_round_word(sizeof(*b))+size*sizeof(short);
	b=(wtk_short_buffer_t*)wtk_malloc(t);
	b->odd=0;
	b->rstart=b->cur=b->start=(short*)((char*)b+wtk_round_word(sizeof(*b)));
	b->end=b->rstart+size;
	return b;
}

void wtk_short_buffer_reset(wtk_short_buffer_t *b)
{
	b->odd=0;
	b->rstart=b->cur=b->start=(short*)((char*)b+wtk_round_word(sizeof(*b)));
}

int wtk_short_buffer_delete(wtk_short_buffer_t *b)
{
	wtk_free(b);
	return 0;
}

int wtk_short_buffer_push(wtk_short_buffer_t *b,short *data,int samples)
{
	int cpy;
	int left;

	left=wtk_short_buffer_unuse_samples(b);
	cpy=min(left,samples);
	if(cpy>0)
	{
		memcpy(b->cur,data,cpy*sizeof(short));
		b->cur+=cpy;
	}
	return cpy;
}

int wtk_short_buffer_push_c(wtk_short_buffer_t *b,char *data,int bytes)
{
	short odd;
	char *p;
	int cpy=0,left,samples;

	left=wtk_short_buffer_unuse_samples(b);
	if(left<=0 || bytes<=0){goto end;}
	if(b->odd)
	{
		p=(char*)&odd;
		p[0]=b->odd_char;
		p[1]=data[0];
		bytes-=1;data+=1;
		b->odd=0;
		wtk_short_buffer_push(b,&odd,1);
		cpy+=1;
	}
	samples=bytes/2;
	left=wtk_short_buffer_push(b,(short*)data,samples);
	cpy+=left<<1;
	if((left>=samples) && (bytes%2))
	{
		b->odd_char=data[bytes-1];
		b->odd=1;
		cpy+=1;
	}
end:
	return cpy;
}

int wtk_short_buffer_peek(wtk_short_buffer_t *b,wtk_vframe_t *f)
{
	short *start;
	int ret,used;
	int i;

	used=wtk_short_buffer_used_samples(b);
	if(used<f->frame_size){ret=-1;goto end;}
	start=b->start;
	if(f->sample_data)
	{
		for(i=0;i<f->frame_size;++i)
		{
			f->sample_data[i]=start[i];
		}
	}
	//f->sample_data=start;
	memcpy(f->wav_data,start,sizeof(short)*f->frame_step);
	ret=0;
end:
	return ret;
}

void wtk_short_buffer_skip(wtk_short_buffer_t *b,int samples,int left_enough)
{
	int size;

	b->start+=samples;
	if((b->end-b->start)<left_enough)
	{
		size=b->cur - b->start;
		memmove(b->rstart,b->start,size*sizeof(short));
		b->start=b->rstart;
		b->cur=b->start+size;
	}
}

