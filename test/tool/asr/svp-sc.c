#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/asr/kws/qtk_sond_cluster.h"
#include "wtk/core/rbin/wtk_flist.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"
#include "wtk/core/cfg/wtk_source.h"
#include "wtk/core/math/wtk_matrix.h"
#include "wtk/core/math/wtk_math.h"
#include "wtk/asr/kws/wtk_svprint.h"
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

int xxx_chr(char *s,int len,char c)
{
	char *e;
	int xx = 0;
	e=s+len-1;
	while(s<=e)
	{
		if(*s==c)
		{
			return xx;
		}
		s++;
		xx++;
	}
	return -1;
}

// void result_2_rttm(qtk_sond_cluster_t* sond_cluster,wtk_string_t *v){
//     int i,j,flag=0;
//     int st=0,end=0,ll;
//     short *f;
//     float dur = sond_cluster->frame_dur;
// 	ll = xxx_chr(v->data,v->len,'.');
// 	//wtk_debug("%.*s %d\n",v->len,v->data,ll);
//     qtk_sond_cluster_logits_process(sond_cluster);
//     for(i = 0; i < sond_cluster->spk_cnt; i++){
//         f = sond_cluster->filter_feat[i] + 42;
//         for(j = 0; j < sond_cluster->frames; j++,f++){
//             if(!flag && *f == 1){
//                 st = j; 
//                 flag = 1;
//             }else if(flag && (*f == 0 || j == sond_cluster->frames-1)){
//                 end = j;
//                 flag = 0;
//                 if((end - st) > sond_cluster->cfg->seg_length_threshold)
//                 wtk_strbuf_push_f(sond_cluster->result_buf,"SPEAKER %.*s 1 %.2f %.2f <NA> <NA> %.*s <NA> <NA>\n",ll,v->data,st*dur/1000,(end-st)*dur/1000,
// 					sond_cluster->name[i]->len,sond_cluster->name[i]->data);
//             }
//         }
//     }
// 	printf("%.*s",sond_cluster->result_buf->pos,sond_cluster->result_buf->data);
// }

int read_xvec_src(qtk_sond_cluster_t *dec,wtk_source_t *src)
{
    int ret = 0,i;
	wtk_string_t* name;
	wtk_strbuf_t *buf = wtk_strbuf_new(1024,1);
	wtk_vecf_t *vf = wtk_vecf_new(256);
	while(1){
		ret = wtk_source_read_string(src,buf);
		if(ret == -1){break;}
		name = wtk_string_dup_data(buf->data,buf->pos);
		wtk_debug("%.*s\n",name->len,name->data);
		ret = wtk_source_seek_to_s(src,"[");
		for(i = 0;i < 256; i++){
			ret = wtk_source_read_string(src,buf);
			*(vf->p + i) = wtk_str_atof(buf->data,buf->pos);			
		}
		ret = wtk_source_read_string(src,buf);
		wtk_svprint_enrollvec(dec->svprint, name, vf);
		wtk_string_delete(name);
		if(ret == -1){break;}
	}
	wtk_vecf_delete(vf);
	wtk_strbuf_delete(buf);
	ret = 0;
    return ret;
}

void read_xvec(qtk_sond_cluster_t* dec,char* xvec_fn)
{
	wtk_source_loader_t sl;

	sl.hook=0;
    sl.vf=wtk_source_load_file_v;
	wtk_source_loader_load(&sl,dec,(wtk_source_load_handler_t)read_xvec_src,xvec_fn);
}

void audio_energy_normalize(wtk_strbuf_t *wav_buf){
	int i,len = wav_buf->pos/2;
	double *p = (double*)wtk_malloc(len*sizeof(double));
	short *wav = (short*)wav_buf->data;
	double sum = 0.0,wav_db,scale,max = 0.0;

	for(i = 0; i < len ; i++){
		*(p + i) = *(wav + i) * 1.0/32768;
		sum += (*(p + i))*(*(p + i));
	}
	sum = sum/len + 1e-4;
	wav_db = 10 * log10(sum);
	scale = sqrt(pow(10,(1 - wav_db)/10));
	//wtk_debug("%f %f %f\n",sum,wav_db,scale);
	for(i = 0; i < len ; i++){
		*(p + i) *= scale;
		max = max(max,fabsf(*(p + i)));
	}
	//printf("%f %f\n",scale,max);
	for(i = 0; i < len ; i++){
		*(p + i) = *(p + i)/max*0.99;
		*(wav + i) = *(p + i) * 32768;
	}
	wtk_free(p);
}

