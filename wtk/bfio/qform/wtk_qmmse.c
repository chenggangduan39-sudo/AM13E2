#include "wtk_qmmse.h"

#define qmmse_tobank(n)   (13.1f*atan(.00074f*(n))+2.24f*atan((n)*(n)*1.85e-8f)+1e-4f*(n))

wtk_qmmse_xfilterbank_t* wtk_qmmse_xfilterbank_new(int bands,int rate,int len)
{
	wtk_qmmse_xfilterbank_t *bank;
	float df;
	float max_mel, mel_interval;
   int i;
   int id1;
   int id2;
   float curr_freq;
   float mel;
   float val;

   bank = (wtk_qmmse_xfilterbank_t*)wtk_malloc(sizeof(wtk_qmmse_xfilterbank_t));
   df =rate*1.0f/(2*len);
   max_mel = qmmse_tobank(rate/2);
   mel_interval =max_mel/(bands-1);
   bank->nb_banks = bands;
   bank->len = len;
   bank->bank_left = (int*)wtk_malloc(len*sizeof(int));
   bank->bank_right = (int*)wtk_malloc(len*sizeof(int));
   bank->filter_left = (float*)wtk_malloc(len*sizeof(float));
   bank->filter_right = (float*)wtk_malloc(len*sizeof(float));
   for (i=0;i<len;i++)
   {
      curr_freq = i*df;
      mel = qmmse_tobank(curr_freq);
      if (mel > max_mel)
      {
         break;
      }
      id1 = (int)(floor(mel/mel_interval));
      if (id1>bands-2)
      {
         id1 = bands-2;
         val = 1;
      }else
      {
    	  val=(mel - id1*mel_interval)/mel_interval;
      }
      id2 = id1+1;
      bank->bank_left[i] = id1;
      bank->filter_left[i] = 1-val;
      bank->bank_right[i] = id2;
      bank->filter_right[i] = val;
     //wtk_debug("v[%d]=%d/%d %f/%f mel=%f/%f %f\n",i,bank->bank_left[i],bank->bank_right[i],bank->filter_left[i],bank->filter_right[i],mel,curr_freq,df);
   }
   return bank;

}

void wtk_qmmse_xfilterbank_delete(wtk_qmmse_xfilterbank_t *bank)
{
	   wtk_free(bank->bank_left);
	   wtk_free(bank->bank_right);
	   wtk_free(bank->filter_left);
	   wtk_free(bank->filter_right);
	   wtk_free(bank);
}


void wtk_qmmse_xfilterbank_compute_bank32(wtk_qmmse_xfilterbank_t *bank, float *ps, float *mel)
{
   int i;
   int *left=bank->bank_left;
   int *right=bank->bank_right;
   float *fleft=bank->filter_left;
   float *fright=bank->filter_right;

   memset(mel,0,bank->nb_banks*sizeof(float));
   for (i=0;i<bank->len;i++)
   {
      mel[left[i]] += fleft[i]*ps[i];
      mel[right[i]] += fright[i]*ps[i];
    // wtk_debug("v[%d]=%f/%f %f %f/%f\n",i,mel[left[i]],mel[right[i]],ps[i],fleft[i],fright[i]);
   }
}


void wtk_qmmse_xfilterbank_compute_psd16(wtk_qmmse_xfilterbank_t *bank,float *mel,float *ps)
{
   int i;
   int *left=bank->bank_left;
   int *right=bank->bank_right;
   float *fleft=bank->filter_left;
   float *fright=bank->filter_right;

   for (i=0;i<bank->len;i++)
   {
      ps[i] = mel[left[i]]*fleft[i]+mel[right[i]]*fright[i];
   }
}

wtk_qmmse_t* wtk_qmmse_new(wtk_qmmse_cfg_t *cfg)
{
    wtk_qmmse_t *qmmse;

    qmmse=(wtk_qmmse_t *)wtk_malloc(sizeof(wtk_qmmse_t));
    qmmse->cfg=cfg;

    qmmse->bank=NULL;
    if(qmmse->cfg->use_bank)
    {
        qmmse->nm=cfg->step+cfg->nbands;
        qmmse->bank=wtk_qmmse_xfilterbank_new(cfg->nbands,cfg->rate,cfg->step);
        qmmse->zeta=(float*)wtk_calloc(cfg->nbands,sizeof(float));
    }else
    {
        qmmse->nm=cfg->step;
        qmmse->zeta=(float*)wtk_calloc(cfg->step,sizeof(float));
    }
    qmmse->min_noise=1e-9;

    qmmse->noise_frame=(int*)wtk_calloc(cfg->step,sizeof(int));
    qmmse->noise=(float*)wtk_calloc(qmmse->nm,sizeof(float));

    qmmse->echo_noise=NULL;
    if(cfg->use_echonoise)
    {
        qmmse->echo_noise=(float*)wtk_calloc(qmmse->nm,sizeof(float));
    }
    
    qmmse->ps=(float*)wtk_calloc(qmmse->nm,sizeof(float));
    qmmse->old_ps=(float*)wtk_calloc(qmmse->nm,sizeof(float));
    qmmse->prior=(float*)wtk_calloc(qmmse->nm,sizeof(float));
    qmmse->post=(float*)wtk_calloc(qmmse->nm,sizeof(float));
    qmmse->gain2=(float*)wtk_calloc(qmmse->nm,sizeof(float));
    qmmse->gain=(float*)wtk_calloc(qmmse->nm,sizeof(float));
    qmmse->gain_floor=(float*)wtk_calloc(qmmse->nm,sizeof(float));
    qmmse->loudness_weight = (float*)wtk_malloc(cfg->step*sizeof(float));

    qmmse->pn1=(float *)wtk_malloc((cfg->step)*sizeof(float));
    qmmse->sym=(float *)wtk_malloc((cfg->step)*sizeof(float));

    qmmse->S=(float*)wtk_calloc(cfg->step,sizeof(float));
    qmmse->Smin=(float*)wtk_calloc(cfg->step,sizeof(float));
    qmmse->Stmp=(float*)wtk_calloc(cfg->step,sizeof(float));
    qmmse->update_prob=(char*)wtk_calloc(cfg->step,sizeof(char));


    qmmse->Se=NULL;
    qmmse->Sd=NULL;
    qmmse->Sed=NULL;
    qmmse->leak=NULL;
    if(cfg->use_sed){
        qmmse->Se=(float *)wtk_malloc(sizeof(float)*cfg->step);
        qmmse->Sd=(float *)wtk_malloc(sizeof(float)*cfg->step);
        qmmse->Sed=(wtk_complex_t *)wtk_malloc(sizeof(wtk_complex_t)*cfg->step);
        qmmse->leak=(float *)wtk_malloc(sizeof(float)*cfg->step);
    }
    qmmse->tgt_eng_frame=NULL;
    if(cfg->use_agc_smooth){
        qmmse->tgt_eng_frame=(float*)wtk_malloc(cfg->smooth_frame*sizeof(float));
    }
    qmmse->down_eng_frame=NULL;
    if(cfg->use_down_agc){
        qmmse->down_eng_frame=(float*)wtk_malloc(cfg->smooth_frame*sizeof(float));
    }

    qmmse->entropy_E=NULL;
    qmmse->entropy_Eb=NULL;
    if(cfg->entropy_thresh!=-1){
        qmmse->entropy_E=(float*)wtk_malloc((cfg->step+1)*sizeof(float));
        qmmse->entropy_Eb=(float *)wtk_malloc(cfg->step*2*sizeof(float));
    }

    wtk_qmmse_reset(qmmse);

    return qmmse;
}

void wtk_qmmse_delete(wtk_qmmse_t *qmmse)
{
    if(qmmse->bank)
    {
	    wtk_qmmse_xfilterbank_delete(qmmse->bank);
    }
    wtk_free(qmmse->gain2);
    wtk_free(qmmse->zeta);
    wtk_free(qmmse->noise_frame);
    wtk_free(qmmse->noise);
    if(qmmse->echo_noise)
    {
        wtk_free(qmmse->echo_noise);
    }
    wtk_free(qmmse->ps);
    wtk_free(qmmse->old_ps);
    wtk_free(qmmse->prior);
    wtk_free(qmmse->post);
    wtk_free(qmmse->gain);
    wtk_free(qmmse->gain_floor);
    wtk_free(qmmse->loudness_weight);

    wtk_free(qmmse->S);
    wtk_free(qmmse->Smin);
    wtk_free(qmmse->Stmp);
    wtk_free(qmmse->update_prob);
    
    wtk_free(qmmse->pn1);
    wtk_free(qmmse->sym);
    if(qmmse->Se){
        wtk_free(qmmse->Se);
    }
    if(qmmse->Sd){
        wtk_free(qmmse->Sd);
    }
    if(qmmse->Sed){
        wtk_free(qmmse->Sed);
    }
    if(qmmse->leak){
        wtk_free(qmmse->leak);
    }
    if(qmmse->tgt_eng_frame)
    {
        wtk_free(qmmse->tgt_eng_frame);
    }
    if(qmmse->down_eng_frame)
    {
        wtk_free(qmmse->down_eng_frame);
    }
    if(qmmse->entropy_E)
    {
        wtk_free(qmmse->entropy_E);
    }
    if(qmmse->entropy_Eb)
    {
        wtk_free(qmmse->entropy_Eb);
    }

    wtk_free(qmmse);
}

