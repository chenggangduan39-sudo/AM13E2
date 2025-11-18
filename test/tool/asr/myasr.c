#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/asr/wfst/qtk_asr_wrapper_cfg.h"
#include "wtk/asr/wfst/qtk_asr_wrapper.h"
#include "wtk/core/rbin/wtk_flist.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"
#include "wtk/core/cfg/wtk_source.h"
#include "wtk/core/math/wtk_matrix.h"
#include "wtk/core/math/wtk_math.h"
#include "wtk/os/wtk_pid.h"
#include "wtk/asr/vad/wtk_vad.h"

typedef struct wtk_rate
{
	double wave_time;
	double cpu_time;
	double time;
	double rescore_time;
	double rec_mem;
	double rescore_mem;
}wtk_rate_t;


void wtk_rate_init(wtk_rate_t *r)
{
	r->wave_time=0;
	r->cpu_time=0;
	r->time=0;
	r->rescore_time=0;
	r->rec_mem=0;
	r->rescore_mem=0;
}

wtk_queue_t vqueue;

void test_vad_file(qtk_k2_dec_t *dec,wtk_vad_t *vad,char *fn,wtk_rate_t *rate){
    wtk_strbuf_t **buf;
	wtk_vframe_t *f;
	int state=wtk_vframe_sil;
	int n;

    buf=wtk_riff_read_channel(fn,&n);
	wtk_vad_start(vad);
	wtk_vad_feed(vad,buf[0]->data,buf[0]->pos,0);
	wtk_vad_feed(vad,0,0,1);
	wtk_queue_node_t *qn;
	int st=0,end=0,i=0;
	//wtk_wavfile_t *lwav = wtk_wavfile_new(16000);
	while(1){
		qn = wtk_queue_pop(&vqueue);
		if(!qn){
			break;
		}
		f = data_offset(qn,wtk_vframe_t,q_n);
		switch (state)
		{
		case wtk_vframe_sil:
			if(f->state != wtk_vframe_sil){
				st = i;
				qtk_asr_wrapper_set_vadindex(dec,f->index);
				qtk_asr_wrapper_start(dec);
				//wtk_wavfile_open2(lwav,"logwav/xx");
				//wtk_wavfile_write(lwav,(char*)f->wav_data,sizeof(short)*vad->cfg->dnnvad.parm.frame_step);
				wtk_debug("==============>>>>>>>>>>vad start\n");
				qtk_asr_wrapper_feed(dec,(char*)f->wav_data,sizeof(short)*vad->cfg->dnnvad.parm.frame_step,0);
				state=wtk_vframe_speech;
			}
			break;
		case wtk_vframe_speech:
			if(f->state != wtk_vframe_speech){
				end = i;
				state = wtk_vframe_sil;
				//wtk_wavfile_close(lwav);
				//wtk_debug("%d %d ",st,end);
				wtk_debug("==============>>>>>>>>>>vad end\n");
				qtk_asr_wrapper_feed(dec,0,0,1);
				wtk_string_t v;
				qtk_asr_wrapper_get_result(dec,&v);
				if(v.len > 0){
					printf("%.*s\n",v.len,v.data);
				}
				qtk_asr_wrapper_reset(dec);
			}else{
				//wtk_wavfile_write(lwav,(char*)f->wav_data,sizeof(short)*vad->cfg->dnnvad.parm.frame_step);
				qtk_asr_wrapper_feed(dec,(char*)f->wav_data,sizeof(short)*vad->cfg->dnnvad.parm.frame_step,0);	
			}
			break;
		case wtk_vframe_speech_end:
			break;		
		default:
			break;
		}
		++i;
	}

}

void test_wav_file(qtk_asr_wrapper_t *dec,char *fn,wtk_rate_t *rate)
{
    wtk_riff_t *riff = NULL;
    char buf[1024];
    int read_cnt;
    int is_end = 0;

    riff = wtk_riff_new();
    wtk_riff_open(riff, fn);
    wtk_string_t v;
    printf("%s ",fn);
	qtk_asr_wrapper_start(dec);
	{
        while (is_end == 0) {
            read_cnt = wtk_riff_read(riff, buf, sizeof(buf));
//            qtk_asr_wrapper_get_hint_result(dec,&v);
//            printf("%.*s\n",v.len,v.data);
            if (read_cnt < sizeof(buf)) {
                is_end = 1;
            }
            qtk_asr_wrapper_feed(dec, buf, read_cnt,0);
        }
		qtk_asr_wrapper_feed(dec,0,0,1);
	}

	qtk_asr_wrapper_get_result(dec,&v);
	printf("%.*s\n",v.len,v.data);
	qtk_asr_wrapper_reset(dec);
	wtk_riff_delete(riff);
}

typedef struct test_mul test_mul_t;
struct test_mul
{
	wtk_matrix_t *m;
	int row;
	int col;
};

