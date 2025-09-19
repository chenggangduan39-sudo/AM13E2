#include "wtk_grunet.h" 



wtk_grunet_node_t *wtk_grunet_node_new(int gru_hidden, wtk_grunet_wb_t *wb)
{
    wtk_grunet_node_t *grun;

    grun=(wtk_grunet_node_t *)wtk_malloc(sizeof(wtk_grunet_node_t));

    grun->rt=(float *)wtk_malloc(sizeof(float)*gru_hidden);
    grun->zt=(float *)wtk_malloc(sizeof(float)*gru_hidden);
    grun->nt=(float *)wtk_malloc(sizeof(float)*gru_hidden);

    grun->h_out=(float *)wtk_malloc(sizeof(float)*gru_hidden);

    grun->wb=wb;
    grun->gru_hidden=gru_hidden;

    return grun;
}

void wtk_grunet_node_reset(wtk_grunet_node_t *grun)
{    
    memset(grun->rt,0,sizeof(float)*grun->gru_hidden);
    memset(grun->zt,0,sizeof(float)*grun->gru_hidden);
    memset(grun->nt,0,sizeof(float)*grun->gru_hidden);

    memset(grun->h_out,0,sizeof(float)*grun->gru_hidden);
}

void wtk_grunet_node_delete(wtk_grunet_node_t *grun)
{
    wtk_free(grun->h_out);
    wtk_free(grun->rt);
    wtk_free(grun->zt);
    wtk_free(grun->nt);

    wtk_free(grun);
}

wtk_grunet_t *wtk_grunet_new(wtk_grunet_layer_t *layer, int depth_idx)
{
    wtk_grunet_t *gru;
    wtk_grunet_node_t *grun;
    int i;

    gru=(wtk_grunet_t *)wtk_malloc(sizeof(wtk_grunet_t));
    gru->layer=layer;

    gru->depth_idx=depth_idx;

    wtk_queue_init(&(gru->gru_q));
    for(i=0;i<layer->gru_depth;++i)
    {
        grun=wtk_grunet_node_new(layer->gru_hidden, layer->gru_wb+i);
        wtk_queue_push(&(gru->gru_q),&(grun->q_n));
    }

    gru->out=(float *)wtk_malloc(sizeof(float)*layer->gru_hidden);

    gru->ths=NULL;
    gru->notify=NULL;
    wtk_grunet_reset(gru);

    return gru;
}

void wtk_grunet_delete(wtk_grunet_t *gru)
{
    wtk_queue_t *gru_q=&(gru->gru_q);
    wtk_queue_node_t *qn;
    wtk_grunet_node_t *grun;

    while(1)
    {
        qn=wtk_queue_pop(gru_q);
        if(!qn){break;}
        grun=(wtk_grunet_node_t *)data_offset2(qn,wtk_grunet_node_t,q_n);
        wtk_grunet_node_delete(grun);
    }

    wtk_free(gru->out);
    wtk_free(gru);
}

void wtk_grunet_reset(wtk_grunet_t *gru)
{
    wtk_queue_t *gru_q=&(gru->gru_q);
    wtk_queue_node_t *qn;
    wtk_grunet_node_t *grun;
    int i;

    for(i=0;i<gru->layer->gru_depth;++i)
    {
        qn=wtk_queue_peek(gru_q,i);
        if(!qn){break;}
        grun=(wtk_grunet_node_t *)data_offset2(qn,wtk_grunet_node_t,q_n);
        wtk_grunet_node_reset(grun);
    }
    memset(gru->out,0,sizeof(float)*gru->layer->gru_hidden);
}

void wtk_grunet_relu(float *m,int len)
{
    int i;
    float tmp;

	for(i=0;i<len;++i)
	{
		tmp=*m;
        *m=tmp>0? tmp:0;
		++m;
	}
}

void wtk_grunet_prelu(float *m,int len,float w)
{
    int i;
    float tmp;

	for(i=0;i<len;++i)
	{
		tmp=*m;
        *m=tmp>0? tmp:tmp*w;
		++m;
	}
}