void wtk_qmmse_reset(wtk_qmmse_t *qmmse)
{
    int i;
    float ff;

    qmmse->nframe=0;
    qmmse->den_cnt=qmmse->nb_adapt=0;
    memset(qmmse->update_prob,0,(qmmse->cfg->step)*sizeof(char));
    memset(qmmse->S,0,(qmmse->cfg->step)*sizeof(float));
    memset(qmmse->Smin,0,(qmmse->cfg->step)*sizeof(float));
    memset(qmmse->Stmp,0,(qmmse->cfg->step)*sizeof(float));

    for(i=0;i<qmmse->cfg->step;++i)
    {
        qmmse->noise_frame[i]=0;
        qmmse->noise[i]=qmmse->min_noise;

        if(qmmse->echo_noise)
        {
            qmmse->echo_noise[i]=qmmse->min_noise;
        }
    }

    if(qmmse->bank)
    {
        memset(qmmse->zeta,0,(qmmse->cfg->nbands)*sizeof(float));
    }else
    {
        memset(qmmse->zeta,0,(qmmse->cfg->step)*sizeof(float));
    }
    memset(qmmse->gain2,0,qmmse->nm*sizeof(float));
    memset(qmmse->gain_floor,0,qmmse->nm*sizeof(float));
    memset(qmmse->post,0,qmmse->nm*sizeof(float));
    memset(qmmse->prior,0,qmmse->nm*sizeof(float));
    memset(qmmse->ps,0,qmmse->nm*sizeof(float));
    memset(qmmse->old_ps,0,qmmse->nm*sizeof(float));
    memset(qmmse->gain,0,qmmse->nm*sizeof(float));
    for (i=0;i<qmmse->cfg->step;i++)
    {
        ff=i*0.5f*qmmse->cfg->rate*1.0f/(qmmse->cfg->step);
        /*qmmse->loudness_weight[i] = .5f*(1.f/(1.f+ff/8000.f))+1.f*expf(-.5f*(ff-3800.f)*(ff-3800.f)/9e5f);*/
        qmmse->loudness_weight[i] = 0.35f-.35f*ff/16000.f+.73f*expf(-0.5f*(ff-3800.f)*(ff-3800.f)/9e5f);
        if (qmmse->loudness_weight[i]<0.01f)
        {
            qmmse->loudness_weight[i]=0.01f;
        }
        qmmse->loudness_weight[i] *= qmmse->loudness_weight[i];
    }
    qmmse->loudness = 1e-15f;
    qmmse->agc_gain = 1.f;
    qmmse->out_agc_gain = 1.f;
    qmmse->prev_loudness = 1.f;
    qmmse->init_max = 1.f;
    qmmse->loudness_accum=1e-15f;
    
    for(i=0;i<qmmse->cfg->step;++i)
    {
        qmmse->pn1[i]=1.0f;
        qmmse->sym[i]=qmmse->cfg->init_sym;
    }
    if(qmmse->Se){
        memset(qmmse->Se, 0, sizeof(float)*qmmse->cfg->step);
    }
    if(qmmse->Sd){
        memset(qmmse->Sd, 0, sizeof(float)*qmmse->cfg->step);
    }
    if(qmmse->Sed){
        memset(qmmse->Sed, 0, sizeof(wtk_complex_t)*qmmse->cfg->step);
    }
    if(qmmse->leak){
        memset(qmmse->leak, 0, sizeof(float)*qmmse->cfg->step);
    }
    if(qmmse->tgt_eng_frame)
    {
        memset(qmmse->tgt_eng_frame, 0, sizeof(float)*qmmse->cfg->smooth_frame);
    }
    if(qmmse->down_eng_frame)
    {
        memset(qmmse->down_eng_frame, 0, sizeof(float)*qmmse->cfg->smooth_frame);
    }
    if(qmmse->entropy_E)
    {
        memset(qmmse->entropy_E, 0, sizeof(float)*(qmmse->cfg->step+1));
    }
    if(qmmse->entropy_Eb)
    {
        memset(qmmse->entropy_Eb, 0, sizeof(float)*qmmse->cfg->step*2);
    }
    qmmse->tgt_eng=qmmse->cfg->init_tgt_eng;
    qmmse->init_f=qmmse->cfg->smooth_frame;
    qmmse->smooth_f=qmmse->cfg->smooth_frame;
    qmmse->smooth_cnt=0;
    qmmse->down_eng = 1.0;
    qmmse->down_smooth_f = qmmse->cfg->down_frame;
    qmmse->down_cnt = qmmse->cfg->down_cnt;
    qmmse->agc_init_frame = qmmse->cfg->agc_init_frame;
    qmmse->loudness_frame = 0;
    qmmse->loudness_frame2 = 0;
    qmmse->pframe = 0;
    qmmse->sp_sil = 1;
    qmmse->agc_mask_frame = 0;
    qmmse->echo_agc_mask_frame = 0;
}

void wtk_qmmse_analysis(wtk_qmmse_t *qmmse,wtk_complex_t *fft)
{
	int N=qmmse->cfg->step;
	float *ps=qmmse->ps;
    int i;
    float *pn1=qmmse->pn1,*sym=qmmse->sym;
    float mm;

	for(i=1;i<N;++i)
	{
		ps[i]=fft[i].a*fft[i].a+fft[i].b*fft[i].b;

        if(qmmse->cfg->use_cnon)
        {
            pn1[i]= 0.9f*pn1[i]+0.1f*ps[i];
            if(qmmse->nframe>50)
            {
                mm=min(pn1[i],sym[i]);
                sym[i]=(mm+0.3f*(sym[i]-mm))*1.0003f;
            }
        }
	}
    if(qmmse->bank)
    {
    	wtk_qmmse_xfilterbank_compute_bank32(qmmse->bank,ps,ps+N);
    }
}

void wtk_qmmse_compute_gain_floor(float noise_suppress,float *noise,float *gain_floor,float min_noise,int len)
{
	float noise_floor;
	int i;

	noise_floor=expf(0.2302585f*noise_suppress);
	for(i=0;i<len;++i)
	{
		gain_floor[i]=sqrtf(noise_floor*noise[i])/sqrtf(min_noise+noise[i]);
	}
}

void wtk_qmmse_compute_gain_floor2(float noise_suppress,float noise_suppress2,float *noise,float *noise2,float *gain_floor,float min_noise,int len)
{
	float noise_floor;
    float noise_floor2;
	int i;

	noise_floor=expf(0.2302585f*noise_suppress);
    noise_floor2=expf(0.2302585f*noise_suppress2);
	for(i=0;i<len;++i)
	{
		gain_floor[i]=sqrtf(noise_floor*noise[i]+noise_floor2*noise2[i])/sqrtf(min_noise+noise[i]+noise2[i]);
	}
}

void wtk_qmmse_compute_gain_floor_echo(float noise_suppress,float echo_suppress,float *noise,
		float *echo,float *gain_floor,float min_noise,int len)
{
	float noise_floor,echo_floor;
	int i;

	noise_floor=expf(0.2302585f*noise_suppress);
	echo_floor=expf(0.2302585f*echo_suppress);
	for(i=0;i<len;++i)
	{
		//wtk_debug("v[%d]: %f/%f %f/%f\n",i,noise_floor,echo_floor,noise[i],echo[i]);
		gain_floor[i]=sqrtf(noise_floor*noise[i]+echo_floor*echo[i])/sqrtf(min_noise+noise[i]+echo[i]);
	}
}

double wtk_qmmse_expp(double x)
{
    int m,i,j;
    double s,p,ep,h,aa,bb,w,xx,g,r,q;
    static double t[5]={-0.9061798459f,-0.5384693101f,0.0f,
                         0.5384693101f,0.9061798459f};
    static double c[5]={0.2369268851f,0.4786286705f,0.5688888889f,
                        0.4786286705f,0.2369268851f};
    m=1;
    if(x==0)
    { 
        x=1.0e-10f;
    }
    if(x<0.0f)
    {
        x=-x;
    }
    r=0.57721566490153286060651f;
    q=r+log(x);
    h=x; 
    s=fabs(0.0001f*h);
    p=1.0e+35f; 
    ep=0.000001f; 
    g=0.0f;
    while((ep>=0.0000001f)&&(fabs(h)>s))
    {
        g=0.0f;
        for (i=1;i<=m;i++)
        { 
            aa=(i-1.0)*h; 
            bb=i*h;
            w=0.0f;
            for (j=0;j<=4;j++)
            { 
                xx=((bb-aa)*t[j]+(bb+aa))/2.0f;
                w=w+(expf(-xx)-1.0f)/xx*c[j];
            }
            g=g+w;
        }
        g=g*h/2.0f;
        ep=fabs(g-p)/(1.0f+fabs(g));
        p=g; 
        m=m+1; 
        h=x/m;
    }
    g=q+g;
    return g;
}

