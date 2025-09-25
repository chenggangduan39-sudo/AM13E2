#include "wtk_rtjoin.h"

wtk_rtjoin_t* wtk_rtjoin_new(wtk_rtjoin_cfg_t *cfg)
{
	wtk_rtjoin_t *rtjoin;

	rtjoin=(wtk_rtjoin_t *)wtk_malloc(sizeof(wtk_rtjoin_t));
	rtjoin->cfg=cfg;
	rtjoin->ths=NULL;
	rtjoin->notify=NULL;

	rtjoin->mic=wtk_strbufs_new(rtjoin->cfg->channel);

	rtjoin->nbin=cfg->wins/2+1;
	rtjoin->window=wtk_malloc(sizeof(float)*cfg->wins);///2);
	rtjoin->synthesis_window=wtk_malloc(sizeof(float)*cfg->wins);///2);
	rtjoin->analysis_mem=wtk_float_new_p2(cfg->channel, rtjoin->nbin-1);
	rtjoin->synthesis_mem=wtk_malloc(sizeof(float)*(rtjoin->nbin-1));
	rtjoin->rfft=wtk_drft_new(cfg->wins);
	rtjoin->rfft_in=(float*)wtk_malloc(sizeof(float)*(cfg->wins));

	rtjoin->fft=wtk_complex_new_p2(cfg->channel, rtjoin->nbin);

    rtjoin->W=wtk_complex_new_p2(cfg->channel-1, rtjoin->nbin*(cfg->M2+cfg->M));
    rtjoin->X=wtk_complex_new_p2(cfg->channel,rtjoin->nbin*(cfg->M2+cfg->M));
    rtjoin->Y=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*rtjoin->nbin);
	rtjoin->E=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*rtjoin->nbin);
	rtjoin->power_x=wtk_float_new_p2(cfg->channel-1, rtjoin->nbin);

	rtjoin->fftx=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*rtjoin->nbin);

	rtjoin->nchenr=NULL;
	rtjoin->fring=NULL;
	if(cfg->use_choicech)
	{
		rtjoin->nchenr=(float *)wtk_malloc(sizeof(float)*cfg->channel);
		rtjoin->nchenr2=(float *)wtk_malloc(sizeof(float)*cfg->channel);
		rtjoin->nchenr3=(float *)wtk_malloc(sizeof(float)*cfg->channel);
		rtjoin->nlms_idx=(int *)wtk_malloc(sizeof(int)*cfg->channel);
		rtjoin->nlms_weight=(float *)wtk_malloc(sizeof(float)*cfg->channel);
		rtjoin->fring=(wtk_fring_t **)wtk_malloc(sizeof(wtk_fring_t *)*cfg->channel);
		for(int i=0;i<cfg->channel;i++)
		{
			rtjoin->fring[i]=wtk_fring_new(cfg->M+cfg->M2);
		}
	}

	rtjoin->eq=NULL;
	if(cfg->use_eq)
	{
		rtjoin->eq=wtk_equalizer_new(&(cfg->eq));
	}
	
	rtjoin->out=wtk_malloc(sizeof(float)*(rtjoin->nbin-1));

	rtjoin->change_frame=NULL;
	if(cfg->change_frame_len>0){
		rtjoin->change_frame=(int *)wtk_malloc(sizeof(int)*cfg->change_frame_len);
	}

	wtk_rtjoin_reset(rtjoin);

	return rtjoin;
}