int read_matrix(test_mul_t *tmul,wtk_source_t *src)
{
	int ret;
	int cnt;
	//int v;
	wtk_matrix_t *m=0;
	//wtk_strbuf_t *buf;

	//buf=wtk_strbuf_new(256,1);
	ret=wtk_source_read_int(src,&cnt,1,0);
	tmul->row=cnt;

	m=wtk_matrix_new2(cnt,3045);
//	m=wtk_matrix_new2(cnt,5373);

	ret=wtk_source_read_matrix(src,m,0);
//	wtk_matrix_print(m);
	tmul->col=3045;
//	tmul->col=5373;
	tmul->m=m;

	return ret;
}

void test_scp(qtk_asr_wrapper_t *dec,char *fn)
{
	wtk_flist_t *f;
	wtk_queue_node_t *n;
	wtk_fitem_t *item;
	//test_mul_t* tmul=(test_mul_t*)malloc(sizeof(test_mul_t));
	wtk_rate_t rate;
	int i;

	f=wtk_flist_new(fn);
	wtk_rate_init(&(rate));

	if(f)
	{
		for(i=0,n=f->queue.pop;n;n=n->next,++i)
		{
			item=data_offset(n,wtk_fitem_t,q_n);
//			printf("%s\n",item->str->data);
			test_wav_file(dec,item->str->data,&rate);
		}
//		printf("xtot: time rate=%.3f\n",rate.time/rate.wave_time);
		wtk_flist_delete(f);
	}
	//wtk_flist_delete(f);
}


int main(int argc,char **argv)
{
	wtk_main_cfg_t *main_cfg=0;
	wtk_main_cfg_t *main_cfg2=0;		
	wtk_arg_t *arg;
	qtk_asr_wrapper_t *dec=0;
	qtk_asr_wrapper_cfg_t *cfg=NULL;
	wtk_vad_cfg_t *vcfg=0;
    wtk_vad_t *vad=0;
	char *vcfg_fn=0;	
	char *cfg_fn=0;
	char *scp_fn=0;
	char *wav_fn=0;
	char *feat_fn=0;
	char *f_fn=0;
	char *h_fn=0;
	int bin=0;
	arg=wtk_arg_new(argc,argv);

	wtk_arg_get_str_s(arg,"v",&vcfg_fn);	
	wtk_arg_get_str_s(arg,"hw",&h_fn);
	wtk_arg_get_str_s(arg,"c",&cfg_fn);
	wtk_arg_get_str_s(arg,"scp",&scp_fn);
	wtk_arg_get_str_s(arg,"wav",&wav_fn);
	wtk_arg_get_str_s(arg,"featscp",&feat_fn);
	wtk_arg_get_str_s(arg,"f",&f_fn);
	wtk_arg_get_int_s(arg,"b",&bin);

    //wtk_debug("======== rec is ready =============\n");
    if(cfg_fn)
    {
		if(bin)
		{
			cfg=qtk_asr_wrapper_cfg_new_bin(cfg_fn);
		}else
		{
			main_cfg=wtk_main_cfg_new_type(qtk_asr_wrapper_cfg,cfg_fn);
			if(!main_cfg)
			{
				wtk_debug("load configure failed.\n");
				goto end;
			}
			cfg=(qtk_asr_wrapper_cfg_t*)main_cfg->cfg;
		}
		dec=qtk_asr_wrapper_new(cfg);
		if(!dec)
		{
			wtk_debug("create k2 failed.\n");
			goto end;
		}
    }

    char *data;
    int len;
#ifndef USE_KSD_EBNF
    if(h_fn){
		data = file_read_buf(h_fn,&len);
		qtk_asr_wrapper_set_context(dec,data,len);
	}
#endif

    if(vcfg_fn)
    {

		main_cfg2=wtk_main_cfg_new_type(wtk_vad_cfg,vcfg_fn);
		if(!main_cfg2)
		{
			wtk_debug("load configure failed.\n");
			goto end;
		}
		vcfg=(wtk_vad_cfg_t*)main_cfg2->cfg;
		wtk_queue_init(&vqueue);
		vad=wtk_vad_new(vcfg,&vqueue);
		if(!vad)
		{
			wtk_debug("create decoder failed.\n");
			goto end;
		}
    }

	if(vad){
		test_vad_file(dec,vad,wav_fn,NULL);
	}else if(wav_fn)
	{
		test_wav_file(dec,wav_fn,NULL);
	}
	if(scp_fn)
	{
		test_scp(dec,scp_fn);
	}

end:

	if(dec)
	{
		qtk_asr_wrapper_delete(dec);
	}
	if(main_cfg)
	{
		wtk_main_cfg_delete(main_cfg);
	}else
	{
		if(cfg)
		{
			qtk_asr_wrapper_cfg_delete_bin(cfg);
		}
	}
	wtk_arg_delete(arg);

	return 0;
}