float wtk_qmmse_hypergeom_gain(float xx)
{
   int ind;
   float integer, frac;
   float x;
   static const float table[21] = {
      0.82157f, 1.02017f, 1.20461f, 1.37534f, 1.53363f, 1.68092f, 1.81865f,
      1.94811f, 2.07038f, 2.18638f, 2.29688f, 2.40255f, 2.50391f, 2.60144f,
      2.69551f, 2.78647f, 2.87458f, 2.96015f, 3.04333f, 3.12431f, 3.20326f};

      x = xx;
      integer = floor(2*x);
      ind = (int)integer;
     // wtk_debug("ind=%d\n",ind);
      if (ind<0)
      {
         return 1.f;
      }
      if (ind>19)
      {
         return (1.f+0.1296f/x);
      }
      frac = 2.f*x-integer;
      x=((1.f-frac)*table[ind] + frac*table[ind+1])/sqrtf(x+.0001f);
      return x;
}

void _swap(float* a, float* b) {  
    float t = *a;  
    *a = *b;  
    *b = t;  
}  
  
int _partition(float arr[], int low, int high) {  
    float pivot = arr[high];    // 选择最后一个元素作为轴点  
    int i = (low - 1);  // 小于轴点的元素索引  
  
    for (int j = low; j <= high - 1; j++) {  
        // 如果当前元素小于或等于轴点  
        if (arr[j] <= pivot) {  
            i++;    // 增加索引  
            _swap(&arr[i], &arr[j]);  
        }  
    }  
    _swap(&arr[i + 1], &arr[high]);  
    return (i + 1);  
}  
  
void _quickSort(float arr[], int low, int high) {  
    if (low < high) {  
        /* pi 是分区后pivot的索引，arr[p]已经在最终位置 */  
        int pi = _partition(arr, low, high);  
  
        // 分别对轴点左右两侧进行递归排序  
        _quickSort(arr, low, pi - 1);  
        _quickSort(arr, pi + 1, high);  
    }  
} 

float wtk_qmmse_entropy(wtk_qmmse_t *qmmse) {
    int rate = qmmse->cfg->rate;
    int wins = qmmse->cfg->step * 2;
    int nbin = wins/2+1;
    int i;
    int fx1 = (250 * 1.0 * wins) / rate;
    int fx2 = (3500 * 1.0 * wins) / rate;
    int km = floor(wins * 1.0 / 8);
    float K = 0.5;
    float *E = qmmse->entropy_E;
    float P1;
    float *Eb = qmmse->entropy_Eb;
    float sum;
    float prob;
    float Hb;

    memset(E, 0, sizeof(float) * nbin);
    memset(Eb, 0, sizeof(float) * wins);
    memcpy(E+fx1, qmmse->ps + fx1, sizeof(float) * (fx2 - fx1));
    sum = 1e-10;
    for (i = fx1; i < fx2; ++i) {
        sum += E[i];
    }
    for (i = fx1; i < fx2; ++i) {
        P1 = E[i] / sum;
    if (P1 >= 0.9) {
        E[i] = 0;
    }
    }
    sum = 0;
    for (i = 0; i < km; ++i) {
        Eb[i] = K;
        Eb[i] += E[i * 4 + 1] + E[i * 4 + 2] + E[i * 4 + 3] + E[i * 4 + 4];
        sum += Eb[i];
    }
    Hb = 0;
    for (i = 0; i < nbin; ++i) {
        prob = (E[i] + K) / sum;
        Hb += -prob * logf(prob + 1e-12);
    }
    // printf("%f\n", Hb);

    return Hb;
}