void wtk_grunet_sigmoid(float *m,int len)
{
    int i;

	for(i=0;i<len;++i)
	{
		*m=1.0/(1.0+expf(-*m));
		++m;
	}
}

void wtk_grunet_tanh(float *f,int len)
{
	float *p;
	float *e;
	float inv_expx,expx;
	p=f;e=p+len;
	while(p<e)
	{
		if(*p>0.0)
		{
			inv_expx=expf(-(*p));
			*p=-1.0+2.0/(1.0+inv_expx*inv_expx);
		}else
		{
			expx=expf(*p);
			*p=1.0-2.0/(1.0+expx*expx);
		}
		++p;
	}
}

void wtk_grunet_update_gru_layer_wb_rz(float *it,float *data, float *h_out,
        float **weii,float *biasi,float **weih,float *biash,int input_idim,int input_hdim,int gru_hidden)
{

///////////////////////////////////////
    float *weii1,*weih1;
    int i,j;
    int idim_1, hdim_1;
    idim_1 = (int)(input_idim >> 2) << 2;
    hdim_1 = (int)(input_hdim >> 2) << 2;

    float tmp1, tmp2, tmp3, tmp4, tmp5, tmp6;
    for(i=0;i<gru_hidden;++i,++it,++biasi,++biash)
    {   
        weii1=weii[i];
        weih1=weih[i];
        tmp1=tmp2=tmp3=tmp4=tmp5=0;
        for(j=0;j<idim_1;j += 4)
        {   
            tmp1+=weii1[j]*data[j];
            tmp2+=weii1[j + 1]*data[j + 1];
            tmp3+=weii1[j + 2]*data[j + 2];
            tmp4+=weii1[j + 3]*data[j + 3];
        }
        tmp5 += (tmp1 + tmp2) + (tmp3 + tmp4);
        for(j=idim_1;j<input_idim;++j){
            tmp5+=weii1[j]*data[j];
        }

        tmp1=tmp2=tmp3=tmp4=tmp6=0;
        for(j=0;j<hdim_1;j += 4)
        {
            tmp1+=weih1[j]*h_out[j];
            tmp2+=weih1[j + 1]*h_out[j + 1];
            tmp3+=weih1[j + 2]*h_out[j + 2];
            tmp4+=weih1[j + 3]*h_out[j + 3];
        }
        tmp6 += (tmp1 + tmp2) + (tmp3 + tmp4);
        for(j=hdim_1;j<input_hdim;++j){
            tmp6+=weih1[j]*h_out[j];
        }

        *it+=(*biasi+*biash) + (tmp5 + tmp6);
    }
////////////////////////////////////////
    // float *weii1,*weih1;
    // int i,j;
    // for(i=0;i<gru_hidden;++i)
    // {
    //     weii1=weii[i];
    //     weih1=weih[i];
    //     for(j=0;j<input_idim;++j)
    //     {
    //         it[i]+=weii1[j]*data[j];
    //     }
    //     for(j=0;j<input_hdim;++j)
    //     {
    //         it[i]+=weih1[j]*h_out[j];
    //     }
    //     it[i]+=biasi[i]+biash[i];
    //     // it[i]+=biasi[i];
    // }
////////////////////////////////////////
}

