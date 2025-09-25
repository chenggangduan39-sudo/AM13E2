/*
 * wtk_mtts.c
 *
 *  Created on: Dec 17, 2016
 *      Author: dm
 */

#include "wtk_mtts.h"
#include "wtk/core/wtk_strbuf.h"
#include <ctype.h>

//int wtk_mtts_process_sole(wtk_mtts_t* tts, wtk_tts_snt_t* s);
int wtk_mtts_pitch_sole(wtk_mtts_t* mtts, wtk_tts_snt_t* snt);
wtk_mtts_t *wtk_mtts_new(wtk_mtts_cfg_t *cfg)
{
	wtk_mtts_t *mtts;

	mtts=(wtk_mtts_t*) wtk_calloc(sizeof(*mtts), 1);
	mtts->cfg=cfg;
	mtts->cn=wtk_tts_new(cfg->cncfg);
	mtts->pitch=mtts->cn->pitch;
	mtts->svol=cfg->svol;
	mtts->sshift=cfg->sshift;

	return mtts;
}

int wtk_mtts_process(wtk_mtts_t *mtts,char *txt,int txt_bytes)
{
	wtk_tts_t *cn;
	wtk_tts_snt_t **snt,*s;
	wtk_tts_lab_t* lab;
	wtk_tts_msg_t msg;
	int ret;
	int i;
	wtk_strbuf_t *buf=wtk_strbuf_new(256,1);

	wtk_strbuf_push(buf,txt,txt_bytes);
	wtk_strbuf_strip(buf);
	txt=buf->data;
	txt_bytes=buf->pos;

	cn=mtts->cn;
	if (cn){
		cn->stop_hint=0;
		lab=wtk_tts_parser_to_sntm(cn->parser,txt,txt_bytes, 1);
		if(!lab){goto end;}
		snt=(wtk_tts_snt_t**)lab->snts->slot;
		for(i=0;i<lab->snts->nslot && cn->stop_hint==0 ;++i)
		{
			//wtk_debug("v[%d]=[%.*s]\n",i,snt[i]->snt->len,snt[i]->snt->data);
			if (cn->pause_hint){
				wtk_sem_acquire(&(cn->pause_sem),-1);
			}
			s=snt[i];
			if (s->is_ctx_pick){
				wtk_mtts_pitch_sole(mtts, s);
			}else{
				wtk_mtts_set_minsil_time(mtts, mtts->cn->cfg->min_sil_time);
				if ((i<lab->snts->nslot-1 && snt[i+1]->is_ctx_pick) || (i>0 && snt[i-1]->is_ctx_pick)){
					wtk_mtts_set_minsil_time(mtts, mtts->cfg->mix_sil_time);
				}

				ret=wtk_tts_parser_process_snt(cn->parser,s);
				if(ret!=0){continue;}
				if(cn->stop_hint)
				{
					break;
				}
				//tts->parser->lab->speech_speed=tts->speed;
				ret=wtk_syn_process_snt(cn->syn,s,cn->speed);
				if(ret!=0){continue;}
			}
		}
		if(cn->cfg->use_thread && cn->pitch_shit!=0)
		{
			msg.type=WTK_TTS_MSG_STOP;
			wtk_blockqueue_push(&(cn->msg_q),&(msg.q_n));
			wtk_sem_acquire(&(cn->wait_sem),-1);
		}
	}


	ret=0;
end:
	wtk_mtts_notify(mtts,NULL,0);
	wtk_strbuf_delete(buf);
	return ret;
}

#include "wtk/core/wtk_riff.h"
#include "wtk/core/wavehdr.h"

int wtk_mtts_convert(wtk_mtts_t *mtts, char *in,char *out)
{
	wtk_pitch_t *p=mtts->pitch;
#define BUF_SIZE 1024
	char buf[BUF_SIZE];
	wtk_riff_t *riff;
	int ret,i,len;
	float shift=mtts->sshift;
	float vol_scale=mtts->svol;
	short *pdata;
	wtk_tts_msg_t *msg;

	wtk_pitch_reset(p);
	//wtk_debug("rover=%d\n",p->rover);
	riff=wtk_riff_new();
	ret=wtk_riff_open(riff,in);
	if (ret < 0){
		ret=-1;goto end;
	}
	while(1)
	{
		ret=wtk_riff_read(riff,buf,BUF_SIZE);
		//wtk_debug("ret=%d\n",ret);
		if (ret > 0){
			len=ret>>1;
			pdata=(short*)buf;
			for(i=0;i<len;++i)
			{
				pdata[i]=(short)pdata[i]*vol_scale;
			}

			if(shift==0)
			{
				wtk_tts_notify(mtts->cn, buf, ret);
			}else
			{
				if(mtts->cn->cfg->use_thread)
				{
					msg=wtk_tts_msg_new(buf,ret);
					wtk_blockqueue_push(&(mtts->cn->msg_q),&(msg->q_n));
				}else
				{
					wtk_pitch_process(mtts->pitch,mtts->sshift,buf, ret);
				}
			}
		}

		if(ret<BUF_SIZE)
		{
			break;
		}
	}
	if (out){
		wave_write_file(out,riff->fmt.sample_rate,p->buf->data,p->buf->pos);
		//wtk_debug("write %s [shift=%f, rate=%d]\n",out,shift,riff->fmt.sample_rate);
	}
	wtk_riff_close(riff);
	wtk_riff_delete(riff);

	ret=0;
end:
	return ret;
}