void wtk_qmmse_agc(wtk_qmmse_t *qmmse, float pframe, wtk_complex_t  *out, float *mask)
{
    int i;
    int N = qmmse->cfg->step;
    float target_gain=300000.f;
    float loudness=1.f;
    float rate;
    float max_increase_step;
    float max_decrease_step;
    float loudness_thresh=qmmse->cfg->loudness_thresh;
    int loudness_frame=qmmse->cfg->loudness_frame;
    float loudness_thresh2=qmmse->cfg->loudness_thresh2;
    int loudness_frame2=qmmse->cfg->loudness_frame2;
    float rate_scale=qmmse->cfg->rate_scale;
    float entropy_thresh=qmmse->cfg->entropy_thresh;
    float entropy=-2;
    float agc_pframe_thresh;
    float agc_mean_mask_thresh = qmmse->cfg->agc_mean_mask_thresh;
    float echo_agc_mean_mask_thresh = qmmse->cfg->echo_agc_mean_mask_thresh;
	int agc_mask_cnt = qmmse->cfg->agc_mask_cnt;
    int echo_agc_mask_cnt = qmmse->cfg->echo_agc_mask_cnt;
    int mask_state = 1;

    if (qmmse->sp_sil) {
        agc_pframe_thresh = qmmse->cfg->agc_pframe_thresh;
    } else {
        agc_pframe_thresh = qmmse->cfg->echo_agc_pframe_thresh;
    }

    if (mask && (agc_mask_cnt > 0 || echo_agc_mask_cnt > 0)) {
        float mean_mask = wtk_float_mean(mask, N);
        if (qmmse->agc_mask_frame > 0) {
            --qmmse->agc_mask_frame;
        }
        if (qmmse->echo_agc_mask_frame > 0) {
            --qmmse->echo_agc_mask_frame;
        }


        if (qmmse->sp_sil) {
            if (agc_mask_cnt > 0) {
                if (mean_mask > agc_mean_mask_thresh) {
                    qmmse->agc_mask_frame = agc_mask_cnt;
                }
                if (qmmse->agc_mask_frame == 0) {
                    mask_state = 0;
                }
            }
        } else{
            if (echo_agc_mask_cnt > 0) {
                if (mean_mask > echo_agc_mean_mask_thresh) {
                    qmmse->echo_agc_mask_frame = echo_agc_mask_cnt;
                }
                if (qmmse->echo_agc_mask_frame == 0) {
                    mask_state = 0;
                }
            }
        }
    }

    qmmse->out_agc_gain = 1.0;

    if(qmmse->cfg->entropy_thresh!=-1){
        entropy = wtk_qmmse_entropy(qmmse);
    }
    // 计算单帧加权能量
    for (i=2;i<N;i++)
    {
        loudness += 2.f*N*qmmse->ps[i]* qmmse->loudness_weight[i];  // ps为各个频点能量，loudness_weight为各个频点的权重
    }
    loudness=sqrtf(loudness);

    if(loudness_thresh2>0){
        if(loudness < loudness_thresh2){
            qmmse->loudness_frame2 = loudness_frame2;
        }else{
            --qmmse->loudness_frame2;
        }
    }

    if (qmmse->cfg->b_agc_scale != 1.0 && qmmse->loudness_frame2 >= 0) {
        float b_agc_scale = qmmse->cfg->b_agc_scale;
        for(i=1;i<N;++i)
        {
            out[i].a*=b_agc_scale;
            out[i].b*=b_agc_scale;
        }
    }

    if(loudness_thresh>0){
        if(loudness>loudness_thresh){
            qmmse->loudness_frame = loudness_frame;
        }else{
            --qmmse->loudness_frame;
        }
    }

    // 通过语音概率计算动态调整增益，其中loudness和loudness_accum具体含义不明，最终是为了计算出target_gain
    if (pframe>.3f)  // pframe为当前帧的语音概率
    {
        if(qmmse->loudness_frame>0){
            rate = rate_scale*pframe*pframe;
        }else{
            rate = .03f*pframe*pframe;
        }
        qmmse->loudness = (1.f-rate)*qmmse->loudness + (rate)*powf(0.001f*loudness, 5);
        qmmse->loudness_accum = (1.f-rate)*qmmse->loudness_accum + rate*sqrtf(qmmse->loudness);
        if (qmmse->init_max < qmmse->cfg->max_gain && qmmse->nframe > 20) // 计算动态增益上限
        {
            qmmse->init_max *= 1.f + qmmse->cfg->init_max_alpha*pframe*pframe;
        }
        target_gain = 0.001f*qmmse->cfg->agc_level*powf(qmmse->loudness/(1e-4f+qmmse->loudness_accum), qmmse->cfg->loudness_pow);
        if(qmmse->agc_init_frame>0){
            --qmmse->agc_init_frame;
        }
    }

    if(qmmse->agc_init_frame>0){
        max_increase_step = qmmse->cfg->max_init_increase_step;
        max_decrease_step = qmmse->cfg->max_init_decrease_step;
    }else{
        max_increase_step = qmmse->cfg->max_increase_step;
        max_decrease_step = qmmse->cfg->max_decrease_step;
    }
    // 对计算出的增益进行限制
    if ((pframe>0.5f && entropy < entropy_thresh && qmmse->nframe > 20) || target_gain < qmmse->agc_gain)
    {
        if (target_gain > max_increase_step*qmmse->agc_gain) // 增益增长限制
        {
            target_gain = max_increase_step*qmmse->agc_gain;
        }
        if (target_gain < max_decrease_step*qmmse->agc_gain && loudness < 10.f*qmmse->prev_loudness) // 增益减小限制
        {
            target_gain = max_decrease_step*qmmse->agc_gain;
        }
        if (target_gain > qmmse->cfg->max_gain)  // 增益上限限制
        {
            target_gain = qmmse->cfg->max_gain;
        }
        if (target_gain > qmmse->init_max)  // 增益上限限制2
        {
            target_gain = qmmse->init_max;
        }

        qmmse->agc_gain = target_gain;  // 更新增益
    }

    //该部分往下均为后边新增的内容

    // 该部分为下行agc算法，目标是将增益总体控制在某个均衡值，一般不启用
    if(qmmse->cfg->use_down_agc){
        float down_gain;
        float down_scale = qmmse->cfg->down_scale;
        float down_scale_1 = 1.0f-down_scale;
        int down_idx = (int)(qmmse->cfg->down_frame*qmmse->cfg->down_percent);
        float down_thresh = qmmse->cfg->down_thresh;
        down_gain = 0.05f/powf(powf(10, (90.f*powf(loudness, 0.06f)-208)/20.f), 0.5f);
        if(pframe > down_thresh){
            qmmse->down_cnt = qmmse->cfg->down_cnt;
            if(qmmse->down_smooth_f > 0){
                --qmmse->down_smooth_f;
                qmmse->down_eng_frame[qmmse->down_smooth_f] = down_gain;
            }else{
                _quickSort(qmmse->down_eng_frame, 0, qmmse->cfg->down_frame-1);
                qmmse->down_eng = qmmse->down_eng*down_scale + down_scale_1*qmmse->down_eng_frame[down_idx];
                qmmse->down_smooth_f = qmmse->cfg->down_frame;
            }
        }else{
            --qmmse->down_cnt;
        }
        // printf("%f\n", down_gain);
        // printf("%f\n", qmmse->down_eng);
        qmmse->down_eng = max(min(qmmse->down_eng, qmmse->cfg->max_down_gain), qmmse->cfg->min_down_gain);
        down_gain = max(min(down_gain, qmmse->cfg->max_down_gain), qmmse->cfg->min_down_gain);
        qmmse->down_eng = (qmmse->down_eng+down_gain)/2.f;
        qmmse->down_eng = max(min(qmmse->down_eng + (qmmse->down_eng - 0.6) * 0.3, qmmse->cfg->max_down_gain), qmmse->cfg->min_down_gain);
        // printf("%f\n", qmmse->down_eng);
        if(qmmse->cfg->use_agc_smooth){
            if(qmmse->down_cnt >= 0){
                qmmse->agc_gain = qmmse->down_eng;
            }else{
                qmmse->agc_gain = 1.0;
            }
        }else{
            if(qmmse->down_cnt >= 0){
                for (i=1;i<N;i++)
                {
                    out[i].a *= qmmse->down_eng;
                    out[i].b *= qmmse->down_eng;
                }
                qmmse->out_agc_gain *= qmmse->down_eng;
            }
        }
    }
    // 该部分为增益平滑，因为上边计算的agc不太均衡，该部分用于平滑agc的变化，基本原理是将增益后的音量用指数平滑获取一个动态均值，然后所有帧总能量往该均值靠拢
    if(pframe > agc_pframe_thresh && mask_state){
        float out_amp=0;
        if(qmmse->cfg->use_agc_smooth){
            float min_amp = qmmse->tgt_eng * 0.05;
            float scale = qmmse->cfg->tgt_gain_scale;
            float scale_1 = 1.0-scale;
            float smooth_scale = qmmse->cfg->smooth_scale;
            float smooth_scale_1 = 1.0-smooth_scale;
            int smooth_idx = (int)(qmmse->cfg->smooth_frame*qmmse->cfg->smooth_percent);  // 目前能量在统计能量的百分位
            float t_gain;
            for (i=1;i<N;i++)
            {
                out[i].a *= qmmse->agc_gain;
                out[i].b *= qmmse->agc_gain;
                out_amp += (out[i].a * out[i].a + out[i].b * out[i].b)*qmmse->agc_gain; // 计算当前帧增益后的能量
            }
            qmmse->out_agc_gain *= qmmse->agc_gain;
            if(pframe > 0.95f){ // 将语音概率高的能量纳入均值统计
                if(qmmse->init_f > 0){
                    --qmmse->init_f;
                    qmmse->tgt_eng_frame[qmmse->init_f] = out_amp;
                }else if(qmmse->init_f==0){  // 累计到初始化帧数后，将当前目标百分位的能量作为目标能量
                    --qmmse->init_f;
                    _quickSort(qmmse->tgt_eng_frame, 0, qmmse->cfg->smooth_frame-1);
                    qmmse->tgt_eng = qmmse->tgt_eng_frame[smooth_idx];
                }else{
                    if(qmmse->smooth_f > 0){
                        --qmmse->smooth_f;
                        qmmse->tgt_eng_frame[qmmse->smooth_f] = out_amp;
                    }else{  // 动态平滑能量均值
                        _quickSort(qmmse->tgt_eng_frame, 0, qmmse->cfg->smooth_frame-1);
                        qmmse->tgt_eng = qmmse->tgt_eng*smooth_scale+ smooth_scale_1*qmmse->tgt_eng_frame[smooth_idx];
                        qmmse->smooth_f = qmmse->cfg->smooth_frame;
                    }
                }
            }
            // printf("%f\n", qmmse->tgt_eng);
            t_gain = qmmse->tgt_eng/out_amp * scale + scale_1;  // 动态平滑二次增益均值
            if(t_gain > qmmse->cfg->max_smooth_gain){  // 限制平滑增益范围
                t_gain = qmmse->cfg->max_smooth_gain;
            }
            if(t_gain < qmmse->cfg->min_smooth_gain){  // 限制平滑增益范围
                t_gain = qmmse->cfg->min_smooth_gain;
            }
            if(pframe > 0.95f && out_amp > min_amp){  // 对语音概率高的帧进行二次增益
                qmmse->smooth_cnt=20;
            }else{
                --qmmse->smooth_cnt;
            }
            if(qmmse->smooth_cnt>0 && pframe > 0.7f){  // 拖尾衰减，平滑听感
                t_gain = 1.0f + (t_gain-1.0f) * qmmse->smooth_cnt / 20.f;
            }else{
                t_gain = 1.0f;
            }
            if(qmmse->init_f > 0){
                t_gain = 1.0f;
            }
            // printf("%f\n", t_gain);

            for(i=1;i<N;++i){
                out[i].a *= t_gain;
                out[i].b *= t_gain;
            }
            qmmse->out_agc_gain *= t_gain;
        }else if(qmmse->cfg->max_out_gain != -1){ // 限制输出增益，基本弃用
            for (i=1;i<N;i++)
            {
                out[i].a *= qmmse->agc_gain;
                out[i].b *= qmmse->agc_gain;
                out_amp += (out[i].a * out[i].a + out[i].b * out[i].b)*qmmse->agc_gain;
            }
            qmmse->out_agc_gain *= qmmse->agc_gain;
            // printf("%f\n", out_amp);
            if(out_amp > qmmse->cfg->max_out_gain){
                for (i=1;i<N;i++)
                {
                    out[i].a *= qmmse->cfg->max_out_gain/out_amp;
                    out[i].b *= qmmse->cfg->max_out_gain/out_amp;
                }
                qmmse->out_agc_gain *= qmmse->cfg->max_out_gain/out_amp;
            }
        }else if(!qmmse->cfg->use_down_agc){
            for (i=1;i<N;i++)
            {
                out[i].a *= qmmse->agc_gain;
                out[i].b *= qmmse->agc_gain;
            }
            qmmse->out_agc_gain *= qmmse->agc_gain;
        }
    }

    // 使用模型mask进行增益调整
    if(qmmse->cfg->use_agc_mask && mask){
        float agc_mask_thresh = qmmse->cfg->agc_mask_thresh;
        float agc_mask_scale = qmmse->cfg->agc_mask_scale;
        float agc_mask_scale2 = qmmse->cfg->agc_mask_scale2;
        float agc_mask_scale2_1 = 1.0 - agc_mask_scale2;
        float max_gain = qmmse->cfg->max_gain;
        float d_gain = agc_mask_scale*1.0/max_gain;
        for(i=1;i<N;i++){
            if(mask[i]<agc_mask_thresh){
                out[i].a *= d_gain;
                out[i].b *= d_gain;
            }
            out[i].a *= (mask[i] * agc_mask_scale + agc_mask_scale2_1);
            out[i].b *= (mask[i] * agc_mask_scale + agc_mask_scale2_1);
        }
    }

    qmmse->prev_loudness = loudness;
}