void wtk_rtjoin_delete(wtk_rtjoin_t *rtjoin)
{
	wtk_strbufs_delete(rtjoin->mic,rtjoin->cfg->channel);

	wtk_free(rtjoin->window);
	wtk_free(rtjoin->synthesis_window);
	wtk_float_delete_p2(rtjoin->analysis_mem, rtjoin->cfg->channel);
	wtk_free(rtjoin->synthesis_mem);
	wtk_free(rtjoin->rfft_in);
	wtk_drft_delete(rtjoin->rfft);
	wtk_complex_delete_p2(rtjoin->fft, rtjoin->cfg->channel);
	if(rtjoin->eq)
	{
		wtk_equalizer_delete(rtjoin->eq);
	}

	wtk_complex_delete_p2(rtjoin->W, rtjoin->cfg->channel-1);
	wtk_complex_delete_p2(rtjoin->X, rtjoin->cfg->channel);
	wtk_free(rtjoin->Y);
	wtk_free(rtjoin->E);
	wtk_float_delete_p2(rtjoin->power_x, rtjoin->cfg->channel-1);

	if(rtjoin->nchenr)
	{
		wtk_free(rtjoin->nchenr);
		wtk_free(rtjoin->nchenr2);
		wtk_free(rtjoin->nchenr3);
		wtk_free(rtjoin->nlms_idx);
		wtk_free(rtjoin->nlms_weight);
		for(int i=0;i<rtjoin->cfg->channel;i++){
			wtk_fring_delete(rtjoin->fring[i]);
		}
		wtk_free(rtjoin->fring);
	}

	wtk_free(rtjoin->fftx);
	wtk_free(rtjoin->out);
	if(rtjoin->change_frame){
		wtk_free(rtjoin->change_frame);
	}
	wtk_free(rtjoin);
}


void wtk_rtjoin_start(wtk_rtjoin_t *rtjoin)
{
}

void wtk_rtjoin_reset(wtk_rtjoin_t *rtjoin)
{
	int wins=rtjoin->cfg->wins;
	int i;

	wtk_strbufs_reset(rtjoin->mic,rtjoin->cfg->channel);
	for (i=0;i<wins;++i)
	{
		rtjoin->window[i] = sin((0.5+i)*PI/(wins));
	}
	wtk_drft_init_synthesis_window(rtjoin->synthesis_window, rtjoin->window, wins);

	wtk_float_zero_p2(rtjoin->analysis_mem, rtjoin->cfg->channel, (rtjoin->nbin-1));
	memset(rtjoin->synthesis_mem, 0, sizeof(float)*(rtjoin->nbin-1));

	wtk_complex_zero_p2(rtjoin->W, rtjoin->cfg->channel-1, rtjoin->nbin*(rtjoin->cfg->M+rtjoin->cfg->M2));
	wtk_complex_zero_p2(rtjoin->X, rtjoin->cfg->channel, rtjoin->nbin*(rtjoin->cfg->M+rtjoin->cfg->M2));
	wtk_float_zero_p2(rtjoin->power_x, rtjoin->cfg->channel-1, rtjoin->nbin);
	wtk_complex_zero_p2(rtjoin->fft, rtjoin->cfg->channel, rtjoin->nbin);

	memset(rtjoin->fftx, 0, sizeof(wtk_complex_t)*(rtjoin->nbin));

	rtjoin->chioce_ch=0;
	rtjoin->hide_chioce_ch=0;
	if(rtjoin->nchenr)
	{
		memset(rtjoin->nchenr, 0, sizeof(float)*rtjoin->cfg->channel);
		memset(rtjoin->nchenr2, 0, sizeof(float)*rtjoin->cfg->channel);
		memset(rtjoin->nchenr3, 0, sizeof(float)*rtjoin->cfg->channel);
		memset(rtjoin->nlms_idx, 0, sizeof(int)*rtjoin->cfg->channel);
		memset(rtjoin->nlms_weight, 0, sizeof(float)*rtjoin->cfg->channel);
		for(int i=0;i<rtjoin->cfg->channel;i++)
		{
			wtk_fring_reset(rtjoin->fring[i]);
		}
	}
	if(rtjoin->change_frame){
		memset(rtjoin->change_frame, 0, sizeof(int)*rtjoin->cfg->change_frame_len);
	}
	rtjoin->change_frame_delay=0;
	rtjoin->init_change_frame=rtjoin->cfg->init_change_frame;
	rtjoin->bs_scale=1.0;
	rtjoin->bs_last_scale=1.0;
	rtjoin->bs_max_cnt=0;
	rtjoin->csd_in_cnt=0;
	rtjoin->csd_out_cnt=0;
	rtjoin->mean_nchenr_cnt=0;
	rtjoin->nchenr_state=0;
	rtjoin->nchenr_cnt=rtjoin->cfg->nchenr_cnt;
	rtjoin->change_delay=0;
	rtjoin->nlms_change_init=rtjoin->cfg->nlms_change_init;
}