void wtk_grunet_update_gru_layer_wb_n(float *nt,float *rt,float *data, float *h_out,
        float **weii,float *biasi,float **weih,float *biash,int input_idim,int input_hdim,int gru_hidden)
{

    
///////////////////////////////////////
    float *weii1,*weih1;
    int i,j;
    float tmp;
    int idim_1, hdim_1;
    idim_1 = (int)(input_idim >> 2) << 2;
    hdim_1 = (int)(input_hdim >> 2) << 2;

    float tmp1, tmp2, tmp3, tmp4, tmp5;
    for(i=0;i<gru_hidden;++i,++nt,++biasi,++biash,++rt)
    {
        weii1=weii[i];
        weih1=weih[i];
        tmp1=tmp2=tmp3=tmp4=tmp5=0;
        for(j=0;j<idim_1;j+=4)
        {
            tmp1+=weii1[j]*data[j];
            tmp2+=weii1[j + 1]*data[j + 1];
            tmp3+=weii1[j + 2]*data[j + 2];
            tmp4+=weii1[j + 3]*data[j + 3];
        }
        tmp5 += (tmp1 + tmp2) + (tmp3 + tmp4);
        for(j=idim_1;j<input_idim;++j){
            tmp5+=weii1[j]*data[j];
        }
        tmp=tmp1=tmp2=tmp3=tmp4=0;
        for(j=0;j<hdim_1;j+=4)
        {
            tmp1+=weih1[j]*h_out[j];
            tmp2+=weih1[j + 1]*h_out[j + 1];
            tmp3+=weih1[j + 2]*h_out[j + 2];
            tmp4+=weih1[j + 3]*h_out[j + 3];
        }
        tmp += *biash + (tmp1 + tmp2) + (tmp3 + tmp4);
        for(j=hdim_1;j<input_hdim;++j){
            tmp+=weih1[j]*h_out[j];
        }

        // nt[i]+=biasi[i] + rt[i]*tmp + tmp5;
        *nt+=*biasi + *rt*tmp + tmp5;
    }

////////////////////////////////////////
    // float *weii1,*weih1;
    // int i,j;
    // float tmp;
    // for(i=0;i<gru_hidden;++i)
    // {
    //     weii1=weii[i];
    //     weih1=weih[i];
    //     for(j=0;j<input_idim;++j)
    //     {
    //         nt[i]+=weii1[j]*data[j];
    //     }
    //     tmp=0;
    //     for(j=0;j<input_hdim;++j)
    //     {
    //         tmp+=weih1[j]*h_out[j];
    //     }
    //     tmp+=biash[i];

    //     nt[i]+=biasi[i]+rt[i]*tmp;
    // }
///////////////////////////////////////
}
#ifdef USE_NEON
static void wtk_math_vecf_muti_matf_transf2_add_vec(float *vec_c,float *mat_a,float *vec_b,float *vec_d,int row,int col){
    float32x4_t *a, *b, *d;
    float32_t *b1;
    float32x4_t t_1;
    float32x4_t r_1, r_2, r_3, r_4;
    float tmp[4] = {0,0,0,0};
    float o_1, o_2, o_3, o_4;
    int row_1, col_1;
    int j;

    a = (float32x4_t *)mat_a;
    d = (float32x4_t *)vec_d;
    row_1=row;
    while(row_1>=4){
        col_1=col;
        b = (float32x4_t *)vec_b;
        b1 = (float32_t *)vec_b;
        r_1 = r_2 = r_3 = r_4 = vld1q_f32(tmp);
        while(col_1>=4){
            // r_1 = vaddq_f32(r_1, vmulq_f32(a[0], b[0]));
            // r_2 = vaddq_f32(r_2, vmulq_f32(a[1], b[0]));
            // r_3 = vaddq_f32(r_3, vmulq_f32(a[2], b[0]));
            // r_4 = vaddq_f32(r_4, vmulq_f32(a[3], b[0]));
            r_1 = vmlaq_f32(r_1, a[0], b[0]);
            r_2 = vmlaq_f32(r_2, a[1], b[0]);
            r_3 = vmlaq_f32(r_3, a[2], b[0]);
            r_4 = vmlaq_f32(r_4, a[3], b[0]);
            a+=4;
            b+=1;
            mat_a+=16;
            b1+=4;
            col_1 -= 4;
        }

        o_1 = vgetq_lane_f32(r_1, 0) + vgetq_lane_f32(r_1, 1) + vgetq_lane_f32(r_1, 2) + vgetq_lane_f32(r_1, 3) +  vgetq_lane_f32(d[0], 0);
        o_2 = vgetq_lane_f32(r_2, 0) + vgetq_lane_f32(r_2, 1) + vgetq_lane_f32(r_2, 2) + vgetq_lane_f32(r_2, 3) +  vgetq_lane_f32(d[0], 1);
        o_3 = vgetq_lane_f32(r_3, 0) + vgetq_lane_f32(r_3, 1) + vgetq_lane_f32(r_3, 2) + vgetq_lane_f32(r_3, 3) +  vgetq_lane_f32(d[0], 2);
        o_4 = vgetq_lane_f32(r_4, 0) + vgetq_lane_f32(r_4, 1) + vgetq_lane_f32(r_4, 2) + vgetq_lane_f32(r_4, 3) +  vgetq_lane_f32(d[0], 3);

        while(col_1>0){
            t_1 = vmulq_n_f32(a[0], *b1++);
            o_1 += vgetq_lane_f32(t_1, 0);
            o_2 += vgetq_lane_f32(t_1, 1);
            o_3 += vgetq_lane_f32(t_1, 2);
            o_4 += vgetq_lane_f32(t_1, 3);
            a+=1;
            mat_a+=4;
            --col_1;
        }
        *vec_c++ = o_1;
        *vec_c++ = o_2;
        *vec_c++ = o_3;
        *vec_c++ = o_4;
        vec_d+=4;
        d+=1;
        row_1 -= 4;
    }
    while(row_1>0){
        b1 = (float32_t *)vec_b;
        o_1 = 0;
        for(j=0;j<col;++j,++mat_a,++b1){
            o_1 += *mat_a * *b1;
        }
        *vec_c++ = o_1 + *vec_d++;
        --row_1;
    }
}