void wtk_qmmse_update_noise_prob(wtk_qmmse_t *qmmse)
{
	int N=qmmse->cfg->step;
	float *S=qmmse->S;
	float *ps=qmmse->ps;
	int i;
	int min_range;
	float *Smin=qmmse->Smin;
	float *Stmp=qmmse->Stmp;
	char *update_prob=qmmse->update_prob;
    int max_range=qmmse->cfg->max_range;
    float noise_prob=qmmse->cfg->noise_prob;

    ++qmmse->den_cnt;
	++qmmse->nb_adapt;
    if(qmmse->den_cnt>20000)
    {
        qmmse->den_cnt=20000;
    }
    if(qmmse->nb_adapt>20000)
    {
        qmmse->nb_adapt=20000;
    }

	S[0]=0.8f*S[0]+0.2f*ps[0];
	S[N-1]=0.8f*S[N-1]+0.2f*ps[N-1];
	for(i=1;i<N-1;++i)
	{
		S[i]=0.8f*S[i]+0.05f*ps[i-1]+0.1f*ps[i]+0.05f*ps[i+1];
	}
	if(qmmse->nb_adapt<100)
	{
		min_range=min(15, max_range);
	}else if(qmmse->nb_adapt<1000)
	{
		min_range=min(50, max_range);
	}else if(qmmse->nb_adapt<10000)
	{
		min_range=min(150, max_range);
	}else
	{
		min_range=min(300, max_range);
	}
	if(qmmse->den_cnt>min_range)
	{
		qmmse->den_cnt=0;
		for(i=0;i<N;++i)
		{
			Smin[i]=min(Stmp[i],S[i]);
			Stmp[i]=S[i];
		}
	}else
	{
		for(i=0;i<N;++i)
		{
			Smin[i]=min(Smin[i],S[i]);
			Stmp[i]=min(Stmp[i],S[i]);
		}
	}
	for(i=0;i<N;++i)
	{
		if(noise_prob*S[i]>Smin[i])
		{
			update_prob[i]=1;
		}else
		{
			update_prob[i]=0;
		}
	}
}

void wtk_qmmse_flush(wtk_qmmse_t *qmmse, wtk_complex_t *io, int flush, float *mask)
{
	float *ps=qmmse->ps;
    float *noise=qmmse->noise;
	float *xpost=qmmse->post;
	float *old_ps=qmmse->old_ps;
	float *prior=qmmse->prior;
	float *gain=qmmse->gain;
	float *gain_floor=qmmse->gain_floor;
	float *gain2=qmmse->gain2;
	float beta,beta_1,f,f2,f3;
	int i;
	int N=qmmse->cfg->step;
	int M=qmmse->cfg->nbands;
	int nm=qmmse->nm;
    float min_noise=qmmse->min_noise;
	float *pf;
	float zframe,pframe;
    float noise_floor=expf(0.2302585f*qmmse->cfg->noise_suppress);
    static float fx=2.0f*PI/RAND_MAX;
    float *sym=qmmse->sym;
    float io_alpha=qmmse->cfg->io_alpha;

    if(qmmse->cfg->use_bank)
    {
	    wtk_qmmse_xfilterbank_compute_bank32(qmmse->bank,noise,noise+N);
    }
	if(qmmse->nframe==1)
	{
		memcpy(qmmse->old_ps,qmmse->ps,nm*sizeof(float));
	}
	for(i=1;i<nm;++i)
	{
		f=min_noise+noise[i];
		xpost[i]=ps[i]/f - 1.f;
		if(xpost[i]>100)
		{
			xpost[i]=100;
		}
		f2=old_ps[i]/(old_ps[i]+f);
		beta=0.1f+0.89f*f2*f2;
		prior[i]=beta*max(0,xpost[i])+(1-beta)*old_ps[i]/f;
		if(prior[i]>100.f)
		{
			prior[i]=100.f;
		}
	}
    if(qmmse->cfg->use_bank)
    {
        pf=qmmse->zeta;
        zframe=0;
        for(i=0;i<M;++i)
        {
            zframe+=pf[i]=0.7f*pf[i]+0.3f*prior[i+N];
        }
        pframe=0.1f+0.899f*(1.f/(1.f+0.15f/(zframe/M)));
        for(i=N;i<nm;++i)
        {
            f=prior[i]/(prior[i]+1);
            beta_1=f*(1.f+xpost[i]);
            if(qmmse->cfg->use_logmmse)
            {
                beta=-0.5f*wtk_qmmse_expp(beta_1);
                gain[i]=min(1.f,f*expf(beta));   
            }else
            {
                beta=wtk_qmmse_hypergeom_gain(beta_1);
                gain[i]=min(1.f,f*beta);   
            }
            old_ps[i]=0.2f*old_ps[i]+0.8f*gain[i]*gain[i]*ps[i];
            f2=0.199f+0.8f/(1+0.15f/pf[i-N]);
            f2=1.f-pframe*f2;
            gain2[i]=1.f/(1.f+(f2/(1.f-f2))*(1.f+prior[i])*expf(-beta_1));
        }
        wtk_qmmse_xfilterbank_compute_psd16(qmmse->bank,qmmse->gain2+N,qmmse->gain2);
        wtk_qmmse_xfilterbank_compute_psd16(qmmse->bank,qmmse->gain+N,qmmse->gain);

        wtk_qmmse_compute_gain_floor(qmmse->cfg->noise_suppress,noise+N,qmmse->gain_floor+N,min_noise,M);
        wtk_qmmse_xfilterbank_compute_psd16(qmmse->bank,qmmse->gain_floor+N,qmmse->gain_floor);

    	for(i=1;i<N;++i)
        {
            f=prior[i]/(prior[i]+1.f);
            beta_1=f*(1.f+xpost[i]);
            if(qmmse->cfg->use_logmmse)
            {
                beta=-0.5f*wtk_qmmse_expp(beta_1);
                f2=min(1.f,f*expf(beta)); 
            }else
            {
                beta=wtk_qmmse_hypergeom_gain(beta_1);
                f2=min(1.f,f*beta);
            }
            if(gain[i]<0.333f*f2)
            {
                f2=3.f*gain[i];
            }
            gain[i]=f2;
            old_ps[i]=0.2f*old_ps[i]+0.8f*gain[i]*gain[i]*ps[i];

            if(qmmse->cfg->use_imcra_org)
            {
                gain[i]=powf(gain[i],gain2[i])*powf(gain_floor[i],(1.f-gain2[i]));
            }else
            {
                // wtk_debug("%d %f %f %f\n",i,gain_floor[i],gain[i],gain2[i]);
                gain[i]=max(gain_floor[i],gain[i]);
                f=gain2[i]*sqrtf(gain[i])+(1.f-gain2[i])*sqrtf(gain_floor[i]);
                gain[i]=f*f;   
            }

            // old_ps[i]=0.2f*old_ps[i]+0.8f*gain[i]*gain[i]*ps[i];
        }
    }else
    {
        pf=qmmse->zeta;
        zframe=0;
        for(i=1;i<N-1;++i)
        {
            zframe+=pf[i]=0.7f*pf[i]+0.15f*prior[i]+0.075f*(prior[i-1]+prior[i+1]);
        }
        zframe+=pf[N-1]=0.7f*pf[N-1]+0.3f*prior[N-1];
        pframe=0.1f+0.899f*(1.f/(1.f+0.15f/(zframe/(N-1))));
    	for(i=1;i<N;++i)
        {
            gain_floor[i]=sqrtf(noise_floor*noise[i])/sqrtf(min_noise+noise[i]);

            f=prior[i]/(prior[i]+1);
            beta_1=f*(1+xpost[i]);
            if(qmmse->cfg->use_logmmse)
            {
                beta=-0.5f*wtk_qmmse_expp(beta_1);
                f2=min(1.f,f*expf(beta)); 
            }else
            {
                beta=wtk_qmmse_hypergeom_gain(beta_1);
                f2=min(1.f,f*beta);
            }
            gain[i]=f2;
            // old_ps[i]=0.2*old_ps[i]+0.8*gain[i]*gain[i]*ps[i];
            
            f2=0.199f+0.8f/(1.f+0.15f/pf[i]);
            f2=1-pframe*f2;
            gain2[i]=1.f/(1.f+(f2/(1.f-f2))*(1.f+prior[i])*expf(-beta_1));

            if(qmmse->cfg->use_imcra_org)
            {
                gain[i]=powf(gain[i],gain2[i])*powf(gain_floor[i],(1.f-gain2[i]));
            }else
            {
                gain[i]=max(gain_floor[i],gain[i]);
                f=gain2[i]*sqrtf(gain[i])+(1.f-gain2[i])*sqrtf(gain_floor[i]);
                gain[i]=f*f;   
            }

            old_ps[i]=0.2f*old_ps[i]+0.8f*gain[i]*gain[i]*ps[i];
        }
    }

    qmmse->pframe = pframe;
    if(flush)
    {
        io[0].a=io[0].b=0;
        io[N].a=io[N].b=0;
        for(i=1;i<N;++i)
        {
            io[i].a*=io_alpha * gain[i] + 1.0-io_alpha;
            io[i].b*=io_alpha * gain[i] + 1.0-io_alpha;
        }
        if(qmmse->cfg->use_agc)
        {
            wtk_qmmse_agc(qmmse, pframe, io, mask);
        }
        if(qmmse->cfg->use_cnon)
        {
            for(i=1;i<N;++i)
            {
                f=rand()*fx;

                f3=sqrtf(sym[i]);
                f2=1.f-gain[i]*gain[i];
                if(f2>0)
                {
                    f2=sqrtf(f2);
                    io[i].a+=f3*cos(f)*f2;
                    io[i].b+=f3*sin(f)*f2;
                }
            }
        }
    }
}

