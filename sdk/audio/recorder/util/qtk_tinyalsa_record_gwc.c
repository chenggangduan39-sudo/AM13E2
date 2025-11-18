#include "qtk_tinyalsa_record_gwc.h"

short** qtk_trg_short_new_p2(int n1,int n2)
{
	short **pv;
	int i;

	pv=(short**)wtk_calloc(n1,sizeof(short*));
	for(i=0;i<n1;++i)
	{
		pv[i]=(short*)wtk_calloc(n2,sizeof(short));
		wtk_debug("pv[%d] = %p\n",i,pv[i]);
	}
	return pv;
}

void qtk_trg_short_delete_p2(short **pv,int n1)
{
	int i;

	for(i=0;i<n1;++i)
	{
		wtk_debug("pv[%d] = %p\n",i,pv[i]);
		wtk_free(pv[i]);
	}
	wtk_free(pv);
}

qtk_tinyalsa_record_gwc_t* qtk_tinyalsa_record_gwc_new()
{
	qtk_tinyalsa_record_gwc_t *trg;

	trg=(qtk_tinyalsa_record_gwc_t*)wtk_malloc(sizeof(qtk_tinyalsa_record_gwc_t));
	trg->c=12;
	trg->oc=5;
	trg->sample=trg->c*4;
	trg->buf_size=16*100;
	trg->first_flag = 1;
	trg->ths=NULL;
	trg->notify=NULL;
	trg->buf=wtk_strbuf_new(1024,1);
	trg->pv=qtk_trg_short_new_p2(trg->oc,trg->buf_size);
	qtk_tinyalsa_record_gwc_reset(trg);
	return trg;
}

void qtk_tinyalsa_record_gwc_delete(qtk_tinyalsa_record_gwc_t *trg)
{
	qtk_trg_short_delete_p2(trg->pv,trg->oc);
	wtk_strbuf_delete(trg->buf);
	wtk_free(trg);
}

void qtk_tinyalsa_record_gwc_start(qtk_tinyalsa_record_gwc_t *trg)
{
}

void qtk_tinyalsa_record_gwc_reset(qtk_tinyalsa_record_gwc_t *trg)
{
	trg->state=QTK_TINYALSA_RECORD_GWC_ALIGN;
	wtk_strbuf_reset(trg->buf);
}

void qtk_tinyalsa_record_gwc_set_notify(qtk_tinyalsa_record_gwc_t *trg,void *ths,qtk_tinyalsa_record_gwc_notify_f notify)
{
	trg->ths=ths;
	trg->notify=notify;
}

void qtk_trg_process(qtk_tinyalsa_record_gwc_t *trg)
{
	wtk_strbuf_t *buf=trg->buf;
	char *ps,*pe;
	short *sv;
	int n,i,j;
	int v;
	short **pv;
	short *pv1,*pv2,*pv3,*pv4,*pv5;
	int cx;

	switch(trg->state)
	{
	case QTK_TINYALSA_RECORD_GWC_ALIGN:
		ps=buf->data;
		pe=ps+buf->pos;
		n=0;
		if(trg->first_flag == 1){
			while((pe-ps)>=4)
			{
				//low high
				//00  11 22 33
				//8bit not-use
				//8bit low-4bit is channle high 4bit is audio data
				//8bit audio data
				//8bit audio data
				v=ps[1]&0x0f;
//				wtk_debug("====================>v  =%d\n",v);
				//发现第一个通道
				if(v==1)
				{
					//wtk_debug("======= n=%d/%ld ========\n",n,ps-buf->data);
					if(n>0)
					{
						wtk_strbuf_pop(buf,NULL,n*4);
					}
					//exit(0);
					trg->state=QTK_TINYALSA_RECORD_GWC_READY;
					//wtk_debug("ns=%d\n",buf->pos/(12*4));
					qtk_trg_process(trg);
					break;
				}
				ps+=4;
				++n;
			}
		}else{
			trg->state=QTK_TINYALSA_RECORD_GWC_READY;
			qtk_trg_process(trg);
		}

		break;
	case QTK_TINYALSA_RECORD_GWC_READY:
		if(buf->pos<trg->sample){return;}
		n=buf->pos/trg->sample;
		pv=trg->pv;
		sv=(short*)(buf->data);
		pv1=pv[0];pv2=pv[1];pv3=pv[2];pv4=pv[3],pv5=pv[4];
		cx=trg->c*2;
		if(trg->first_flag == 1){
			for(i=0,j=0;i<n;++i)
			{
//				wtk_debug("i=%d/%d %d/%d\n",i,n,n-i,hp->sample);
				//  0           2                                                          1        4
				//0x00112233 0x00112233 0x00112233  0x00112233 0x00112233 0x00112233 0x00112233 0x00112233 0x00112233
				pv1[j]=sv[1]; //firist channel
				pv2[j]=sv[13];	//second channel
				pv3[j]=sv[3];
				pv4[j]=sv[15];
				pv5[j]=sv[7]+sv[19];
				sv+=cx;
				if(j>=(trg->buf_size-1))
				{
					trg->notify(trg->ths,pv,trg->buf_size);
					j=0;
				}else
				{
					++j;
				}
			}
			trg->first_flag = 0;
		}else{
			for(i=0,j=0;i<n;++i)
			{
//				wtk_debug("i=%d/%d %d/%d\n",i,n,n-i,hp->sample);
				//  0           2                                                          1        4
				//0x00112233 0x00112233 0x00112233  0x00112233 0x00112233 0x00112233 0x00112233 0x00112233 0x00112233
				pv1[j]=sv[21]; //firist channel
				pv2[j]=sv[9];	//second channel
				pv3[j]=sv[23];
				pv4[j]=sv[11];
				pv5[j]=sv[3]+sv[15];
				sv+=cx;
				if(j>=(trg->buf_size-1))
				{
					trg->notify(trg->ths,pv,trg->buf_size);
					j=0;
				}else
				{
					++j;
				}
			}
		}

		if(j>0)
		{
			trg->notify(trg->ths,pv,j);
		}
		wtk_strbuf_pop(buf,NULL,n*trg->sample);
		//wtk_debug("pos=%d\n",buf->pos);
		break;
	}
}


void qtk_tinyalsa_record_gwc_feed(qtk_tinyalsa_record_gwc_t *trg,char *data,int len)
{
	if(len<=0){return;}
	wtk_strbuf_push(trg->buf,data,len);
	qtk_trg_process(trg);
}