static void wtk_math_vec_add(float *vec_c,float *vec_a,float *vec_b,int len){
    float32x4_t *a, *b;
    a = (float32x4_t *)vec_a;
    b = (float32x4_t *)vec_b;
    while(len>=16){
        vst1q_f32(vec_c, vaddq_f32(a[0], b[0]));
        vec_c += 4;
        vst1q_f32(vec_c, vaddq_f32(a[1], b[1]));
        vec_c += 4;
        vst1q_f32(vec_c, vaddq_f32(a[2], b[2]));
        vec_c += 4;
        vst1q_f32(vec_c, vaddq_f32(a[3], b[3]));
        vec_c += 4;
        a+=4;
        b+=4;
        vec_a+=16;
        vec_b+=16;
        len -= 16;
    }
    if(len>=8){
        vst1q_f32(vec_c, vaddq_f32(a[0], b[0]));
        vec_c += 4;
        vst1q_f32(vec_c, vaddq_f32(a[1], b[1]));
        vec_c += 4;
        a+=2;
        b+=2;
        vec_a+=8;
        vec_b+=8;
        len -= 8;
    }
    if(len>=4){
        vst1q_f32(vec_c, vaddq_f32(a[0], b[0]));
        vec_c += 4;
        vec_a+=4;
        vec_b+=4;
        len -= 4;
    }
    while(len>0){
        *vec_c++ = *vec_a++ + *vec_b++;
        --len;
    }
}