void wtk_qmmse_feed_cohv(wtk_qmmse_t *qmmse,wtk_complex_t *io,float *cohv)
{
	float *ps=qmmse->ps;
    float *noise=qmmse->noise;
	float beta,beta_1,f;
	int i;
	int N=qmmse->cfg->step;
    char *update_prob=qmmse->update_prob;

    ++qmmse->nframe;
    if(qmmse->nframe>20000)
	{
		qmmse->nframe=20000;
	}

	wtk_qmmse_analysis(qmmse,io);
    wtk_qmmse_update_noise_prob(qmmse);
    for(i=1;i<N;++i)
    {
        if(update_prob[i] == 0 || ps[i] < noise[i] || cohv[i] <= 0.0f)
        {
            ++qmmse->noise_frame[i];
            if(qmmse->noise_frame[i]>20000)
            {
                qmmse->noise_frame[i]=20000;
            }
            beta=max(qmmse->cfg->beta,1.0f/qmmse->noise_frame[i]);
	        beta_1=1.f-beta;
            f=beta_1 * noise[i] + beta*ps[i];
            if(f<0)
            {
                noise[i]=0;
            }else
            {
                noise[i]=f;
            }
        }
    }
    wtk_qmmse_flush(qmmse, io, 1, NULL);
}

void wtk_qmmse_feed_cohv2(wtk_qmmse_t *qmmse,wtk_complex_t *io,char *cohv)
{
	float *ps=qmmse->ps;
    float *noise=qmmse->noise;
	float beta,beta_1,f;
	int i;
	int N=qmmse->cfg->step;

    ++qmmse->nframe;
    if(qmmse->nframe>20000)
	{
		qmmse->nframe=20000;
	}

	wtk_qmmse_analysis(qmmse,io);
    for(i=1;i<N;++i)
    {
        if(cohv[i] <= 0)
        {
            ++qmmse->noise_frame[i];
            if(qmmse->noise_frame[i]>20000)
            {
                qmmse->noise_frame[i]=20000;
            }
            beta=max(qmmse->cfg->beta,1.0f/qmmse->noise_frame[i]);
	        beta_1=1.f-beta;
            f=beta_1 * noise[i] + beta*ps[i];
            if(f<0)
            {
                noise[i]=0;
            }else
            {
                noise[i]=f;
            }
        }
    }
    wtk_qmmse_flush(qmmse, io, 1, NULL);
}


void wtk_qmmse_feed_mask(wtk_qmmse_t *qmmse,wtk_complex_t *io,float *mask)
{
	float *ps=qmmse->ps;
    float *noise=qmmse->noise;
	float beta,beta_1,f;
	int i;
	int N=qmmse->cfg->step;

    ++qmmse->nframe;
    if(qmmse->nframe>20000)
	{
		qmmse->nframe=20000;
	}

	wtk_qmmse_analysis(qmmse,io);
    // wtk_qmmse_update_noise_prob(qmmse);
    for(i=1;i<N;++i)
    {
        // if(mask[i] > 0)// && (update_prob[i] == 0 || ps[i] < noise[i]))
        {
            ++qmmse->noise_frame[i];
            if(qmmse->noise_frame[i]>20000)
            {
                qmmse->noise_frame[i]=20000;
            }
            beta=max(qmmse->cfg->beta,1.0f/qmmse->noise_frame[i]);
            beta_1=1.f-beta;

            if(mask){
                f=beta_1 * noise[i] + beta*(1-mask[i])*ps[i];
            }else{
                f=beta_1 * noise[i] + beta*ps[i];
            }
            
            if(f<0)
            {
                noise[i]=0;
            }else
            {
                noise[i]=f;
            }
        }
    }
    wtk_qmmse_flush(qmmse, io, 1, mask);
}


void wtk_qmmse_update_mask(wtk_qmmse_t *qmmse,wtk_complex_t *specs,float *mask)
{
	float *ps=qmmse->ps;
    float *noise=qmmse->noise;
	float beta,beta_1,f;
    float *gain=qmmse->gain;
	int i;
	int N=qmmse->cfg->step;

    ++qmmse->nframe;
    if(qmmse->nframe>20000)
	{
		qmmse->nframe=20000;
	}

	wtk_qmmse_analysis(qmmse,specs);
    // wtk_qmmse_update_noise_prob(qmmse);
    for(i=1;i<N;++i)
    {
        // if(mask[i] > 0)// && (update_prob[i] == 0 || ps[i] < noise[i]))
        {
            ++qmmse->noise_frame[i];
            if(qmmse->noise_frame[i]>20000)
            {
                qmmse->noise_frame[i]=20000;
            }
            beta=max(qmmse->cfg->beta,1.0f/qmmse->noise_frame[i]);
            beta_1=1.f-beta;

            f=beta_1 * noise[i] + beta*(1-mask[i])*ps[i];
            
            if(f<0)
            {
                noise[i]=0;
            }else
            {
                noise[i]=f;
            }
        }
    }
    wtk_qmmse_flush(qmmse, specs, 0, NULL);
	for(i=1;i<N;++i)
	{
		mask[i]=gain[i];
	}
}