void test_wav_file2(qtk_sond_cluster_t *dec,char *fn,wtk_rate_t *rate)
{
	double t,t2;
	double wav_t;
	wtk_strbuf_t **buf;
	int n;
	buf=wtk_riff_read_channel(fn,&n);

    //printf("==>fn: %s\n",fn);
	//audio_energy_normalize(buf[0]);

	wav_t=((buf[0]->pos)*1.0/32);//for 16k
	if(rate)
	{
		rate->wave_time+=wav_t;
	}

	t2=time_get_cpu();
	t=time_get_ms();

	qtk_sond_cluster_start(dec);
	{
		char *s,*e;
		int step=4096;
		int nx;
		double nt;

		nt=0;
		s=buf[0]->data;
		e=s+buf[0]->pos;
		while(s<e)
		{
			nx=min(step,e-s);
			nt+=nx/2;
			qtk_sond_cluster_feed(dec,s,nx,0);
			s+=nx;
		}
		qtk_sond_cluster_feed(dec,0,0,1);
//		wtk_debug("want end delay= %f\n",time_get_ms()-tx1);
	}

	t2=time_get_cpu()-t2;
	t=time_get_ms()-t;
	if(rate)
	{
		rate->cpu_time+=t2;
		rate->time+=t;
	}
	//wtk_debug("time=%f,timerate=%f,cpurate=%f\n",t,t/wav_t,t2/wav_t);
	qtk_sond_cluster_reset(dec);
	wtk_strbuf_delete(buf[0]);
	wtk_free(buf);
//	wtk_free(data);
}
int xspk = 0;
void test_wav_file(qtk_sond_cluster_t *dec,char *fn,wtk_rate_t *rate)
{
	double t,t2,t3;
	double wav_t;
	wtk_strbuf_t **buf;
	int n;
	buf=wtk_riff_read_channel(fn,&n);

    //printf("==>fn: %s\n",fn);
	//audio_energy_normalize(buf[0]);

	wav_t=((buf[0]->pos)*1.0/32);//for 16k
	if(rate)
	{
		rate->wave_time+=wav_t;
	}

	wtk_string_t v2;
	wtk_string_t *v = wtk_basename(fn,'/');
	//dec->vv = v;
	//printf("%.*s\n",v->len,v->data);
	int ret;
	if(dec->cfg->use_clu){
		ret = qtk_sond_cluster_set_spk_nums(dec,xspk);
		if(ret!=0){
			return;
		}		
	}else{
		ret = qtk_sond_cluster_prepare(dec);
		if(ret!=0){
			return;
		}
	}

	t2=time_get_cpu();
	t=time_get_ms();
	qtk_sond_cluster_start(dec);
	{
		char *s,*e;
		int step=64*32;
		int nx;
		double nt;

		nt=0;
		s=buf[0]->data;
		e=s+buf[0]->pos;
		while(s<e)
		{
			nx=min(step,e-s);
			nt+=nx/2;
			t3 = time_get_ms();
			
			//wtk_debug("====================>>>>>>>feed time=%f\n",nt/16.0);
			qtk_sond_cluster_feed(dec,s,nx,0);
			//t3 = time_get_ms() - t3;
			//wtk_debug("================>>>>>>>>>>>feed bytes=%d time=%f\n",nx,t3);

			if(nt/(1000*16) > 1){
				//if(nt/(1000*16*11) > 1)
				if(0)
				{
					break;
				}
				qtk_sond_cluster_get_result(dec,&v2);
				//qtk_sond_cluster_get_result(dec,&v);
				if(v2.len > 0)
				{
					wtk_debug("%.*s\n",v2.len,v2.data);				
				}
			}
			s+=nx;
		}
		qtk_sond_cluster_feed(dec,0,0,1);
//		wtk_debug("want end delay= %f\n",time_get_ms()-tx1);
	}

	//result_2_rttm(dec,v);
	qtk_sond_cluster_get_result(dec,&v2);
	//qtk_sond_cluster_get_result(dec,&v);
	printf("%.*s",v2.len,v2.data);
	t2=time_get_cpu()-t2;
	t=time_get_ms()-t;
	if(rate)
	{
		rate->cpu_time+=t2;
		rate->time+=t;
	}
	wtk_debug("time=%f/%f,timerate=%f,cpurate=%f\n",t,wav_t,t/wav_t,t2/wav_t);
	qtk_sond_cluster_reset(dec);
	wtk_strbuf_delete(buf[0]);
	wtk_string_delete(v);
	wtk_free(buf);
//	wtk_free(data);
}

