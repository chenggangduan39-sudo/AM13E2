#include "wtk_lstmnet.h" 



wtk_lstmnet_node_t *wtk_lstmnet_node_new(int lstm_hidden, wtk_lstmnet_wb_t *wb)
{
    wtk_lstmnet_node_t *lstmn;

    lstmn=(wtk_lstmnet_node_t *)wtk_malloc(sizeof(wtk_lstmnet_node_t));

    lstmn->ft=(float *)wtk_malloc(sizeof(float)*lstm_hidden);
    lstmn->it=(float *)wtk_malloc(sizeof(float)*lstm_hidden);
    lstmn->gt=(float *)wtk_malloc(sizeof(float)*lstm_hidden);
    lstmn->ot=(float *)wtk_malloc(sizeof(float)*lstm_hidden);

    lstmn->h_out=(float *)wtk_malloc(sizeof(float)*lstm_hidden);
    lstmn->c=(float *)wtk_malloc(sizeof(float)*lstm_hidden);

    lstmn->wb=wb;
    lstmn->lstm_hidden=lstm_hidden;

    return lstmn;
}

void wtk_lstmnet_node_reset(wtk_lstmnet_node_t *lstmn)
{    
    memset(lstmn->it,0,sizeof(float)*lstmn->lstm_hidden);
    memset(lstmn->ft,0,sizeof(float)*lstmn->lstm_hidden);
    memset(lstmn->gt,0,sizeof(float)*lstmn->lstm_hidden);
    memset(lstmn->ot,0,sizeof(float)*lstmn->lstm_hidden);

    memset(lstmn->c,0,sizeof(float)*lstmn->lstm_hidden);
    memset(lstmn->h_out,0,sizeof(float)*lstmn->lstm_hidden);
}

void wtk_lstmnet_node_delete(wtk_lstmnet_node_t *lstmn)
{
    wtk_free(lstmn->c);
    wtk_free(lstmn->h_out);
    wtk_free(lstmn->it);
    wtk_free(lstmn->ft);
    wtk_free(lstmn->gt);
    wtk_free(lstmn->ot);
    wtk_free(lstmn);
}

wtk_lstmnet_t *wtk_lstmnet_new(wtk_lstmnet_layer_t *layer, int depth_idx)
{
    wtk_lstmnet_t *lstm;
    wtk_lstmnet_node_t *lstmn;
    int i;

    lstm=(wtk_lstmnet_t *)wtk_malloc(sizeof(wtk_lstmnet_t));
    lstm->layer=layer;

    lstm->depth_idx=depth_idx;

    wtk_queue_init(&(lstm->lstm_q));
    for(i=0;i<layer->lstm_depth;++i)
    {
        lstmn=wtk_lstmnet_node_new(layer->lstm_hidden, layer->lstm_wb+i);
        wtk_queue_push(&(lstm->lstm_q),&(lstmn->q_n));
    }

    lstm->out=(float *)wtk_malloc(sizeof(float)*layer->lstm_hidden);

    lstm->ths=NULL;
    lstm->notify=NULL;
    wtk_lstmnet_reset(lstm);

    return lstm;
}

void wtk_lstmnet_delete(wtk_lstmnet_t *lstm)
{
    wtk_queue_t *lstm_q=&(lstm->lstm_q);
    wtk_queue_node_t *qn;
    wtk_lstmnet_node_t *lstmn;

    while(1)
    {
        qn=wtk_queue_pop(lstm_q);
        if(!qn){break;}
        lstmn=(wtk_lstmnet_node_t *)data_offset2(qn,wtk_lstmnet_node_t,q_n);
        wtk_lstmnet_node_delete(lstmn);
    }

    wtk_free(lstm->out);
    wtk_free(lstm);
}

void wtk_lstmnet_reset(wtk_lstmnet_t *lstm)
{
    wtk_queue_t *lstm_q=&(lstm->lstm_q);
    wtk_queue_node_t *qn;
    wtk_lstmnet_node_t *lstmn;
    int i;

    for(i=0;i<lstm->layer->lstm_depth;++i)
    {
        qn=wtk_queue_peek(lstm_q,i);
        if(!qn){break;}
        lstmn=(wtk_lstmnet_node_t *)data_offset2(qn,wtk_lstmnet_node_t,q_n);
        wtk_lstmnet_node_reset(lstmn);
    }
    memset(lstm->out,0,sizeof(float)*lstm->layer->lstm_hidden);
}