void wtk_qmmse_flush2(wtk_qmmse_t *qmmse,wtk_complex_t *io, int flush, float *mask)
{
	float *ps=qmmse->ps;
    float *noise=qmmse->noise;
    float *echo_noise=qmmse->echo_noise;
	float *xpost=qmmse->post;
	float *old_ps=qmmse->old_ps;
	float *prior=qmmse->prior;
	float *gain=qmmse->gain;
	float *gain_floor=qmmse->gain_floor;
	float *gain2=qmmse->gain2;
	float beta,beta_1,f,f2,f3;
	int i;
	int N=qmmse->cfg->step;
	int M=qmmse->cfg->nbands;
	int nm=qmmse->nm;
    float min_noise=qmmse->min_noise;
	float *pf;
	float zframe,pframe;
    float echo_suppress;
    float noise_floor;
    float echo_floor;
    static float fx=2.0f*PI/RAND_MAX;
    float *sym=qmmse->sym;
    float io_alpha=qmmse->cfg->io_alpha;

    if(qmmse->cfg->use_bank)
    {
	    wtk_qmmse_xfilterbank_compute_bank32(qmmse->bank,noise,noise+N);
        wtk_qmmse_xfilterbank_compute_bank32(qmmse->bank,echo_noise,echo_noise+N);
    }
	if(qmmse->nframe==1)
	{
		memcpy(qmmse->old_ps,qmmse->ps,nm*sizeof(float));
	}
	for(i=1;i<nm;++i)
	{
		f=min_noise+noise[i]+echo_noise[i];
		xpost[i]=ps[i]/f -1.f;
		if(xpost[i]>100)
		{
			xpost[i]=100;
		}
		f2=old_ps[i]/(old_ps[i]+f);
		beta=0.1f+0.89f*f2*f2;
		prior[i]=beta*max(0,xpost[i])+(1-beta)*old_ps[i]/f;
		if(prior[i]>100.f)
		{
			prior[i]=100.f;
		}
	}
    if(qmmse->cfg->use_bank)
    {
        pf=qmmse->zeta;
        zframe=0;
        for(i=0;i<M;++i)
        {
            zframe+=pf[i]=0.7f*pf[i]+0.3f*prior[i+N];
        }
        pframe=0.1f+0.899f*(1.f/(1.f+0.15f/(zframe/M)));
        for(i=N;i<nm;++i)
        {
            f=prior[i]/(prior[i]+1);
            beta_1=f*(1+xpost[i]);
            if(qmmse->cfg->use_logmmse)
            {
                beta=-0.5f*wtk_qmmse_expp(beta_1);
                gain[i]=min(1.f,f*expf(beta));   
            }else
            {
                beta=wtk_qmmse_hypergeom_gain(beta_1);
                gain[i]=min(1.f,f*beta);   
            }
            old_ps[i]=0.2f*old_ps[i]+0.8f*gain[i]*gain[i]*ps[i];
            f2=0.199f+0.8f/(1.f+0.15f/pf[i-N]);
            f2=1-pframe*f2;
            gain2[i]=1.f/(1.f+(f2/(1.f-f2))*(1.f+prior[i])*expf(-beta_1));
        }
        wtk_qmmse_xfilterbank_compute_psd16(qmmse->bank,qmmse->gain2+N,qmmse->gain2);
        wtk_qmmse_xfilterbank_compute_psd16(qmmse->bank,qmmse->gain+N,qmmse->gain);

        echo_suppress=(1-pframe)*qmmse->cfg->echo_suppress + pframe*qmmse->cfg->echo_suppress_active;
        wtk_qmmse_compute_gain_floor_echo(qmmse->cfg->noise_suppress,echo_suppress,noise+N,echo_noise+N,qmmse->gain_floor+N,min_noise,M);
        wtk_qmmse_xfilterbank_compute_psd16(qmmse->bank,qmmse->gain_floor+N,qmmse->gain_floor);

    	for(i=1;i<N;++i)
        {
            f=prior[i]/(prior[i]+1.f);
            beta_1=f*(1.f+xpost[i]);
            if(qmmse->cfg->use_logmmse)
            {
                beta=-0.5f*wtk_qmmse_expp(beta_1);
                f2=min(1.f,f*expf(beta)); 
            }else
            {
                beta=wtk_qmmse_hypergeom_gain(beta_1);
                f2=min(1.f,f*beta);
            }
            if(gain[i]<0.333f*f2)
            {
                f2=3.f*gain[i];
            }
            gain[i]=f2;
            old_ps[i]=0.2f*old_ps[i]+0.8f*gain[i]*gain[i]*ps[i];

            if(qmmse->cfg->use_imcra_org)
            {
                gain[i]=powf(gain[i],gain2[i])*powf(gain_floor[i],(1.f-gain2[i]));
            }else
            {
                // wtk_debug("%d %f %f %f\n",i,gain_floor[i],gain[i],gain2[i]);
                gain[i]=max(gain_floor[i],gain[i]);
                f=gain2[i]*sqrtf(gain[i])+(1.f-gain2[i])*sqrtf(gain_floor[i]);
                gain[i]=f*f;   
            }

            // old_ps[i]=0.2f*old_ps[i]+0.8f*gain[i]*gain[i]*ps[i];
        }
    }else
    {
        pf=qmmse->zeta;
        zframe=0;
        for(i=1;i<N-1;++i)
        {
            zframe+=pf[i]=0.7f*pf[i]+0.15f*prior[i]+0.075f*(prior[i-1]+prior[i+1]);
        }
        zframe+=pf[N-1]=0.7f*pf[N-1]+0.3f*prior[N-1];
        pframe=0.1f+0.899f*(1.f/(1.f+0.15f/(zframe/(N-1))));

        echo_suppress=(1.f-pframe)*qmmse->cfg->echo_suppress + pframe*qmmse->cfg->echo_suppress_active;
        echo_floor=expf(0.2302585f*echo_suppress);
        noise_floor=expf(0.2302585f*qmmse->cfg->noise_suppress);
    	for(i=1;i<N;++i)
        {
            gain_floor[i]=sqrtf(noise_floor*noise[i]+echo_floor*echo_noise[i])/sqrtf(min_noise+noise[i]+echo_noise[i]);

            f=prior[i]/(prior[i]+1.f);
            beta_1=f*(1.f+xpost[i]);
            if(qmmse->cfg->use_logmmse)
            {
                beta=-0.5f*wtk_qmmse_expp(beta_1);
                f2=min(1.f,f*expf(beta)); 
            }else
            {
                beta=wtk_qmmse_hypergeom_gain(beta_1);
                f2=min(1.f,f*beta);
            }
            gain[i]=f2;
            // old_ps[i]=0.2f*old_ps[i]+0.8f*gain[i]*gain[i]*ps[i];
            
            f2=0.199f+0.8f/(1.f+0.15f/pf[i]);
            f2=1-pframe*f2;
            gain2[i]=1.f/(1.f+(f2/(1.f-f2))*(1.f+prior[i])*expf(-beta_1));

            if(qmmse->cfg->use_imcra_org)
            {
                gain[i]=powf(gain[i],gain2[i])*powf(gain_floor[i],(1.f-gain2[i]));
            }else
            {
                gain[i]=max(gain_floor[i],gain[i]);
                f=gain2[i]*sqrtf(gain[i])+(1.f-gain2[i])*sqrtf(gain_floor[i]);
                gain[i]=f*f;   
            }

            old_ps[i]=0.2f*old_ps[i]+0.8f*gain[i]*gain[i]*ps[i];
        }
    }
    qmmse->pframe = pframe;
    if(flush)
    {
        io[0].a=io[0].b=0;
        io[N].a=io[N].b=0;
        for(i=1;i<N;++i)
        {
            io[i].a*=io_alpha * gain[i] + 1.0-io_alpha;
            io[i].b*=io_alpha * gain[i] + 1.0-io_alpha;
        }
        if(qmmse->cfg->use_agc)
        {
            wtk_qmmse_agc(qmmse, pframe, io, mask);
        }
        if(qmmse->cfg->use_cnon)
        {
            for(i=1;i<N;++i)
            {
                f=rand()*fx;

                f3=sqrtf(sym[i]);
                f2=1.f-gain[i]*gain[i];
                if(f2>0)
                {
                    f2=sqrtf(f2);
                    io[i].a+=f3*cos(f)*f2;
                    io[i].b+=f3*sin(f)*f2;
                }
            }
        }
    }

}


void wtk_qmmse_flush_mask(wtk_qmmse_t *qmmse,wtk_complex_t *io,float *yf)
{
  	float *ps=qmmse->ps;
    float *noise=qmmse->noise;
    float *echo_noise=qmmse->echo_noise;
	float beta,beta_1,f;
	int i;
	int N=qmmse->cfg->step;
    char *update_prob=qmmse->update_prob;
    float echo_alpha=qmmse->cfg->echo_alpha;

    ++qmmse->nframe;
    if(qmmse->nframe>20000)
	{
		qmmse->nframe=20000;
	}

	wtk_qmmse_analysis(qmmse,io);
    wtk_qmmse_update_noise_prob(qmmse);
    for(i=1;i<N;++i)
    {
        if(update_prob[i] == 0 || ps[i] < noise[i])
        {
            ++qmmse->noise_frame[i];
            if(qmmse->noise_frame[i]>20000)
            {
                qmmse->noise_frame[i]=20000;
            }
            beta=max(qmmse->cfg->beta,1.0f/qmmse->noise_frame[i]);
            beta_1=1.f-beta;
            f=beta_1 * noise[i] + beta*ps[i];
            
            if(f<0)
            {
                noise[i]=0;
            }else
            {
                noise[i]=f;
            }
        }
        f=echo_alpha*echo_noise[i];
        echo_noise[i]=max(f,yf[i]);
    }
    wtk_qmmse_flush2(qmmse, io, 0, NULL);
}


void wtk_qmmse_feed_echo_denoise(wtk_qmmse_t *qmmse,wtk_complex_t *io, float *yf)
{
	float *ps=qmmse->ps;
    float *noise=qmmse->noise;
    float *echo_noise=qmmse->echo_noise;
	float beta,beta_1,f;
	int i;
	int N=qmmse->cfg->step;
    char *update_prob=qmmse->update_prob;
    float echo_alpha=qmmse->cfg->echo_alpha;

    ++qmmse->nframe;
    if(qmmse->nframe>20000)
	{
		qmmse->nframe=20000;
	}

	wtk_qmmse_analysis(qmmse,io);
    wtk_qmmse_update_noise_prob(qmmse);
    for(i=1;i<N;++i)
    {
        if(update_prob[i] == 0 || ps[i] < noise[i])
        {
            ++qmmse->noise_frame[i];
            if(qmmse->noise_frame[i]>20000)
            {
                qmmse->noise_frame[i]=20000;
            }
            beta=max(qmmse->cfg->beta,1.0f/qmmse->noise_frame[i]);
            beta_1=1.f-beta;
            f=beta_1 * noise[i] + beta*ps[i];
            
            if(f<0)
            {
                noise[i]=0;
            }else
            {
                noise[i]=f;
            }
        }
        f=echo_alpha*echo_noise[i];
        echo_noise[i]=max(f,yf[i]);
    }
    wtk_qmmse_flush2(qmmse, io, 1, NULL);
}

void wtk_qmmse_feed_echo_denoise2(wtk_qmmse_t *qmmse,wtk_complex_t *io, float *yf, float *mask)
{
    float *ps=qmmse->ps;
    float *noise=qmmse->noise;
    float *echo_noise=qmmse->echo_noise;
	float beta,beta_1,f;
	int i;
	int N=qmmse->cfg->step;
    float echo_alpha=qmmse->cfg->echo_alpha;

    ++qmmse->nframe;
    if(qmmse->nframe>20000)
	{
		qmmse->nframe=20000;
	}

	wtk_qmmse_analysis(qmmse,io);
    for(i=1;i<N;++i)
    {
        ++qmmse->noise_frame[i];
        if(qmmse->noise_frame[i]>20000)
        {
            qmmse->noise_frame[i]=20000;
        }
        beta=max(qmmse->cfg->beta,1.0f/qmmse->noise_frame[i]);
        beta_1=1.f-beta;

        f=beta_1 * noise[i] + beta*(1-mask[i])*ps[i];
        
        if(f<0)
        {
            noise[i]=0;
        }else
        {
            noise[i]=f;
        }
        f=echo_alpha*echo_noise[i];
        echo_noise[i]=max(f,yf[i]);
    }
    wtk_qmmse_flush2(qmmse, io, 1, mask);
}
void wtk_qmmse_feed_echo_denoise4(wtk_qmmse_t *qmmse,wtk_complex_t *io, float *yf, float *mask)
{
	float *ps=qmmse->ps;
    float *noise=qmmse->noise;
    float *echo_noise=qmmse->echo_noise;
	float beta,beta_1,f;
	int i;
	int N=qmmse->cfg->step;
    char *update_prob=qmmse->update_prob;
    float echo_alpha=qmmse->cfg->echo_alpha;

    ++qmmse->nframe;
    if(qmmse->nframe>20000)
	{
		qmmse->nframe=20000;
	}

	wtk_qmmse_analysis(qmmse,io);
    wtk_qmmse_update_noise_prob(qmmse);
    for(i=1;i<N;++i)
    {
        if(update_prob[i] == 0 || ps[i] < noise[i])
        {
            ++qmmse->noise_frame[i];
            if(qmmse->noise_frame[i]>20000)
            {
                qmmse->noise_frame[i]=20000;
            }
            beta=max(qmmse->cfg->beta,1.0f/qmmse->noise_frame[i]);
            beta_1=1.f-beta;
            f=beta_1 * noise[i] + beta*ps[i];
            
            if(f<0)
            {
                noise[i]=0;
            }else
            {
                noise[i]=f;
            }
        }
        f=echo_alpha*echo_noise[i];
        echo_noise[i]=max(f,yf[i]);
    }
    wtk_qmmse_flush2(qmmse, io, 1, mask);
}