//c[i]=d[i]+a[i]*b[i]
static void wtk_math_vec_add_mul(float *vec_c,float *vec_a,float *vec_b,float *vec_d,int len){
    float32x4_t *a, *b, *d;
    a = (float32x4_t *)vec_a;
    b = (float32x4_t *)vec_b;
    d = (float32x4_t *)vec_d;
    while(len>=16){
        // vst1q_f32(vec_c, vaddq_f32(vmulq_f32(a[0], b[0]), d[0]));
        vst1q_f32(vec_c, vmlaq_f32(d[0], a[0], b[0]));
        vec_c += 4;
        // vst1q_f32(vec_c, vaddq_f32(vmulq_f32(a[1], b[1]), d[1]));
        vst1q_f32(vec_c, vmlaq_f32(d[1], a[1], b[1]));
        vec_c += 4;
        // vst1q_f32(vec_c, vaddq_f32(vmulq_f32(a[2], b[2]), d[2]));
        vst1q_f32(vec_c, vmlaq_f32(d[2], a[2], b[2]));
        vec_c += 4;
        // vst1q_f32(vec_c, vaddq_f32(vmulq_f32(a[3], b[3]), d[3]));
        vst1q_f32(vec_c, vmlaq_f32(d[3], a[3], b[3]));
        vec_c += 4;
        a+=4;
        b+=4;
        d+=4;
        vec_a+=16;
        vec_b+=16;
        vec_d+=16;
        len -= 16;
    }
    if(len>=8){
        // vst1q_f32(vec_c, vaddq_f32(vmulq_f32(a[0], b[0]), d[0]));
        vst1q_f32(vec_c, vmlaq_f32(d[0], a[0], b[0]));
        vec_c += 4;
        // vst1q_f32(vec_c, vaddq_f32(vmulq_f32(a[1], b[1]), d[1]));
        vst1q_f32(vec_c, vmlaq_f32(d[1], a[1], b[1]));
        vec_c += 4;
        a+=2;
        b+=2;
        d+=2;
        vec_a+=8;
        vec_b+=8;
        vec_d+=8;
        len -= 8;
    }
    if(len>=4){
        // vst1q_f32(vec_c, vaddq_f32(vmulq_f32(a[0], b[0]), d[0]));
        vst1q_f32(vec_c, vmlaq_f32(d[0], a[0], b[0]));
        vec_c += 4;
        vec_a+=4;
        vec_b+=4;
        vec_d+=4;
        len -= 4;
    }
    while(len>0){
        *vec_c++ = *vec_a++ * *vec_b++ + *vec_d++;
        --len;
    }
}


static void wtk_neon_gru_rz(float *it, float *da, float *h, float *wi, float *bi, float *wh, float *bh, float *tmp1, float *tmp2, int idim, int hdim, int gru)
{
    memset(tmp1,0,sizeof(float)*gru);
    memset(tmp2,0,sizeof(float)*gru);
    wtk_math_vecf_muti_matf_transf2_add_vec(tmp1,wi,da,bi,gru,idim);
    wtk_math_vecf_muti_matf_transf2_add_vec(tmp2,wh,h,bh,gru,hdim);
    wtk_math_vec_add(it,tmp1,tmp2,gru);

}