void test_scp(qtk_sond_cluster_t *dec,char *fn)
{
	wtk_flist_t *f;
	wtk_queue_node_t *n;
	wtk_fitem_t *item;
	//test_mul_t* tmul=(test_mul_t*)malloc(sizeof(test_mul_t));
	wtk_rate_t rate;
	int i,len;
	wtk_array_t *a;
	wtk_heap_t *heap = wtk_heap_new(4096);
	wtk_string_t **strs;
	wtk_string_t *prev = NULL;

	f=wtk_flist_new(fn);
	wtk_rate_init(&(rate));

	if(f)
	{
		for(i=0,n=f->queue.pop;n;n=n->next,++i)
		{
			item=data_offset(n,wtk_fitem_t,q_n);
//			printf("%s\n",item->str->data);
			a = wtk_str_to_array(heap, item->str->data, item->str->len, ' ');
			strs = (wtk_string_t**) a->slot;
			wtk_debug("%.*s\n",strs[0]->len,strs[0]->data);
			len = strs[1]->len;
			strs[1]->data[len]='\0';
			if(!prev)
			{
				prev = strs[0];
				qtk_sond_cluster_enroll(dec,strs[0]->data,strs[0]->len);
			}else if(!wtk_str_equal(prev->data,prev->len,strs[0]->data,strs[0]->len))
			{
				qtk_sond_cluster_enroll_end(dec);
				//qtk_sond_cluster_reset(dec);
				prev = strs[0];
				wtk_debug("xxxx\n");
				qtk_sond_cluster_enroll(dec,strs[0]->data,strs[0]->len);
			}
			test_wav_file2(dec,strs[1]->data,&rate);
		}
		qtk_sond_cluster_enroll_end(dec);
		//qtk_sond_cluster_reset(dec);
		printf("xtot: time rate=%.3f\n",rate.time/rate.wave_time);
		wtk_flist_delete(f);
	}

	///* //test clean
	qtk_sond_cluster_clean(dec);
	qtk_sond_cluster_enroll(dec,"aa",2);
	test_wav_file2(dec,"白.wav",&rate);
	qtk_sond_cluster_enroll_end(dec);
	//*/

	wtk_heap_delete(heap);
	//wtk_flist_delete(f);
}

void test_feat(qtk_sond_cluster_t *dec,char *fn)
{
	wtk_string_t *v;

	v = wtk_basename(fn,'/');
	dec->wav = v;
	qtk_sond_cluster_prepare(dec);
	qtk_sond_cluster_start(dec);
	//qtk_sond_cluster_logits_pre_compute(dec, 415, myfeat,2);

	//result_2_rttm(dec,v);
}

int main(int argc,char **argv)
{
	wtk_main_cfg_t *main_cfg=0;
	wtk_mbin_cfg_t *mbin_cfg=0;
	wtk_arg_t *arg;
	qtk_sond_cluster_t *dec=0;
	qtk_sond_cluster_cfg_t *cfg=NULL;
	char *cfg_fn=0;
	char *scp_fn=0;
	char *wav_fn=0;
	char *xvec_fn=0;
	char *vprint_fn=0;
	int bin=0;
	int mode=-1;//mode 0 enroll;mode 1 Voiceprint stream;mode 2 Voiceprint offline
	int n_skps = 0;

	arg=wtk_arg_new(argc,argv);

	wtk_arg_get_str_s(arg,"c",&cfg_fn);
    wtk_arg_get_str_s(arg,"scp",&scp_fn);
	wtk_arg_get_str_s(arg,"wav",&wav_fn);
	wtk_arg_get_str_s(arg,"xvec",&xvec_fn);
	wtk_arg_get_str_s(arg,"vp",&vprint_fn);
	wtk_arg_get_int_s(arg,"b",&bin);
	wtk_arg_get_int_s(arg,"mode",&mode);
    wtk_arg_get_int_s(arg, "ns", &n_skps);

	xspk = n_skps;
//	if(!cfg_fn || (!scp_fn && !wav_fn))
//	{
//		print_usage(argc,argv);
//		goto end;
//	}
    if(cfg_fn)
    {
		
		if(bin){
			mbin_cfg = wtk_mbin_cfg_new_type(qtk_sond_cluster_cfg,cfg_fn,"./cls.cfg");
			cfg = (qtk_sond_cluster_cfg_t*)mbin_cfg->cfg;
			dec=qtk_sond_cluster_new(cfg);
			if(!dec)
			{
				wtk_debug("create sond cluster from bin failed.\n");
				goto end;
			}
		}else{
			main_cfg=wtk_main_cfg_new_type(qtk_sond_cluster_cfg,cfg_fn);
			if(!main_cfg)
			{
				wtk_debug("load configure failed.\n");
				goto end;
			}
			cfg=(qtk_sond_cluster_cfg_t*)main_cfg->cfg;

			dec=qtk_sond_cluster_new(cfg);
			if(!dec)
			{
				wtk_debug("create sond cluster failed.\n");
				goto end;
			}
		}

    }

	if(vprint_fn){
		qtk_sond_cluster_set_enroll_fn(dec,vprint_fn,sizeof(vprint_fn));
		qtk_sond_cluster_prepare(dec);
		qtk_sond_cluster_set_enroll_fn(dec,vprint_fn,sizeof(vprint_fn));
	}

	if(xvec_fn)
	{
		read_xvec(dec,xvec_fn);
	}

	if(wav_fn && mode == 1)
	{
		test_wav_file(dec,wav_fn,NULL);
	}

	if(wav_fn && mode == 2)
	{
		test_feat(dec,wav_fn);
	}

	if(scp_fn && mode == 0)
	{
		test_scp(dec,scp_fn);
		if(wav_fn)
		{
			test_wav_file(dec,wav_fn,NULL);
		}
	}

end:

	if(dec)
	{
		qtk_sond_cluster_delete(dec);
	}
	if(main_cfg)
	{
		wtk_main_cfg_delete(main_cfg);
	}
	if(mbin_cfg)
	{
		wtk_mbin_cfg_delete(mbin_cfg);
	}
	wtk_arg_delete(arg);

	return 0;
}