void wtk_qmmse_flush_denoise_mask(wtk_qmmse_t *qmmse,wtk_complex_t *io)
{
    float *ps=qmmse->ps;
    float *noise=qmmse->noise;
	float beta,beta_1,f;
	int i;
	int N=qmmse->cfg->step;
    char *update_prob=qmmse->update_prob;

    ++qmmse->nframe;
    if(qmmse->nframe>20000)
	{
		qmmse->nframe=20000;
	}

	wtk_qmmse_analysis(qmmse,io);
    wtk_qmmse_update_noise_prob(qmmse);
    for(i=1;i<N;++i)
    {
        if(update_prob[i] == 0 || ps[i] < noise[i])
        {
            ++qmmse->noise_frame[i];
            if(qmmse->noise_frame[i]>20000)
            {
                qmmse->noise_frame[i]=20000;
            }
            beta=max(qmmse->cfg->beta,1.0f/qmmse->noise_frame[i]);
            beta_1=1.f-beta;

            f=beta_1 * noise[i] + beta*ps[i];
            
            if(f<0)
            {
                noise[i]=0;
            }else
            {
                noise[i]=f;
            }
        }
    }
    wtk_qmmse_flush(qmmse, io, 0, NULL);
}

void wtk_qmmse_denoise(wtk_qmmse_t *qmmse,wtk_complex_t *io)
{
	float *ps=qmmse->ps;
    float *noise=qmmse->noise;
	float beta,beta_1,f;
	int i;
	int N=qmmse->cfg->step;
    char *update_prob=qmmse->update_prob;

    ++qmmse->nframe;
    if(qmmse->nframe>20000)
	{
		qmmse->nframe=20000;
	}

	wtk_qmmse_analysis(qmmse,io);
    wtk_qmmse_update_noise_prob(qmmse);
    for(i=1;i<N;++i)
    {
        if(update_prob[i] == 0 || ps[i] < noise[i])
        {
            ++qmmse->noise_frame[i];
            if(qmmse->noise_frame[i]>20000)
            {
                qmmse->noise_frame[i]=20000;
            }
            beta=max(qmmse->cfg->beta,1.0f/qmmse->noise_frame[i]);
            beta_1=1.f-beta;

            f=beta_1 * noise[i] + beta*ps[i];
            
            if(f<0)
            {
                noise[i]=0;
            }else
            {
                noise[i]=f;
            }
        }
    }
    wtk_qmmse_flush(qmmse, io, 1, NULL);
}


void wtk_qmmse_feed_echo_denoise3(wtk_qmmse_t *qmmse,wtk_complex_t *io, wtk_complex_t *err, int sp_sil)
{
	float *ps=qmmse->ps;
    float *noise=qmmse->noise;
    float *echo_noise=qmmse->echo_noise;
	float beta,beta_1,f;
	int i;
	int N=qmmse->cfg->step;
    char *update_prob=qmmse->update_prob;
    float echo_alpha=qmmse->cfg->echo_alpha;
    float *leak=qmmse->leak;

    if(sp_sil==0){
        wtk_qmmse_sed(qmmse, io, err, leak);
    }

    ++qmmse->nframe;
    if(qmmse->nframe>20000)
	{
		qmmse->nframe=20000;
	}

	wtk_qmmse_analysis(qmmse,io);
    wtk_qmmse_update_noise_prob(qmmse);
    for(i=1;i<N;++i)
    {
        if(update_prob[i] == 0 || ps[i] < noise[i])
        {
            ++qmmse->noise_frame[i];
            if(qmmse->noise_frame[i]>20000)
            {
                qmmse->noise_frame[i]=20000;
            }
            beta=max(qmmse->cfg->beta,1.0f/qmmse->noise_frame[i]);
            beta_1=1.f-beta;
            f=beta_1 * noise[i] + beta*ps[i];
            if(f<0)
            {
                noise[i]=0;
            }else
            {
                noise[i]=f;
            }
        }
        f=echo_alpha*echo_noise[i];
        echo_noise[i]=max(f,leak[i]);
    }
    wtk_qmmse_flush2(qmmse, io, 1, NULL);
}

void wtk_qmmse_flush_echo_mask(wtk_qmmse_t *qmmse,wtk_complex_t *io, wtk_complex_t *err, int sp_sil)
{
	float *ps=qmmse->ps;
    float *noise=qmmse->noise;
    float *echo_noise=qmmse->echo_noise;
	float beta,beta_1,f;
	int i;
	int N=qmmse->cfg->step;
    char *update_prob=qmmse->update_prob;
    float echo_alpha=qmmse->cfg->echo_alpha;
    float *leak=qmmse->leak;

    if(sp_sil==0){
        wtk_qmmse_sed(qmmse, io, err, leak);
    }

    ++qmmse->nframe;
    if(qmmse->nframe>20000)
	{
		qmmse->nframe=20000;
	}

	wtk_qmmse_analysis(qmmse,io);
    wtk_qmmse_update_noise_prob(qmmse);
    for(i=1;i<N;++i)
    {
        if(update_prob[i] == 0 || ps[i] < noise[i])
        {
            ++qmmse->noise_frame[i];
            if(qmmse->noise_frame[i]>20000)
            {
                qmmse->noise_frame[i]=20000;
            }
            beta=max(qmmse->cfg->beta,1.0f/qmmse->noise_frame[i]);
            beta_1=1.f-beta;
            f=beta_1 * noise[i] + beta*ps[i];
            if(f<0)
            {
                noise[i]=0;
            }else
            {
                noise[i]=f;
            }
        }
        f=echo_alpha*echo_noise[i];
        echo_noise[i]=max(f,leak[i]);
    }
    wtk_qmmse_flush2(qmmse, io, 0, NULL);
}

void wtk_qmmse_sed(wtk_qmmse_t *qmmse,wtk_complex_t *io, wtk_complex_t *err, float *leak)
{
    int i;
	int N=qmmse->cfg->step;
    wtk_complex_t lsty;
    float Rf;
    float Yf;

    float *Se=qmmse->Se;
    float *Sd=qmmse->Sd;
    wtk_complex_t *Sed=qmmse->Sed;
    float alpha=qmmse->cfg->sed_alpha;

    for(i=0;i<N;++i){
        // lsty.a = err[i].a - io[i].a;
        // lsty.b = err[i].b - io[i].b;
        lsty.a = err[i].a;
        lsty.b = err[i].b;
        Rf = io[i].a * io[i].a + io[i].b * io[i].b;
        Yf = lsty.a * lsty.a + lsty.b * lsty.b;
        if(qmmse->nframe==0){
            Se[i] += Rf;
            Sd[i] += Yf;
            Sed[i].a += lsty.a*io[i].a+lsty.b*io[i].b;
            Sed[i].b +=-lsty.a*io[i].b+lsty.b*io[i].a;
        }else{
            Se[i] = (1-alpha) * Se[i] + alpha * Rf;
            Sd[i] = (1-alpha) * Sd[i] + alpha * Yf;
            Sed[i].a = (1-alpha) * Sed[i].a + alpha * (lsty.a*io[i].a+lsty.b*io[i].b);
            Sed[i].b = (1-alpha) * Sed[i].b + alpha * (-lsty.a*io[i].b+lsty.b*io[i].a);
        }
        if(qmmse->cfg->use_sed2){
            leak[i] = (Sed[i].a * Sed[i].a + Sed[i].b * Sed[i].b)/(max(Se[i], Sd[i])*Sd[i]+1e-9);
            leak[i] = leak[i] * Yf;
        }else{
            leak[i] = (Sed[i].a * Sed[i].a + Sed[i].b * Sed[i].b)/(Se[i]*Sd[i]+1e-9);
            leak[i] = max(leak[i], 0.005);
        }
    }
}


void wtk_qmmse_set_sp_sil(wtk_qmmse_t *qmmse, int sp_sil)
{
    qmmse->sp_sil = sp_sil;
}