static void wtk_neon_gru_wb_n(float *nt, float *rt, float *da, float *h, float *wi, float *bi, float *wh, float *bh, float *tmp1, float *tmp2, int idim, int hdim, int gru)
{
    memset(tmp1,0,sizeof(float)*gru);
    memset(tmp2,0,sizeof(float)*gru);
    wtk_math_vecf_muti_matf_transf2_add_vec(tmp1,wi,da,bi,gru,idim);
    wtk_math_vecf_muti_matf_transf2_add_vec(tmp2,wh,h,bh,gru,hdim);
    wtk_math_vec_add_mul(nt,rt,tmp2,tmp1,gru);

}
#endif
void wtk_grunet_update_grulayer_rzn(wtk_grunet_node_t *grun,float *data, int len)
{
    int i;
    int input_idim=grun->wb->input_idim;
    int input_hdim=grun->wb->input_hdim;
    int gru_hidden=grun->gru_hidden;
    float *rt=grun->rt;
    float *zt=grun->zt;
    float *nt=grun->nt;
    float *h_out=grun->h_out;
    float *biasi=grun->wb->bias_ih_l;
    float *biash=grun->wb->bias_hh_l;

    memset(rt,0,sizeof(float)*gru_hidden);
    memset(zt,0,sizeof(float)*gru_hidden);
    memset(nt,0,sizeof(float)*gru_hidden);

#ifdef USE_NEON
    float *wei=grun->wb->weight_i_l;
    float *weh=grun->wb->weight_h_l;
    float *tmp1=grun->wb->gru_tmp1;
    float *tmp2=grun->wb->gru_tmp2;
    wtk_neon_gru_rz(rt, data, h_out, wei, biasi, weh, biash, tmp1, tmp2, input_idim, input_hdim, gru_hidden);
    wtk_grunet_sigmoid(rt,gru_hidden);
    wtk_neon_gru_rz(zt, data, h_out, wei+gru_hidden*input_idim, biasi+gru_hidden, 
                                weh+gru_hidden*gru_hidden, biash+gru_hidden, tmp1, tmp2, input_idim, input_hdim, gru_hidden);
    wtk_grunet_sigmoid(zt,gru_hidden);
    wtk_neon_gru_wb_n(nt, rt, data, h_out, wei+2*gru_hidden*input_idim, biasi+2*gru_hidden, 
                                weh+2*gru_hidden*gru_hidden, biash+2*gru_hidden, tmp1, tmp2, input_idim, input_hdim, gru_hidden);
    wtk_grunet_tanh(nt,gru_hidden);
#else
    float **weii=grun->wb->weight_ih_l;
    float **weih=grun->wb->weight_hh_l;
    wtk_grunet_update_gru_layer_wb_rz(rt, data, h_out, weii, biasi, weih, biash, input_idim, input_hdim, gru_hidden);
    wtk_grunet_sigmoid(rt,gru_hidden);

    wtk_grunet_update_gru_layer_wb_rz(zt, data, h_out, weii+gru_hidden, biasi+gru_hidden, 
                                                    weih+gru_hidden, biash+gru_hidden, input_idim, input_hdim, gru_hidden);
    wtk_grunet_sigmoid(zt,gru_hidden);

    wtk_grunet_update_gru_layer_wb_n(nt, rt, data, h_out, weii+2*gru_hidden, biasi+2*gru_hidden, 
                                                    weih+2*gru_hidden, biash+2*gru_hidden, input_idim, input_hdim, gru_hidden);
    wtk_grunet_tanh(nt,gru_hidden);
#endif

    for(i=0;i<gru_hidden;++i)
    {
        h_out[i]=(1-zt[i])*nt[i]+zt[i]*h_out[i];
    }
}

void wtk_grunet_update_grulayer_rzn2(wtk_grunet_node_t *grun,float *data, int len)
{
    int i, j;
    int input_idim=grun->wb->input_idim;
    int input_hdim=grun->wb->input_hdim;
    int gru_hidden=grun->gru_hidden;
    int gru_hiddenx2=grun->gru_hidden*2;


    float *rt=grun->rt, *tmp_rt;
    float *zt=grun->zt, *tmp_zt;
    float *nt=grun->nt, *tmp_nt;
    float *h_out=grun->h_out, *tmp_h_out1;
    float *tmp_data;

    float **weii=grun->wb->weight_ih_l;
    float *biasi=grun->wb->bias_ih_l;
    float **weih=grun->wb->weight_hh_l;
    float *biash=grun->wb->bias_hh_l;

    float *weiir, *weihr;
    float *weiiz, *weihz;
    float *weiin, *weihn;

    float *biasir, *biashr;
    float *biasiz, *biashz;
    float *biasin, *biashn;

    float inv_expx, expx;
    float tmp_rt2, tmp_zt2, tmp_nt2, tmp_nt3;

    float tmp;

    tmp_rt=rt;
    tmp_zt=zt;
    tmp_nt=nt;

    biasir=biasi;
    biashr=biash;

    biasiz=biasi+gru_hidden;
    biashz=biash+gru_hidden;

    biasin=biasi+gru_hiddenx2;
    biashn=biash+gru_hiddenx2;

    for(i=0;i<gru_hidden;++i,++tmp_rt,++tmp_zt,++tmp_nt,++biasir,++biasiz,++biasin,++biashr,++biashz,++biashn){

        tmp_rt2=tmp_zt2=tmp_nt2=0;

        weiir=weii[i];
        weihr=weih[i];
        weiiz=weii[i+gru_hidden];
        weihz=weih[i+gru_hidden];
        weiin=weii[i+gru_hiddenx2];
        weihn=weih[i+gru_hiddenx2];

        tmp_data=data;
        tmp_h_out1=h_out;

        for(j=0;j<input_idim;++j,++weiir,++weiiz,++weiin,++tmp_data){
            tmp=*tmp_data;
            tmp_rt2 += *weiir * tmp;
            tmp_zt2 += *weiiz * tmp;
            tmp_nt2 += *weiin * tmp;
        }
        tmp_nt3 = 0;
        for(j=0;j<input_hdim;++j,++weihr,++weihz,++weihn,++tmp_h_out1){
            tmp=*tmp_h_out1;
            tmp_rt2 += *weihr * tmp;
            tmp_zt2 += *weihz * tmp;
            tmp_nt3 += *weihn * tmp;
        }

        tmp_rt2+=*biasir + *biashr;
        tmp_zt2+=*biasiz + *biashz;
        tmp_rt2=1.0/(1.0+expf(-tmp_rt2));
        tmp_zt2=1.0/(1.0+expf(-tmp_zt2));

        tmp_nt3+=*biashn;

        tmp_nt2+=*biasin + tmp_rt2 * tmp_nt3;
        if(tmp_nt2>0.0)
        {
            inv_expx=expf(-tmp_nt2);
            tmp_nt2=-1.0+2.0/(1.0+inv_expx*inv_expx);
        }else
        {
            expx=expf(tmp_nt2);
            tmp_nt2=1.0-2.0/(1.0+expx*expx);
        }

        *tmp_rt=tmp_rt2;
        *tmp_zt=tmp_zt2;
        *tmp_nt=tmp_nt2;
    }

    for(i=0;i<gru_hidden;++i){
        h_out[i]=(1-zt[i])*nt[i]+zt[i]*h_out[i];
    }
}

