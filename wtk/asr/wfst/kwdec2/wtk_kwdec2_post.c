#include "wtk_kwdec2_post.h"

int wtk_kwdec2_key_id_seek(wtk_kwdec2_t* dec,int out_label)
{
	int key;
	int i;
	key=-1;
	wtk_kwdec2_words_t *set;
	for(i=0;i<dec->cfg->words->nslot;++i)
	{
		set = dec->words_set[i];
		//wtk_debug("k=%d\n",set->key_id);
		if(out_label == set->key_id)
		{
			dec->key_id = key = set->key_id;
			// dec->min_kws = set->min_kws;
			if(dec->idle == 1){
				dec->pdf_conf = set->pdf_conf;
			}else{
				dec->pdf_conf = set->pdf_conf2;
			}
			// wtk_debug("%d\n",key);
			break;
		}
	}
	return key;
}

char* wtk_kwdec2_key_word_seek(wtk_kwdec2_t* dec,int key_id)
{
	char* w=NULL;
	int i;
	wtk_kwdec2_words_t *set;
	for(i=0;i<dec->cfg->words->nslot;++i)
	{
		set = dec->words_set[i];
		//wtk_debug("k=%d ,%d\n",set->key_id,key_id);
		if(key_id == set->key_id)
		{
			w = set->word;
			break;
		}
	}

	return w;
}

int wtk_kwdec2_post_check(wtk_kwdec2_t* dec, label_map_t *m, int in_id_cnt, int kw_cnt)
{
	int i,j,k=0;
	int *pdf_id;
	int phone_count;
	int phone_id=-1;
	float pdf;
//	float *pdf_conf;
	wtk_vector_t *obs;
//	int f_idx;
	int ret=-1;
	int pre_phone_id = -1;
	int cnt =0,phone_cnt=0;
	int sil_cnt=0;
	int post_cnt=0;
	float total_pdf=0;
	// int post_wake_idx[120]={0};
	// int post_phone[120]={0};
	// float post_pdf[120]={0};
	// int post_out_id[120]={0};
	int *post_wake_idx;
	int *post_phone;
	float *post_pdf;
	int *post_out_id;
	post_wake_idx=(int*)wtk_calloc(120,sizeof(int));
	post_phone=(int*)wtk_calloc(120,sizeof(int));
	post_pdf=(float*)wtk_calloc(120,sizeof(float));
	post_out_id=(int*)wtk_calloc(120,sizeof(int));
	char* word=NULL;
	// wtk_debug("==============cur_frame=%d=============\n",dec->cur_frame);
	//	printf("cnt=%d\n",in_id_cnt);
	for(i=0;i<in_id_cnt;++i)
	{
//		wtk_debug("kw=%d\n",kw_cnt);
		if(kw_cnt==0)break;
		if(m[i].out_id == dec->key_id)
		{
			kw_cnt --;
		}
		pdf = 0;
		phone_id = *(dec->trans_model->id2phone_id_+m[i].in_id);
		//wtk_debug("====in_id=%d|out_id=%d|phone_id=%d|feat-idx=%d====\n",m[i].in_id,m[i].out_id,phone_id,m[i].feat_idx);
		//f_idx = m[i].feat_idx;
		if(phone_id == 1)
		{
			sil_cnt++;
			continue;
		}
		if(phone_id == 0) continue;
		//	wtk_debug("sil_cnt=%d,kw_cnt=%d,in_id_cnt=%d\n",sil_cnt,kw_cnt,in_id_cnt);
		pdf_id = dec->trans_model->phone2pdf_id_[phone_id-1];
		phone_count = dec->trans_model->phone_cnt[phone_id-1];
		obs = (wtk_vector_t*)wtk_robin_at(dec->feat_rb,m[i].feat_idx);
        //   wtk_debug("obs=%f |idx=%d\n",obs[0],m[i].feat_idx);
		wtk_softmax(obs, dec->out_col);
//		wtk_vector_print(obs);
//	    printf("phone_cnt=%d\n",phone_count);
		for(j=0;j<phone_count;++j)
		{ 
//	       	printf("pdf_id[%d][%d]=%d ",phone_id,j,pdf_id[j]);
//			wtk_debug("pdf=%f - ",(*(obs+pdf_id[j])));
			pdf += (*(obs+pdf_id[j]));
		}
//		printf("\n");
//
//		wtk_debug("pdf=%f|log_pdf=%f|in_id=%d|out_id=%d|phone_id=%d|feat_id=%d\n",pdf,logf(pdf),m[i].in_id,m[i].out_id,phone_id,m[i].feat_idx);
		pdf = logf(pdf);
			
		post_out_id[k]= m[i].out_id;
		post_phone[k]= phone_id;
		post_pdf[k]= pdf;
		post_wake_idx[k] = m[i].feat_idx;
		k++;
			
	}
	cnt = k-1;
	dec->wake_beg_idx = post_wake_idx[k-1];
	dec->wake_end_idx = post_wake_idx[0];
	// wtk_debug("s=%d,e=%d,s=%d\n",dec->wake_beg_idx,dec->wake_end_idx,start_frame);	
	post_cnt = 0;
	while(post_out_id[k-1]!=dec->key_id)
	{
		post_cnt++;
		k--;
	}
	k=cnt-post_cnt;
	pdf =0;	
	pre_phone_id = post_phone[k];	
	for(i=k;i>=0;--i)
	{
//		wtk_debug("out_id=%d|phone_id=%d|pdf=%f\n", post_out_id[i],post_phone[i],post_pdf[i]);
		if(post_phone[i] == pre_phone_id)
		{
			pdf += post_pdf[i];
			post_pdf[k]=pdf;
			phone_cnt++;
		}else
		{	
			post_pdf[k]/=phone_cnt;
			k--;
			post_phone[k]=post_phone[i];
			phone_cnt = 1;
			pdf = post_pdf[i];
			post_pdf[k]=pdf;
		}
		pre_phone_id = post_phone[i];
	}

	post_pdf[k] /= phone_cnt;

//	wtk_debug("sil_cnt=%d\n",sil_cnt);
/*	if(sil_cnt>20)
	{
		wtk_debug("=============sil break=========\n");
		return -1;
	}*/
    for(j=cnt-post_cnt;j>k-1;--j)
    {
		total_pdf+=post_pdf[j];
		// wtk_debug("log_pdf =%f,pdf=%f,
		// phone=%d\n",post_pdf[j],expf(post_pdf[j]),post_phone[j]);
    }
	//wtk_debug("%f %f %d\n",total_pdf,dec->pdf_conf,dec->key_id);
    if(total_pdf< dec->pdf_conf + dec->cfg->plus_conf) 
	{
        // wtk_debug("total pdf break  f/f=%f,%f\n",total_pdf,dec->pdf_conf);
        // wtk_free(m);
        dec->break_flag = 1;
        if (dec->reset_frame > dec->cfg->reset_time) {
            dec->reset = 1;
		}
        ret= -1;
		goto end;
	}
	ret=0;
	dec->conf = total_pdf;
	word = wtk_kwdec2_key_word_seek(dec,dec->key_id);
	if(word)
	{
		// wtk_debug("=========waked[%s]====total_pdf=%f=========\n",word,total_pdf);
	}
end:
	wtk_free(m);
	wtk_free(post_out_id);
	wtk_free(post_wake_idx);
	wtk_free(post_phone);
	wtk_free(post_pdf);
	return ret;
}