void wtk_rtjoin_set_notify(wtk_rtjoin_t *rtjoin,void *ths,wtk_rtjoin_notify_f notify)
{
	rtjoin->notify=notify;
	rtjoin->ths=ths;
}

int wtk_rtjoin_feed_nlms(wtk_rtjoin_t *rtjoin, wtk_complex_t *fft, int nch)
{
	wtk_complex_t *X=rtjoin->X[nch];
	wtk_complex_t *W=rtjoin->W[nch];
	wtk_complex_t  *Xtmp;
    wtk_complex_t *wtmp, *ytmp, *etmp;
	wtk_complex_t *E=rtjoin->E;
    wtk_complex_t *Y=rtjoin->Y;
	float *power_x=rtjoin->power_x[nch];
    int nbin=rtjoin->nbin;
    int M=rtjoin->cfg->M;
	int M2=rtjoin->cfg->M2;
	int MM2=M+M2;
    int i,k;
	float r,f,max_w;
	int idx;
	float mufb=rtjoin->cfg->mufb;

	memset(Y, 0, sizeof(wtk_complex_t)*rtjoin->nbin);
	for(i=0, Xtmp=X, wtmp=W; i<MM2; ++i)
	{
		for(k=0, ytmp=Y; k<nbin; ++k,++Xtmp,++wtmp,++ytmp)
		{
			ytmp->a+=Xtmp->a*wtmp->a-Xtmp->b*wtmp->b;
			ytmp->b+=Xtmp->a*wtmp->b+Xtmp->b*wtmp->a;
		}
	}
	for(k=0, etmp=E, ytmp=Y; k<nbin; ++k, ++etmp, ++ytmp,++fft)
	{
		etmp->a=fft->a-ytmp->a;
		etmp->b=fft->b-ytmp->b;
	}

	wtmp=W;
	Xtmp=X;
	max_w=0;
	idx=0;
	for(i=0; i<MM2; ++i)
	{
		etmp=E;
		f=1e-9;
		
		r=mufb/(MM2*power_x[0]+1e-9);
		wtmp->a+=r*(Xtmp->a*etmp->a+Xtmp->b*etmp->b);
		wtmp->b+=r*(-Xtmp->b*etmp->a+Xtmp->a*etmp->b);
		++wtmp; ++etmp; ++Xtmp;
		for(k=1; k<nbin-1; ++k, ++wtmp, ++etmp, ++Xtmp)
		{
			r=mufb/(MM2*power_x[k]+1e-9);
			wtmp->a+=r*(Xtmp->a*etmp->a+Xtmp->b*etmp->b);
			wtmp->b+=r*(-Xtmp->b*etmp->a+Xtmp->a*etmp->b);
			f+=wtmp->a*wtmp->a+wtmp->b*wtmp->b;
		}
		r=mufb/(MM2*power_x[nbin-1]+1e-9);
		wtmp->a+=r*(Xtmp->a*etmp->a+Xtmp->b*etmp->b);
		wtmp->b+=r*(-Xtmp->b*etmp->a+Xtmp->a*etmp->b);
		++wtmp; ++etmp; ++Xtmp;

		if(f>max_w)
		{
			max_w=f;
			idx=i;
		}
	}
	rtjoin->nlms_weight[nch] = max_w;
	// if(nch==1){
	// 	printf("%f\n", max_w);
	// }
	return idx;
}