int wtk_mtts_pitch_sole(wtk_mtts_t* mtts, wtk_tts_snt_t* snt)
{
	int i;
	wtk_strbuf_t *buf;
	wtk_string_t *v;

	//wtk_array_print_string(wrds);
	buf = wtk_strbuf_new(128,1);
	v = snt->snt;
	wtk_strbuf_reset(buf);
	wtk_strbuf_push(buf, mtts->cfg->swav,strlen(mtts->cfg->swav));
	wtk_strbuf_push_c(buf, '/');
	for(i=0; i < v->len; i++){
		wtk_strbuf_push_c(buf, tolower(v->data[i]));
	}
	wtk_strbuf_push_s(buf, ".wav");
	wtk_strbuf_push_c(buf, 0);
	wtk_mtts_convert(mtts, buf->data, NULL);
	wtk_strbuf_delete(buf);

	return 0;
}

//int wtk_mtts_process_sole(wtk_mtts_t* mtts, wtk_tts_snt_t* s)
//{
//	wtk_tts_parser_t *p=mtts->cn->parser;
//	wtk_array_t *a;
//	wtk_string_t k;
//	wtk_string_t *v;
//	char *ts,*te;
//	int i,state=0;
//	int ret=-1;
//
//	a=wtk_array_new_h(p->heap,s->snt->len/2,sizeof(void*));
//	ts=s->snt->data;
//	te=ts+s->snt->len;
//	k.len=0;
//	k.data=NULL;
//	while(ts<te)
//	{
//		switch(state)
//		{
//		case 0:
//			if(!isspace(*ts))
//			{
//				k.data=ts;
//				state=1;
//			}
//			break;
//		case 1:
//			if(isspace(*ts))
//			{
//				k.len=ts-k.data;
//				//wtk_debug("[%.*s]\n",k.len,k.data);
//				v=wtk_heap_dup_string(p->heap,k.data,k.len);
//				wtk_array_push2(a,&v);
//				k.data=NULL;
//				state=0;
//			}
//			break;
//		}
//		++ts;
//	}
//	if(k.data)
//	{
//		k.len=ts-k.data;
//		//wtk_debug("[%.*s]\n",k.len,k.data);
//		v=wtk_heap_dup_string(p->heap,k.data,k.len);
//		wtk_array_push2(a,&v);
//	}
////	s->wrds=a;
////	s->syls=NULL;
//	wtk_mtts_pitch_sole(mtts, a);
////	wtk_array_print_string(s->wrds);
//end:
//	return ret;
//}

int wtk_mtts_reset(wtk_mtts_t *mtts)
{
	if (mtts->cn){
		wtk_tts_reset(mtts->cn);
	}

	return 0;
}
void wtk_mtts_delete(wtk_mtts_t *mtts)
{
	if (mtts->cn){
		wtk_tts_delete(mtts->cn);
	}
	wtk_free(mtts);
}

void wtk_mtts_set_notify(wtk_mtts_t *mtts,void *ths,wtk_tts_notify_f notify)
{
	wtk_tts_set_notify(mtts->cn, ths, notify);
}

void wtk_mtts_set_volume_scale(wtk_mtts_t *mtts, float scale)
{
	wtk_tts_set_volume_scale(mtts->cn, scale);
	mtts->svol = scale;
}
void wtk_mtts_set_stop_hint(wtk_mtts_t *mtts)
{
	wtk_tts_set_stop_hint(mtts->cn);
}
void wtk_mtts_set_speed(wtk_mtts_t *mtts,float speed)
{
	wtk_tts_set_speed(mtts->cn, speed);
}

void wtk_mtts_set_pitch(wtk_mtts_t *mtts, float pitch)
{
	wtk_tts_set_pitch(mtts->cn, pitch);
	mtts->sshift = mtts->cfg->r_shift *pitch;
}

void wtk_mtts_notify(wtk_mtts_t *mtts,char *data,int len)
{
	wtk_tts_notify(mtts->cn, data, len);
}

void wtk_mtts_set_minsil_time(wtk_mtts_t* mtts, int time)
{
	mtts->cn->min_sil_time = time;
}