int wtk_kwdec2_post_feed2(wtk_kwdec2_t* dec)
{
    wtk_kwdec2_token_t *cur_token=NULL;
	wtk_kwdec2_pth_t* pth;
    int kw_flag = 0;
    int kw_cnt =0,eps_cnt=0;
	int count=0;
    int ret=-1;
    int i;
    label_map_t *m = NULL;
    int pre_out_label=-1;
    //int in_label[500]={0};
    //int out_label[500]={0};
    //int feat_label[500]={0};
	int *in_label;
	int *out_label;
	int *feat_label;
    int max_fid=0;
    int min_fid=10000;
	int key;
	// int key_opt;
    in_label=(int*)wtk_calloc(500,sizeof(int));
	out_label=(int*)wtk_calloc(500,sizeof(int));
	feat_label=(int*)wtk_calloc(500,sizeof(int));
//  memset(in_label,0,sizeof(in_label));
//  memset(out_label,0,sizeof(out_label));
//  memset(feat_label,0,sizeof(feat_label));	
//  wtk_debug("==============cur_frame=%d=============\n",dec->cur_frame);
//	wtk_debug("flag=%d\n",dec->wake_flag);
    if(dec->best_token)
    {
		cur_token = dec->best_token;
    }
    if(cur_token && cur_token->pth!=NULL)
    {
	    for(pth=cur_token->pth;pth;)
	    {
			if (kw_flag == 0) {
				if ((key = wtk_kwdec2_key_id_seek(dec, pth->out_label)) > 0) {
					kw_flag = 1;
				}
			}
			//wtk_debug("=====out_id=%d=in_id=%d=path_frame=%d key=%d=====\n",pth->out_label,pth->in_label,pth->frame,key);
			if (kw_flag == 1) {
				feat_label[count] = pth->frame;
				if (pth->out_label == key) {
					// wtk_debug("pre_out_label:%d\n",pre_out_label);
					if (pre_out_label == key || pre_out_label == EPS_ID ||
						pre_out_label == DEFAULT_ID) {
						in_label[count] = pth->in_label;
						out_label[count++] = pth->out_label;
						pre_out_label = key;
						kw_cnt++;
					}
				} else if (pth->out_label == EPS_ID) {
					if (pre_out_label == key || pre_out_label == EPS_ID) {
						in_label[count] = pth->in_label;
						out_label[count++] = pth->out_label;
						pre_out_label = EPS_ID;
						eps_cnt++;
					}
				} else {
					kw_flag = 0;
				}
			}
			pth = pth->lbest;
		}
		//dec->key_id = key;
		// for(i=0;i<count;++i)
		// {
		// 	wtk_debug("in_id=%d | out_id=%d | count=%d| kw_cnt= %d\n",in_label[i],out_label[i],count,kw_cnt);
		// }
		if (kw_cnt > KW_MIN_NUM) {
			//wtk_debug("cnt=%d %d\n",eps_cnt,kw_cnt);
			dec->wake_flag++;
		} else {
			dec->wake_flag = 0;
		}
    }
    //wtk_debug("wake_flag=%d,min_kws=%d\n",dec->wake_flag,dec->min_kws);
	if(dec->wake_flag>dec->min_kws)
	{
		//wtk_debug("wake_flag=%d,min_kws=%d\n",dec->wake_flag,dec->min_kws);
	//	dec->wake_flag++;
	//	ret =dec->wake_flag;
		m =(label_map_t*)wtk_calloc(count,sizeof(label_map_t));
		for(i=0;i<count;++i)
		{
			m[i].in_id = in_label[i]; 
			m[i].out_id = out_label[i];
			m[i].feat_idx = feat_label[i];
		}
		    // for(i=0;i<count;++i)
			// {
			// 	wtk_debug("in_id=%d | out_id=%d | feat_id=%d |cnt= %d\n",m[i].in_id,m[i].out_id,m[i].feat_idx,count);
			// }
		for(i=0;i<count;++i)
		{
			if(m[i].out_id==dec->key_id)
			{
				if(max_fid < m[i].feat_idx) max_fid=m[i].feat_idx;
				if(min_fid > m[i].feat_idx) min_fid=m[i].feat_idx;
			}
		}
		
		if(max_fid-min_fid>dec->cfg->sil_frame)
		{
                    //wtk_debug("sil break,max_idx=%d,min_idx=%d,sil_frame=%d\n",max_fid,min_fid,dec->cfg->sil_frame);
                    wtk_free(m);
                    ret = -1;
                    goto end;
		}
		//	ret = 0;
		ret = wtk_kwdec2_post_check(dec,m,count,kw_cnt);
	}else if(dec->wake_flag==0 && dec->reset_frame > dec->cfg->reset_time)
	{
		dec->reset = 1;
		ret = -1;
	}else
	{
		ret = -1;
	}
end:
	wtk_free(in_label);
	wtk_free(out_label);
	wtk_free(feat_label);
	return ret;
}