void wtk_rtjoin_control_bs(wtk_rtjoin_t *rtjoin, float *out, int len)
{
	float out_max;
	int i;

	out_max=wtk_float_abs_max(out, len);
	if(out_max>32000.0)
	{
		rtjoin->bs_scale=32000.0f/out_max;
		if(rtjoin->bs_scale<rtjoin->bs_last_scale)
		{
			rtjoin->bs_last_scale=rtjoin->bs_scale;
		}else
		{
			rtjoin->bs_scale=rtjoin->bs_last_scale;
		}
		rtjoin->bs_max_cnt=5;
	}
	for(i=0; i<len; ++i)
	{
		out[i]*=rtjoin->bs_scale;
	}
	if(rtjoin->bs_max_cnt>0)
	{
		--rtjoin->bs_max_cnt;
	}
	if(rtjoin->bs_max_cnt<=0 && rtjoin->bs_scale<1.0)
	{
		rtjoin->bs_scale*=1.1f;
		rtjoin->bs_last_scale=rtjoin->bs_scale;
		if(rtjoin->bs_scale>1.0)
		{
			rtjoin->bs_scale=1.0;
			rtjoin->bs_last_scale=1.0;
		}
	}
} 

void wtk_rtjoin_feed(wtk_rtjoin_t *rtjoin,short *data,int len,int is_end)
{
	int i,j,k;
	int channel=rtjoin->cfg->channel;
	wtk_strbuf_t **mic=rtjoin->mic;
	float fv, *fp1;
	int wins=rtjoin->cfg->wins;
	int fsize=wins/2;
	int length;
	float *rfft_in=rtjoin->rfft_in;
	wtk_drft_t *rfft=rtjoin->rfft;
	wtk_complex_t **fft=rtjoin->fft, *fft1;
	float **analysis_mem=rtjoin->analysis_mem;
	float *synthesis_mem=rtjoin->synthesis_mem;
	float *analysis_window=rtjoin->window, *synthesis_window=rtjoin->synthesis_window;
	wtk_complex_t *fftx=rtjoin->fftx;
	float *out=rtjoin->out;
	short *pv=(short *)out;
    wtk_complex_t **X=rtjoin->X;
	int nbin=rtjoin->nbin;
	int M=rtjoin->cfg->M;
	int M2=rtjoin->cfg->M2;
	int MM2=M+M2;
	static int nframe=0;
	int clip_s=rtjoin->cfg->clip_s;
	int clip_e=rtjoin->cfg->clip_e;
	float **power_x=rtjoin->power_x;
	float power_alpha=rtjoin->cfg->power_alpha, f;
	float power_alpha2=rtjoin->cfg->power_alpha2;
	int idx;
	float *nchenr=rtjoin->nchenr;
	float *nchenr2=rtjoin->nchenr2;
	float *nchenr3=rtjoin->nchenr3;

	for(i=0;i<len;++i)
	{
		for(j=0; j<channel; ++j)
		{
			fv=data[j];
			wtk_strbuf_push(mic[j],(char *)&(fv),sizeof(float));
		}
		data+=channel;
	}
	length=mic[0]->pos/sizeof(float);
	while(length>=fsize)
	{
		if(nframe<=M2)
		{
			++nframe;
		}
		for(i=0; i<channel; ++i)
		{
			fp1=(float *)mic[i]->data;
			wtk_drft_frame_analysis(rfft, rfft_in, analysis_mem[i], fft[i], fp1, wins, analysis_window);
		}
		for(i=0;i<channel;++i)
		{
			memmove(X[i]+nbin,X[i],sizeof(wtk_complex_t)*(MM2-1)*nbin);
			memcpy(X[i], fft[i], sizeof(wtk_complex_t)*nbin);
		}
		for(i=0;i<(channel-1);++i)
		{
			fft1=fft[i];
			for(k=0; k<nbin; ++k,++fft1)
			{
				power_x[i][k]=(1-power_alpha)*power_x[i][k]+power_alpha*(fft1->a*fft1->a+fft1->b*fft1->b);
			}
		}
		if(nchenr2)
		{
			for(i=0;i<channel;++i)
			{
				fft1=fft[i]+1+clip_s;
				f=0;
				for(k=clip_s+1; k<clip_e; ++k, ++fft1)
				{
					f+=fft1->a*fft1->a+fft1->b*fft1->b;
				}
				// if(i==4){
				// 	printf("%f\n", f);
				// }
				nchenr3[i]=f;  // 当前能量，用于低音量判断是否可以切换
				nchenr2[i]=(1-power_alpha2)*nchenr2[i]+power_alpha2*f;
				nchenr[i] = nchenr2[i]; 
				wtk_fring_push2(rtjoin->fring[i], nchenr2[i]);
			}
		}
		if(rtjoin->cfg->mean_nchenr_thresh!=-1){
			float mean_nchenr=0;
			if(nchenr)
			{
				for(i=0;i<channel;++i)
				{
					mean_nchenr+=nchenr2[i];
				}
				mean_nchenr/=channel;
			}
			if(mean_nchenr <= rtjoin->cfg->mean_nchenr_thresh){
				++rtjoin->mean_nchenr_cnt;
			}else{
				rtjoin->mean_nchenr_cnt=0;
			}
			if(rtjoin->mean_nchenr_cnt >=rtjoin->cfg->mean_nchenr_cnt){
				rtjoin->cfg->power_alpha2 = rtjoin->cfg->sil_power_alpha2;
				rtjoin->nchenr_state=0;
				// printf("0\n");
			}else{
				if(rtjoin->nchenr_state==0){
					rtjoin->nchenr_cnt=rtjoin->cfg->nchenr_cnt;
					for(i=0;i<channel;++i){
						wtk_fring_reset(rtjoin->fring[i]);
						wtk_fring_push2(rtjoin->fring[i], nchenr2[i]);
					}
				}
				// printf("1\n");
				rtjoin->cfg->power_alpha2 = rtjoin->cfg->sound_power_alpha2;
				rtjoin->nchenr_state=1;
			}
			--rtjoin->nchenr_cnt;
		}
		// static int cnt=0;
		// cnt++;
		if(rtjoin->nlms_change_init>0){
			--rtjoin->nlms_change_init;
		}
		if(nframe>M2)
		{
			// printf("%d, %f == > ", M2,1.0/16000*cnt*(nbin-1));
			for(i=0;i<channel-1;++i)
			{
				idx=wtk_rtjoin_feed_nlms(rtjoin, X[channel-1]+nbin*M2, i);
				if(rtjoin->nlms_change_init==0){
					if(idx!=rtjoin->nlms_idx[i]){
						if(rtjoin->nchenr_state==1 && rtjoin->chioce_ch==i){
							idx = rtjoin->nlms_idx[i];
						}
					}
				}
				if(rtjoin->nlms_weight[i] > rtjoin->cfg->nlms_align_thresh){
					idx = rtjoin->nlms_idx[i];
				}else if(abs(idx-rtjoin->nlms_idx[i])>rtjoin->cfg->nlms_change_cnt && rtjoin->nlms_weight[i] > rtjoin->cfg->nlms_align_thresh2){
					idx = rtjoin->nlms_idx[i];
				}
				if(rtjoin->nchenr_state==0){
					idx=M2;
				}
				rtjoin->nlms_idx[i] = idx;
				memcpy(fft[i], X[i]+nbin*idx, sizeof(wtk_complex_t)*nbin);
				// if(i==3){
				// 	// printf("%d\n", rtjoin->nchenr_state);
				// 	// printf("%d\n", idx);
				// 	printf("%f\n", rtjoin->nlms_weight[i]);
				// }
				// printf("%d ",idx);
			}
			rtjoin->nlms_weight[i] = wtk_float_mean(rtjoin->nlms_weight, channel-1);
			memcpy(fft[i], X[i]+nbin*M2, sizeof(wtk_complex_t)*nbin);
			if(nchenr)
			{
				f=nchenr[0];
				idx=0;
				f = wtk_fring_mean(rtjoin->fring[0]);
				nchenr[0] = wtk_fring_max(rtjoin->fring[0]);
				// for(j=0;j<MM2;++j){
				// 	if(wtk_fring_at(rtjoin->fring[0], j)>f){
				// 		f=wtk_fring_at(rtjoin->fring[0], j);
				// 		if(f>nchenr[0])
				// 		{
				// 			nchenr[0]=f;
				// 		}
				// 	}
				// }
				// printf("%f\n", nchenr[0]);
				for(i=1;i<channel;++i)
				{
					// if(nchenr[i]>f)
					// {
					// 	idx=i;
					// 	f=nchenr[i];
					// }
					if(wtk_fring_mean(rtjoin->fring[i]) > f){
						f = wtk_fring_mean(rtjoin->fring[i]);
						idx=i;
					}
					nchenr[i] = wtk_fring_max(rtjoin->fring[i]);
					// for(j=0;j<MM2;++j){
					// 	if(wtk_fring_at(rtjoin->fring[i], j)>f){
					// 		idx=i;
					// 		f=wtk_fring_at(rtjoin->fring[i], j);
					// 		if(f>nchenr[i])
					// 		{
					// 			nchenr[i]=f;
					// 		}
					// 	}
					// }
				}
				f = wtk_fring_max(rtjoin->fring[idx]);
				// printf("%f\n", nchenr[1]);
				// printf("%d\n", idx);
				--rtjoin->change_delay;
				if(rtjoin->cfg->change_frame_len<=0){
					if(f >= nchenr[rtjoin->chioce_ch]*rtjoin->cfg->change_thresh && rtjoin->change_delay<=0)
					{
						rtjoin->chioce_ch=idx;
						rtjoin->change_delay=rtjoin->cfg->change_delay;
					}
					memcpy(fftx, fft[rtjoin->chioce_ch], sizeof(wtk_complex_t)*nbin);
				}else{
					memcpy(rtjoin->change_frame, rtjoin->change_frame+1, sizeof(int)*(rtjoin->cfg->change_frame_len-1));
					// printf("%d\n", idx);
					if(rtjoin->chioce_ch != idx && rtjoin->hide_chioce_ch != rtjoin->chioce_ch && rtjoin->nlms_weight[idx] > rtjoin->cfg->nlms_mix_thresh)
					{
						rtjoin->change_frame[rtjoin->cfg->change_frame_len-1]=1;
					}else{
						rtjoin->change_frame[rtjoin->cfg->change_frame_len-1]=0;
					}
					rtjoin->hide_chioce_ch=idx;
					if(rtjoin->nchenr_cnt>=0){
						if(f>=nchenr[rtjoin->chioce_ch]){
							rtjoin->chioce_ch=idx;
						}
					}else if(f >= nchenr[rtjoin->chioce_ch]*rtjoin->cfg->change_thresh && nchenr3[rtjoin->chioce_ch] < rtjoin->cfg->chioce_thresh  && rtjoin->change_delay<=0)
					{
						rtjoin->chioce_ch=idx;
						rtjoin->change_delay=rtjoin->cfg->change_delay;
					}
					// printf("%d\n", rtjoin->chioce_ch);
					f = wtk_int_sum(rtjoin->change_frame, rtjoin->cfg->change_frame_len);  // 切换频率
					// printf("%f\n", nchenr[4]);
					// printf("%f\n", weight[idx]);
					// printf("%f\n", f);
					// printf("%d\n", rtjoin->chioce_ch);
					if(f>=rtjoin->cfg->change_frame_num){  // 短期内切换次数达到阈值
						rtjoin->change_frame_delay=rtjoin->cfg->change_frame_delay;
					}else if(f<rtjoin->cfg->change_frame_num2){  // 切换次数低于阈值2
						--rtjoin->change_frame_delay;
					}
					if(rtjoin->init_change_frame>=0){  // nlms未迭代完善之前不使用混音
						--rtjoin->init_change_frame;
						memcpy(fftx, fft[rtjoin->chioce_ch], sizeof(wtk_complex_t)*nbin);
						if(rtjoin->init_change_frame==0){
							memset(rtjoin->change_frame, 0, sizeof(int)*rtjoin->cfg->change_frame_len);
							rtjoin->change_frame_delay=0;
						}
						// printf("-1\n");
					}else{
						if(rtjoin->change_frame_delay<=0){
							memcpy(fftx, fft[rtjoin->chioce_ch], sizeof(wtk_complex_t)*nbin);
							// printf("-2\n");
						}else{  // 挑选能量最大的两个通道进行混音
							// printf("0\n");
							memset(fftx, 0, sizeof(wtk_complex_t)*nbin);
							int idx2=0;
							f=0;
							idx=rtjoin->chioce_ch;
							for(i=0;i<channel;++i)
							{
								if(nchenr[i]>f && i!=idx)
								{
									idx2=i;
									f=nchenr[i];
								}
							}
							if(rtjoin->cfg->use_csd && abs(idx-idx2)>2){  // 使用CSD计算互功率谱，同一个人的语音则不合并
								float en=0;
								wtk_complex_t *ffty;
								for(i=0;i<MM2;++i){
									float sum=0;
									wtk_complex_t en_fft;
									ffty=X[idx2]+nbin*i;
									for(k=clip_s+1; k<clip_e; ++k)
									{
										en_fft.a=fft[idx][k].a*ffty[k].a+fft[idx][k].b*ffty[k].b;
										en_fft.b=fft[idx][k].a*ffty[k].b-fft[idx][k].b*ffty[k].a;
										sum+=en_fft.a*en_fft.a+en_fft.b*en_fft.b;
									}
									if(sum>en){
										en=sum;
									}
								}
								// printf("%f\n", en);
								if(en > rtjoin->cfg->csd_thresh){
									++rtjoin->csd_in_cnt;
								}else{
									rtjoin->csd_in_cnt=0;
									--rtjoin->csd_out_cnt;
								}
								if(rtjoin->csd_in_cnt>rtjoin->cfg->csd_in_cnt){
									rtjoin->csd_out_cnt=rtjoin->cfg->csd_out_cnt;
								}
							}
							// printf("%d\n", rtjoin->csd_out_cnt);
							if(rtjoin->csd_out_cnt>=0){
								for(k=clip_s+1; k<clip_e; ++k)
								{
									fftx[k].a=fft[idx][k].a;
									fftx[k].b=fft[idx][k].b;
								}
							}else{
								for(i=0;i<channel;++i)
								{
									if(i==idx || i==idx2){
										for(k=clip_s+1; k<clip_e; ++k)
										{
											fftx[k].a+=fft[i][k].a;
											fftx[k].b+=fft[i][k].b;
										}
									}
								}
								for(k=clip_s+1; k<clip_e; ++k)
								{
									f=rtjoin->cfg->mix_scale/2;
									fftx[k].a*=f;
									fftx[k].b*=f;
								}
							}
						}
					}
				}
				// printf("%f\n", wtk_int_sum(rtjoin->change_frame, rtjoin->cfg->change_frame_len));
				// printf("%f %f %d\n",nchenr[0],nchenr[1],rtjoin->chioce_ch);
				// memcpy(fftx, fft[rtjoin->chioce_ch], sizeof(wtk_complex_t)*nbin);
			}else
			{
				memset(fftx, 0, sizeof(wtk_complex_t)*nbin);
				for(i=0;i<channel;++i)
				{
					for(k=clip_s+1; k<clip_e; ++k)
					{
						fftx[k].a+=fft[i][k].a;
						fftx[k].b+=fft[i][k].b;
					}
				}
				for(k=clip_s+1; k<clip_e; ++k)
				{
					f=rtjoin->cfg->mix_scale/channel;
					fftx[k].a*=f;
					fftx[k].b*=f;
				}
			}
			for(k=0; k<=clip_s; ++k)
			{
				fftx[k].a=fftx[k].b=0;
			}
			for(k=clip_e; k<nbin; ++k)
			{
				fftx[k].a=fftx[k].b=0;
			}
		    wtk_drft_frame_synthesis(rfft, rfft_in, synthesis_mem, fftx, out, wins, synthesis_window);
			if(rtjoin->eq)
			{
				wtk_equalizer_feed_float(rtjoin->eq, out, fsize);
			}
			if(rtjoin->cfg->use_control_bs){
				wtk_rtjoin_control_bs(rtjoin, out, fsize);
			}
			for(i=0; i<fsize; ++i)
			{
				pv[i]=floorf(out[i]+0.5);
			}
			if(rtjoin->notify)
			{
				rtjoin->notify(rtjoin->ths,pv,fsize);
			}
		}
		wtk_strbufs_pop(mic, channel, fsize*sizeof(float));
		length=mic[0]->pos/sizeof(float);
	}
}