void wtk_lstmnet_sigmoid(float *m,int len)
{
    int i;

	for(i=0;i<len;++i)
	{
		*m=1.0/(1.0+expf(-*m));
		++m;
	}
}

void wtk_lstmnet_relu(float *m,int len)
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

void wtk_lstmnet_prelu(float *m,int len,float w)
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

void wtk_lstmnet_tanh(float *f,int len)
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

void wtk_lstmnet_update_lstm_layer_wb(float *it,float *data, float *h_out,
        float **weii,float *biasi,float **weih,float *biash,int input_idim,int input_hdim,int lstm_hidden)
{
///////////////////////////////////////
    float *weii1,*weih1;
    int i,j;
    int idim_1, hdim_1;
    idim_1 = (int)(input_idim >> 2) << 2;
    hdim_1 = (int)(input_hdim >> 2) << 2;

    float tmp1, tmp2, tmp3, tmp4, tmp5, tmp6;
    for(i=0;i<lstm_hidden;++i,++it,++biasi,++biash)
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

    // for(i=0;i<lstm_hidden;++i)
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
    // }
////////////////////////////////////////
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

static void wtk_neon_lstm_wb(float *it, float *da, float *h, float *wi, float *bi, float *wh, 
    float *bh, float *tmp1, float *tmp2, int idim, int hdim, int lstm)
{
    memset(tmp1,0,sizeof(float)*lstm);
    memset(tmp2,0,sizeof(float)*lstm);
    wtk_math_vecf_muti_matf_transf2_add_vec(tmp1,wi,da,bi,lstm,idim);
    wtk_math_vecf_muti_matf_transf2_add_vec(tmp2,wh,h,bh,lstm,hdim);
    wtk_math_vec_add(it,tmp1,tmp2,lstm);
}
#endif

void wtk_lstmnet_update_lstmlayer_ifgoch(wtk_lstmnet_node_t *lstmn,float *data, int len)
{
    int i;
    int input_idim=lstmn->wb->input_idim;
    int input_hdim=lstmn->wb->input_hdim;
    int lstm_hidden=lstmn->lstm_hidden;
    float *it=lstmn->it;
    float *ft=lstmn->ft;
    float *gt=lstmn->gt;
    float *ot=lstmn->ot;
    float *h_out=lstmn->h_out;
    float *c=lstmn->c;
    float **weii=lstmn->wb->weight_ih_l;
    float *biasi=lstmn->wb->bias_ih_l;
    float **weih=lstmn->wb->weight_hh_l;
    float *biash=lstmn->wb->bias_hh_l;


    memset(it,0,sizeof(float)*lstm_hidden);
    memset(ft,0,sizeof(float)*lstm_hidden);
    memset(gt,0,sizeof(float)*lstm_hidden);
    memset(ot,0,sizeof(float)*lstm_hidden);

#ifdef USE_NEON
    float *wei=lstmn->wb->weight_i_l;
    float *weh=lstmn->wb->weight_h_l;
    float *tmp1=lstmn->wb->lstm_tmp1;
    float *tmp2=lstmn->wb->lstm_tmp2;
    wtk_neon_lstm_wb(it, data, h_out, wei, biasi, weh, biash, tmp1, tmp2, input_idim, input_hdim, lstm_hidden);
    wtk_lstmnet_sigmoid(it,lstm_hidden);
    wtk_neon_lstm_wb(ft, data, h_out, wei+lstm_hidden*input_idim, biasi+lstm_hidden, weh+lstm_hidden*lstm_hidden, biash+lstm_hidden, tmp1, tmp2, input_idim, input_hdim, lstm_hidden);
    wtk_lstmnet_sigmoid(ft,lstm_hidden);
    wtk_neon_lstm_wb(gt, data, h_out, wei+2*lstm_hidden*input_idim, biasi+2*lstm_hidden, weh+2*lstm_hidden*lstm_hidden, biash+2*lstm_hidden, tmp1, tmp2, input_idim, input_hdim, lstm_hidden);
    wtk_lstmnet_tanh(gt,lstm_hidden);
    wtk_neon_lstm_wb(ot, data, h_out, wei+3*lstm_hidden*input_idim, biasi+3*lstm_hidden, weh+3*lstm_hidden*lstm_hidden, biash+3*lstm_hidden, tmp1, tmp2, input_idim, input_hdim, lstm_hidden);
    wtk_lstmnet_sigmoid(ot,lstm_hidden);
#else
    wtk_lstmnet_update_lstm_layer_wb(it, data, h_out, weii, biasi, weih, biash, input_idim, input_hdim, lstm_hidden);
    wtk_lstmnet_sigmoid(it,lstm_hidden);

    wtk_lstmnet_update_lstm_layer_wb(ft, data, h_out, weii+lstm_hidden, biasi+lstm_hidden, 
                                                    weih+lstm_hidden, biash+lstm_hidden, input_idim, input_hdim, lstm_hidden);
    wtk_lstmnet_sigmoid(ft,lstm_hidden);

    wtk_lstmnet_update_lstm_layer_wb(gt, data, h_out, weii+2*lstm_hidden, biasi+2*lstm_hidden, 
                                                    weih+2*lstm_hidden, biash+2*lstm_hidden, input_idim, input_hdim, lstm_hidden);
    wtk_lstmnet_tanh(gt,lstm_hidden);

    wtk_lstmnet_update_lstm_layer_wb(ot, data, h_out, weii+3*lstm_hidden, biasi+3*lstm_hidden, 
                                                    weih+3*lstm_hidden, biash+3*lstm_hidden, input_idim, input_hdim, lstm_hidden);
    wtk_lstmnet_sigmoid(ot,lstm_hidden);
#endif

    for(i=0;i<lstm_hidden;++i)
    {
        c[i]=ft[i]*c[i]+it[i]*gt[i];
        h_out[i]=c[i];
    }
    wtk_lstmnet_tanh(h_out,lstm_hidden);
    for(i=0;i<lstm_hidden;++i)
    {
        h_out[i]*=ot[i];
    }
}

float *wtk_lstmnet_update_lstmlayer(wtk_lstmnet_t *lstm,float *data,int len,int *depth)
{
    wtk_lstmnet_node_t *lstmn;
    wtk_queue_t *lstm_q=&(lstm->lstm_q);
    wtk_queue_node_t *qn;

    qn=wtk_queue_peek(lstm_q,*depth);
    lstmn=(wtk_lstmnet_node_t *)data_offset2(qn,wtk_lstmnet_node_t,q_n);

    wtk_lstmnet_update_lstmlayer_ifgoch(lstmn, data, len);

    *depth+=1;
    if(*depth==lstm->layer->lstm_depth)
    {
        memcpy(lstm->out, lstmn->h_out, sizeof(float)*lstm->layer->lstm_hidden);
        if(lstm->layer->use_sigmoid)
        {
            wtk_lstmnet_sigmoid(lstm->out,lstm->layer->lstm_hidden);
        }else if(lstm->layer->use_relu)
        {
            wtk_lstmnet_relu(lstm->out,lstm->layer->lstm_hidden);
        }else if(lstm->layer->use_prelu)
        {
            wtk_lstmnet_prelu(lstm->out,lstm->layer->lstm_hidden,lstm->layer->prelu_w);
        }else if(lstm->layer->use_tanh)
        {
            wtk_lstmnet_tanh(lstm->out,lstm->layer->lstm_hidden);
        }

        return lstm->out;
    }else
    {
        return wtk_lstmnet_update_lstmlayer(lstm, lstmn->h_out, lstmn->lstm_hidden , depth);
    }        
}


void wtk_lstmnet_update_lstmlayer_ifgoch3(wtk_lstmnet_node_t *lstmn,float *data, int len)
{
    int i,j;
    int input_idim=lstmn->wb->input_idim;
    int lstm_hidden=lstmn->lstm_hidden;
    int lstm_hiddenx2=lstm_hidden*2;
    int lstm_hiddenx3=lstm_hidden*3;

    float *it=lstmn->it, *tmp_it;
    float *ft=lstmn->ft, *tmp_ft;
    float *gt=lstmn->gt, *tmp_gt;
    float *ot=lstmn->ot, *tmp_ot;
    float *h_out=lstmn->h_out, *tmp_h_out, *tmp_h_out1;
    float *c=lstmn->c, *tmp_c;
    float *tmp_data;

    float **weii=lstmn->wb->weight_ih_l;
    float *biasi=lstmn->wb->bias_ih_l;
    float **weih=lstmn->wb->weight_hh_l;
    float *biash=lstmn->wb->bias_hh_l;

    float *weiii,*weihi;
    float *weiif,*weihf;
    float *weiig,*weihg;
    float *weiio,*weiho;

    float *biasii,*biashi;
    float *biasif,*biashf;
    float *biasig,*biashg;
    float *biasio,*biasho;

    float inv_expx,expx;
    float tmp_it2, tmp_ft2, tmp_gt2, tmp_ot2, tmp_h_out2, tmp_c2;

    float ftmp,ftmp2;

    tmp_it=it;
    tmp_ft=ft;
    tmp_gt=gt;
    tmp_ot=ot;
    tmp_h_out=h_out;
    tmp_c=c;

    biasii=biasi;
    biashi=biash;

    biasif=biasi+lstm_hidden;
    biashf=biash+lstm_hidden;

    biasig=biasi+lstm_hiddenx2;
    biashg=biash+lstm_hiddenx2;

    biasio=biasi+lstm_hiddenx3;
    biasho=biash+lstm_hiddenx3;

    for(i=0;i<lstm_hidden;++i, ++tmp_it, ++tmp_ft, ++tmp_gt, ++tmp_ot, ++tmp_h_out, ++tmp_c, 
                                                                ++biasii, ++biasif, ++biasig, ++biasio, ++biashi, ++biashf, ++biashg, ++biasho)
    {
        tmp_it2=tmp_ft2=tmp_gt2=tmp_ot2=0;

        weiii=weii[i];
        weihi=weih[i];
        
        weiif=weii[i+lstm_hidden];
        weihf=weih[i+lstm_hidden];

        weiig=weii[i+lstm_hiddenx2];
        weihg=weih[i+lstm_hiddenx2];
        
        weiio=weii[i+lstm_hiddenx3];
        weiho=weih[i+lstm_hiddenx3];

        tmp_data=data;
        tmp_h_out1=h_out;
        for(j=0;j<input_idim;++j,++weiii,++weiif,++weiig,++weiio,++tmp_data,
                                                    ++weihi,++weihf,++weihg,++weiho,++tmp_h_out1)
        {
            ftmp=*tmp_data;
            ftmp2=*tmp_h_out1;

            tmp_it2+=*weiii * ftmp +*weihi * ftmp2;

            tmp_ft2+=*weiif * ftmp +*weihf * ftmp2;

            tmp_gt2+=*weiig * ftmp +*weihg * ftmp2;

            tmp_ot2+=*weiio * ftmp +*weiho * ftmp2;
        }
        
        tmp_it2 += *biasii + *biashi;
        tmp_ft2 += *biasif + *biashf;
        tmp_gt2 += *biasig + *biashg;
        tmp_ot2 += *biasio + *biasho;

        tmp_it2=1.0/(1.0+expf(-tmp_it2));
        tmp_ft2=1.0/(1.0+expf(-tmp_ft2));
        if(tmp_gt2>0.0)
        {
            inv_expx=expf(-(tmp_gt2));
            tmp_gt2=-1.0+2.0/(1.0+inv_expx*inv_expx);
        }else
        {
            expx=expf(tmp_gt2);
            tmp_gt2=1.0-2.0/(1.0+expx*expx);
        }
        tmp_ot2=1.0/(1.0+expf(-tmp_ot2));

        // wtk_lstmnet_sigmoid2(tmp_it);
        // wtk_lstmnet_sigmoid2(tmp_ft);
        // wtk_lstmnet_tanh2(tmp_gt);
        // wtk_lstmnet_sigmoid2(tmp_ot);
        
        *tmp_it=tmp_it2;
        *tmp_ft=tmp_ft2;
        *tmp_gt=tmp_gt2;
        *tmp_ot=tmp_ot2;

        tmp_c2=tmp_ft2*(*tmp_c)+tmp_it2* tmp_gt2;
        // *tmp_h_out=*tmp_c;
        // wtk_lstmnet_tanh2(tmp_h_out);
        if(tmp_c2>0.0)
        {
            inv_expx=expf(-tmp_c2);
            tmp_h_out2=-1.0+2.0/(1.0+inv_expx*inv_expx);
        }else
        {
            expx=expf(tmp_c2);
            tmp_h_out2=1.0-2.0/(1.0+expx*expx);
        }
        *tmp_c=tmp_c2;
        *tmp_h_out=tmp_h_out2 * tmp_ot2;
    
    }
}


void wtk_lstmnet_update_lstmlayer_ifgoch2(wtk_lstmnet_node_t *lstmn,float *data, int len)
{
    int i,j;
    int input_idim=lstmn->wb->input_idim;
    int input_hdim=lstmn->wb->input_hdim;
    int lstm_hidden=lstmn->lstm_hidden;
    int lstm_hiddenx2=lstm_hidden*2;
    int lstm_hiddenx3=lstm_hidden*3;

    float *it=lstmn->it, *tmp_it;
    float *ft=lstmn->ft, *tmp_ft;
    float *gt=lstmn->gt, *tmp_gt;
    float *ot=lstmn->ot, *tmp_ot;
    float *h_out=lstmn->h_out, *tmp_h_out, *tmp_h_out1;
    float *c=lstmn->c, *tmp_c;
    float *tmp_data;

    float **weii=lstmn->wb->weight_ih_l;
    float *biasi=lstmn->wb->bias_ih_l;
    float **weih=lstmn->wb->weight_hh_l;
    float *biash=lstmn->wb->bias_hh_l;

    float *weiii,*weihi;
    float *weiif,*weihf;
    float *weiig,*weihg;
    float *weiio,*weiho;

    float *biasii,*biashi;
    float *biasif,*biashf;
    float *biasig,*biashg;
    float *biasio,*biasho;

    float inv_expx,expx;
    float tmp_it2, tmp_ft2, tmp_gt2, tmp_ot2, tmp_h_out2, tmp_c2;

    float tmp;

    tmp_it=it;
    tmp_ft=ft;
    tmp_gt=gt;
    tmp_ot=ot;
    tmp_h_out=h_out;
    tmp_c=c;

    biasii=biasi;
    biashi=biash;

    biasif=biasi+lstm_hidden;
    biashf=biash+lstm_hidden;

    biasig=biasi+lstm_hiddenx2;
    biashg=biash+lstm_hiddenx2;

    biasio=biasi+lstm_hiddenx3;
    biasho=biash+lstm_hiddenx3;

    for(i=0;i<lstm_hidden;++i, ++tmp_it, ++tmp_ft, ++tmp_gt, ++tmp_ot, ++tmp_h_out, ++tmp_c, 
                                                                ++biasii, ++biasif, ++biasig, ++biasio, ++biashi, ++biashf, ++biashg, ++biasho)
    {
        tmp_it2=tmp_ft2=tmp_gt2=tmp_ot2=0;

        weiii=weii[i];
        weihi=weih[i];
        
        weiif=weii[i+lstm_hidden];
        weihf=weih[i+lstm_hidden];

        weiig=weii[i+lstm_hiddenx2];
        weihg=weih[i+lstm_hiddenx2];
        
        weiio=weii[i+lstm_hiddenx3];
        weiho=weih[i+lstm_hiddenx3];

        tmp_data=data;
        tmp_h_out1=h_out;
        for(j=0;j<input_idim;++j,++weiii,++weiif,++weiig,++weiio,++tmp_data)
        {
            tmp=*tmp_data;

            tmp_it2 += *weiii * tmp;

            tmp_ft2 += *weiif * tmp;

            tmp_gt2 += *weiig * tmp;

            tmp_ot2+= *weiio * tmp;
        }

        for(j=0;j<input_hdim;++j,++weihi,++weihf,++weihg,++weiho,++tmp_h_out1)
        {
            tmp=*tmp_h_out1;

            tmp_it2 += *weihi * tmp;

            tmp_ft2 += *weihf * tmp;

            tmp_gt2 +=*weihg * tmp;

            tmp_ot2 +=*weiho * tmp;
        }
        
        tmp_it2 += *biasii + *biashi;
        tmp_ft2 += *biasif + *biashf;
        tmp_gt2 += *biasig + *biashg;
        tmp_ot2 += *biasio + *biasho;

        tmp_it2=1.0/(1.0+expf(-tmp_it2));
        tmp_ft2=1.0/(1.0+expf(-tmp_ft2));
        if(tmp_gt2>0.0)
        {
            inv_expx=expf(-(tmp_gt2));
            tmp_gt2=-1.0+2.0/(1.0+inv_expx*inv_expx);
        }else
        {
            expx=expf(tmp_gt2);
            tmp_gt2=1.0-2.0/(1.0+expx*expx);
        }
        tmp_ot2=1.0/(1.0+expf(-tmp_ot2));

        // wtk_lstmnet_sigmoid2(tmp_it);
        // wtk_lstmnet_sigmoid2(tmp_ft);
        // wtk_lstmnet_tanh2(tmp_gt);
        // wtk_lstmnet_sigmoid2(tmp_ot);
        
        *tmp_it=tmp_it2;
        *tmp_ft=tmp_ft2;
        *tmp_gt=tmp_gt2;
        *tmp_ot=tmp_ot2;

        tmp_c2=tmp_ft2*(*tmp_c)+tmp_it2* tmp_gt2;
        // *tmp_h_out=*tmp_c;
        // wtk_lstmnet_tanh2(tmp_h_out);
        if(tmp_c2>0.0)
        {
            inv_expx=expf(-tmp_c2);
            tmp_h_out2=-1.0+2.0/(1.0+inv_expx*inv_expx);
        }else
        {
            expx=expf(tmp_c2);
            tmp_h_out2=1.0-2.0/(1.0+expx*expx);
        }
        *tmp_c=tmp_c2;
        *tmp_h_out=tmp_h_out2 * tmp_ot2;
    
    }
}

float *wtk_lstmnet_update_lstmlayer2(wtk_lstmnet_t *lstm,float *data,int len)
{
    wtk_lstmnet_node_t *lstmn;
    wtk_queue_t *lstm_q=&(lstm->lstm_q);
    wtk_queue_node_t *qn;
    float *lstm_out=NULL;
    float *input;
    int input_len;
    int depth;
    int lstm_depth=lstm->layer->lstm_depth;


    input=data;
    input_len=len;
    for(depth=0; depth<lstm_depth; ++depth)
    {
        qn=wtk_queue_peek(lstm_q, depth);
        lstmn=(wtk_lstmnet_node_t *)data_offset2(qn,wtk_lstmnet_node_t,q_n);

        if(depth==0)
        {
            wtk_lstmnet_update_lstmlayer_ifgoch2(lstmn, input, input_len);
        }else
        {
            wtk_lstmnet_update_lstmlayer_ifgoch3(lstmn, input, input_len);
        }
        
        lstm_out=lstmn->h_out;
        input=lstmn->h_out;
        input_len=lstmn->lstm_hidden;
    }

    return lstm_out;
}

void wtk_lstmnet_feed(wtk_lstmnet_t *lstm, float *data, int len, int is_end)
{
    int depth=0;
    float *out;

    // int i;
    // for(i=0;i<len;++i)
    // {
    //     printf("%f ",data[i]);
    // }
    // printf("\n");

    out=wtk_lstmnet_update_lstmlayer(lstm,data,len,&depth);
    // out=wtk_lstmnet_update_lstmlayer2(lstm,data,len);
    if(lstm->notify)
    {
        lstm->notify(lstm->ths, lstm->depth_idx, out, lstm->layer->lstm_hidden, is_end);
    }
}

void wtk_lstmnet_set_notify(wtk_lstmnet_t *lstm, void *ths, wtk_lstmnet_notify_f notify)
{
    lstm->notify=notify;
    lstm->ths=ths;
}