extern int wtk_kwdec2_post_feed(wtk_kwdec2_t* dec , wtk_vector_t* f)
{
    //wtk_debug("==============cur_frame=%d===f0=%f=========\n",dec->cur_frame,f[0]);
    if (dec->found == 1) {
        return -1;
	}

	dec->reset_frame++;
//	wtk_vector_t * vf;
	wtk_kwdec2_pth_t*pth;
	wtk_kwdec2_token_t *token;
	wtk_fst_state_t *state;
	wtk_queue_node_t *qn;
	float final_cost, cost;
	dec->best_weight = FLT_MAX;
	dec->best_token = NULL;
	int ret = -1;
    wtk_vector_t* tf;
	wtk_robin_t *rb;
	rb = dec->feat_rb;
    tf = (wtk_vector_t*)wtk_robin_next(rb);

    if(dec->cfg->use_fixpoint)
    {
        short *ts;
        int i;
        ts = (short*)f;       
        for(i=0;i<dec->out_col;++i)
        {
            tf[i]=FIX2FLOAT_ANY(ts[i],dec->shift);
        }
    }else
    {
        memcpy(tf,f,sizeof(float)*dec->out_col);
    }
    if(dec->cur_frame < dec->cfg->wdec_post_win_size/4)
    {
            return -1;
    }
    // wtk_debug("%d %d\n",dec->cur_frame,dec->cfg->wdec_post_win_size/4);

    for (qn = dec->cur_tokq->pop; qn; qn = qn->next)
    {
        token = data_offset2(qn,wtk_kwdec2_token_t,q_n);
        state = token->state;
        if(state->type !=WTK_FST_FINAL_STATE)
        {
        //  printf("frame=%d,====not final=====\n",dec->cur_frame);
        }else{
            cost = token->tot_cost;
            final_cost = state->weight;
            cost +=final_cost;
            if(cost < dec->best_weight)
            {
                // wtk_debug("cost=%f\n",cost);
                dec->best_weight = cost;
                dec->best_token = token;
            }
        }
    }

	if(dec->best_token)
	{
            //wtk_debug("final_cost=%f %d\n",dec->best_weight,dec->break_flag);
            if (dec->break_flag) {
                pth = dec->best_token->pth;
                while (pth) {
                    if (pth->out_label == dec->key_id &&
                        pth->frame == dec->wake_beg_idx) {
                        // wtk_debug("wakeup failed\n");
                        ret = -1;
                        goto end;
                    }
                    pth = pth->lbest;
                }
		}

		if(dec->flag == 0){
	 		return -1;
		}
		ret = wtk_kwdec2_post_feed2(dec);
	}
end:
	return ret;

}