void wtk_rtjoin_feed2(wtk_rtjoin_t *rtjoin,short *data,int len,int choice_ch,int is_end)
{
	int i,j,k;
	int channel=rtjoin->cfg->channel;
	wtk_strbuf_t **mic=rtjoin->mic;
	float fv, *fp1;
	int wins=rtjoin->cfg->wins;
	int fsize=wins/2;
	int length;
	float *rfft_in=rtjoin->rfft_in;
	wtk_drft_t *rfft=rtjoin->rfft;
	wtk_complex_t **fft=rtjoin->fft, *fft1;
	float **analysis_mem=rtjoin->analysis_mem;
	float *synthesis_mem=rtjoin->synthesis_mem;
	float *analysis_window=rtjoin->window, *synthesis_window=rtjoin->synthesis_window;
	wtk_complex_t *fftx=rtjoin->fftx;
	float *out=rtjoin->out;
	short *pv=(short *)out;
	int nbin=rtjoin->nbin;
	int clip_s=rtjoin->cfg->clip_s;
	int clip_e=rtjoin->cfg->clip_e;
	float f;
	float power_alpha2=rtjoin->cfg->power_alpha2;
	float power_alpha3=rtjoin->cfg->power_alpha3;
	float *nchenr=rtjoin->nchenr;
	float *nchenr2=rtjoin->nchenr2;
	float tmp_nchenr=0;
	int chn=0;

	for(i=0;i<len;++i)
	{
		for(j=0; j<channel; ++j)
		{
			fv=data[j];
			wtk_strbuf_push(mic[j],(char *)&(fv),sizeof(float));
		}
		data+=channel;
	}
	length=mic[0]->pos/sizeof(float);
	while(length>=fsize)
	{
		for(i=0; i<channel; ++i)
		{
			fp1=(float *)mic[i]->data;
			wtk_drft_frame_analysis(rfft, rfft_in, analysis_mem[i], fft[i], fp1, wins, analysis_window);
		}
		if(nchenr)
		{
			for(i=0;i<channel;++i)
			{
				fft1=fft[i]+1+clip_s;
				f=0;
				for(k=clip_s+1; k<clip_e; ++k, ++fft1)
				{
					f+=fft1->a*fft1->a+fft1->b*fft1->b;
				}
				nchenr[i]=(1-power_alpha2)*nchenr[i]+power_alpha2*f;
				nchenr2[i]=(1-power_alpha3)*nchenr2[i]+power_alpha3*f;
				// if(i==choice_ch){
				// 	printf("%f\n", nchenr[i]);
				// }
				if(tmp_nchenr<nchenr[i])
				{
					tmp_nchenr = nchenr[i];
					chn=i;
				}
			}
			if(nchenr2[choice_ch]>=rtjoin->cfg->power_thresh)
			{
				rtjoin->chioce_ch=choice_ch;
			}else
			{
				rtjoin->chioce_ch=chn;
			}
			// printf("%d\n", chn);
			// printf("%d\n", rtjoin->chioce_ch);
			memcpy(fftx, fft[rtjoin->chioce_ch], sizeof(wtk_complex_t)*nbin);
		}
		for(k=0; k<=clip_s; ++k)
		{
			fftx[k].a=fftx[k].b=0;
		}
		for(k=clip_e; k<nbin; ++k)
		{
			fftx[k].a=fftx[k].b=0;
		}
		wtk_strbufs_pop(mic, channel, fsize*sizeof(float));
		length=mic[0]->pos/sizeof(float);
		wtk_drft_frame_synthesis(rfft, rfft_in, synthesis_mem, fftx, out, wins, synthesis_window);
		if(rtjoin->eq)
		{
			wtk_equalizer_feed_float(rtjoin->eq, out, fsize);
		}
		for(i=0; i<fsize; ++i)
		{
			pv[i]=floorf(out[i]+0.5);
		}
		if(rtjoin->notify)
		{
			rtjoin->notify(rtjoin->ths,pv,fsize);
		}
	}
}