float *wtk_grunet_update_grulayer(wtk_grunet_t *gru,float *data,int len,int *depth)
{
    wtk_grunet_node_t *grun;
    wtk_queue_t *gru_q=&(gru->gru_q);
    wtk_queue_node_t *qn;

    qn=wtk_queue_peek(gru_q,*depth);
    grun=(wtk_grunet_node_t *)data_offset2(qn,wtk_grunet_node_t,q_n);

    wtk_grunet_update_grulayer_rzn(grun, data, len);
    // wtk_grunet_update_grulayer_rzn2(grun, data, len);

    *depth+=1;
    if(*depth==gru->layer->gru_depth)
    {
        memcpy(gru->out, grun->h_out, sizeof(float)*gru->layer->gru_hidden);
        if(gru->layer->use_sigmoid)
        {
            wtk_grunet_sigmoid(gru->out,gru->layer->gru_hidden);
        }else if(gru->layer->use_relu)
        {
            wtk_grunet_relu(gru->out,gru->layer->gru_hidden);
        }else if(gru->layer->use_prelu)
        {
            wtk_grunet_prelu(gru->out,gru->layer->gru_hidden,gru->layer->prelu_w);
        }else if(gru->layer->use_tanh)
        {
            wtk_grunet_tanh(gru->out,gru->layer->gru_hidden);
        }

        return gru->out;
    }else
    {
        return wtk_grunet_update_grulayer(gru, grun->h_out, grun->gru_hidden , depth);
    }        
}

void wtk_grunet_feed(wtk_grunet_t *gru, float *data, int len, int is_end)
{
    int depth=0;
    float *out;

    // int i;
    // for(i=0;i<len;++i)
    // {
    //     printf("%f ",data[i]);
    // }
    // printf("\n");

    out=wtk_grunet_update_grulayer(gru,data,len,&depth);
    if(gru->notify)
    {
        gru->notify(gru->ths, gru->depth_idx, out, gru->layer->gru_hidden, is_end);
    }
}

void wtk_grunet_set_notify(wtk_grunet_t *gru, void *ths, wtk_grunet_notify_f notify)
{
    gru->notify=notify;
    gru->ths=ths;
}
