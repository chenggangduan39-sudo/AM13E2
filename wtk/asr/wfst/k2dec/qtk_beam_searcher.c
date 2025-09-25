#include "qtk_beam_searcher.h"

qtk_beam_search_hyp_t* qtk_beam_search_hyp_new(){
    qtk_beam_search_hyp_t* hyp = (qtk_beam_search_hyp_t*)wtk_malloc(sizeof(qtk_beam_search_hyp_t));

    hyp->log_prob = 0.0;
    hyp->ys = wtk_strbuf_new(1024,1);
    wtk_strbuf_push_int(hyp->ys,0,1);
    wtk_strbuf_push_int(hyp->ys,0,1);
    hyp->timestamp = wtk_strbuf_new(2048,1);
    hyp->hw_t = wtk_strbuf_new(2048,1);
    hyp->hw_match_t = wtk_strbuf_new(2048,1);
    hyp->token_score = wtk_strbuf_new(2048,1);
    hyp->context_score = 0.0;
    hyp->am_score = 0.0;
    hyp->acc_score = 0.0;
    hyp->state_id2 = 0;
    hyp->state_id[0] = 0;
    hyp->state_id[1] = -1;
    hyp->state_id[2] = -1;
    hyp->state_id[3] = -1;
    hyp->state_id[4] = -1;
    return hyp;
}

void qtk_beam_search_hyp_reset(qtk_beam_search_hyp_t* hyp){
    hyp->log_prob = 0.0;
    hyp->am_score = 0.0;
    hyp->acc_score = 0.0;
    hyp->context_score = 0.0;
    wtk_strbuf_reset(hyp->ys);
    wtk_strbuf_push_int(hyp->ys,0,1);
    wtk_strbuf_push_int(hyp->ys,0,1);
    wtk_strbuf_reset(hyp->timestamp);
    wtk_strbuf_reset(hyp->hw_t);
    wtk_strbuf_reset(hyp->hw_match_t);
    wtk_strbuf_reset(hyp->token_score);
    hyp->state_id[0] = 0;
    hyp->state_id[1] = -1;
    hyp->state_id[2] = -1;
    hyp->state_id[3] = -1;
    hyp->state_id[4] = -1;
}

void qtk_beam_search_hyp_delete(qtk_beam_search_hyp_t* hyp){
    wtk_strbuf_delete(hyp->ys);
    wtk_strbuf_delete(hyp->timestamp);
    wtk_strbuf_delete(hyp->hw_t);
    wtk_strbuf_delete(hyp->hw_match_t);
    wtk_strbuf_delete(hyp->token_score);
    wtk_free(hyp);
}

qtk_beam_searcher_t* qtk_beam_searcher_new(qtk_k2_wrapper_cfg_t* cfg){
    qtk_beam_searcher_t* searcher = (qtk_beam_searcher_t*)wtk_malloc(sizeof(qtk_beam_searcher_t));
    int i;

    searcher->cfg = cfg;
    searcher->ntopk = cfg->beam;
    if(cfg->use_context){
        searcher->ntopk *= 4;
    }
    searcher->probs = (float*)wtk_malloc(searcher->ntopk * sizeof(float));
    searcher->indexes = (int*)wtk_malloc(searcher->ntopk * sizeof(int));
    searcher->out_col = 5537;

    searcher->toks = (qtk_beam_search_hyp_t**)wtk_calloc(sizeof(qtk_beam_search_hyp_t*),searcher->cfg->beam);
    searcher->ptoks = (qtk_beam_search_hyp_t**)wtk_calloc(sizeof(qtk_beam_search_hyp_t*),searcher->cfg->beam);
    for(i = 0; i < searcher->cfg->beam; i++){
        searcher->toks[i] = qtk_beam_search_hyp_new();
        searcher->ptoks[i] = qtk_beam_search_hyp_new();
    }

    if(cfg->use_context){
        searcher->net = qtk_k2_context_net_new(cfg);
        searcher->ntoks = (qtk_beam_search_hyp_t**)wtk_calloc(sizeof(qtk_beam_search_hyp_t*),searcher->ntopk);
        for(i = 0; i < searcher->ntopk; i++){
            searcher->ntoks[i] = qtk_beam_search_hyp_new();
        }
        searcher->pool = wtk_vpool2_new(sizeof(qtk_beam_searcher_result_t), 10);
    }else{
        searcher->net = NULL;
        searcher->ntoks = NULL;
        searcher->pool = NULL;
    }
    searcher->context_net_ok = 0;
    qtk_beam_searcher_reset(searcher);
    return searcher;
}

int qtk_beam_searcher_start(qtk_beam_searcher_t* searcher){
    return 0;
}

void qtk_beam_searcher_topk_indexes(qtk_beam_searcher_t* searcher,float *f, int n){
    float *p = f;
    float *probs = searcher->probs;
    int i,j,*indexes = searcher->indexes,cnt = 0;

    for(i = 0; i < n; i++,p++){
        for(j = 0; j < searcher->ntopk; j++){
            if(*p > *(probs+j)){
                cnt = searcher->ntopk - 1 -j;
                if(cnt >= 0){
                    memmove(probs + j + 1,probs + j,sizeof(float)*cnt);
                    memmove(indexes + j + 1, indexes + j,sizeof(int)*cnt);
                }
                *(probs + j) = *p;
                *(indexes + j) = i;
                break;
            }
        }
    }
}

void qtk_beam_searcher_topk_indexes2(qtk_beam_searcher_t* searcher,float *f, int n){
    float *p = f;
    float *probs = searcher->probs;
    int i,j,*indexes = searcher->indexes,cnt = 0;
    int ntok = searcher->cfg->beam;

    for(i = 0; i < n; i++,p++){
        for(j = 0; j < ntok; j++){
            if(*p > *(probs+j)){
                cnt = ntok - 1 -j;
                if(cnt >= 0){
                    memmove(probs + j + 1,probs + j,sizeof(float)*cnt);
                    memmove(indexes + j + 1, indexes + j,sizeof(int)*cnt);
                }
                *(probs + j) = *p;
                *(indexes + j) = i;
                break;
            }
        }
    }
}

int qtk_beam_searcher_buf_cmp(wtk_strbuf_t *b1,wtk_strbuf_t *b2){
    int j,cnt;
    int *res1,*res2;

    if(b1->pos != b2->pos){
        return 0;
    }

    cnt = b1->pos/sizeof(int);

    res1 = (int*)b1->data;
    res2 = (int*)b2->data;

    for(j=0;j<cnt;j++){
        if(*res1 != *res2){
            return 0;
        }
        res1++;
        res2++;
    }
    return 1;
}

void qtk_beam_searcher_info_debug(wtk_strbuf_t *buf){
    int i;
    int *data = (int*)buf->data;
    printf(" [");
    for(i = 0; i < buf->pos/sizeof(int); i++){
        printf("%d ",*(data + i));
    }
    printf("]");
}

void qtk_beam_searcher_info_debug2(wtk_strbuf_t *buf){
    int i;
    float *data = (float*)buf->data;
    printf(" [");
    for(i = 0; i < buf->pos/sizeof(float); i++){
        printf("%f ",*(data + i));
    }
    printf("]");
}

void qtk_beam_searcher_debug(qtk_beam_searcher_t* wrapper,qtk_beam_search_hyp_t **toks,int x)
{
    int i,cnt,j;
    int *res;
    wtk_debug("==============\n");
    for(i=0;i<x;i++){
        cnt = toks[i]->ys->pos/sizeof(int);
        res = (int*)toks[i]->ys->data;
        printf("[");
        for(j=0;j<cnt;j++){
            printf("%d ",*(res+j));
        }
        printf("] %f %f %f %d %d %d %d %d timestamp:",toks[i]->log_prob,toks[i]->am_score,toks[i]->acc_score,toks[i]->state_id[0],toks[i]->state_id[1],toks[i]->state_id[2],toks[i]->state_id[3],toks[i]->state_id[4]);
        qtk_beam_searcher_info_debug(toks[i]->timestamp);
        printf("hw_t:");
        qtk_beam_searcher_info_debug(toks[i]->hw_t);
        printf("hw_match_t:");
        qtk_beam_searcher_info_debug(toks[i]->hw_match_t);
        printf("token score:");
        qtk_beam_searcher_info_debug2(toks[i]->token_score);//TODO float buf print
        printf("\n");
    }
}

void qtk_beam_search_topk_tok(qtk_beam_searcher_t* searcher,qtk_beam_search_hyp_t **from,qtk_beam_search_hyp_t **to,int beam, int n, int use_aver){
    int i,j;
    qtk_beam_search_hyp_t *hyp,*phyp;
    float probs[16],value = -FLT_MAX;
    int index[4];

    for(i = 0; i < n; i++){
        hyp = from[i];
        if(hyp->ys->pos == 0){
            probs[i] = -FLT_MAX;
        }else{
            if(use_aver){
                probs[i] = hyp->log_prob/(hyp->ys->pos/sizeof(int));
            }else{
                probs[i] = hyp->log_prob;
            }
        }
    }

    for(i = 0; i < beam; i++){
        value = -FLT_MAX;
        for(j = 0; j < n; j++){
            if(i == 0){
                if(probs[j] > value){
                    value = probs[j];
                    index[i] = j;
                }
            }else{
                if(probs[j] > value && probs[j] < probs[index[i-1]]){
                    value = probs[j];
                    index[i] = j;
                }
            }
        }
    }

    for(i = 0; i < beam; i++){
        phyp = to[i];
        j = index[i];
        hyp = from[j];

        wtk_strbuf_reset(phyp->ys);
        wtk_strbuf_reset(phyp->timestamp);
        wtk_strbuf_reset(phyp->hw_t);
        wtk_strbuf_reset(phyp->hw_match_t);
        wtk_strbuf_reset(phyp->token_score);
        wtk_strbuf_push(phyp->ys,hyp->ys->data,hyp->ys->pos);
        wtk_strbuf_push(phyp->timestamp,hyp->timestamp->data,hyp->timestamp->pos);
        wtk_strbuf_push(phyp->hw_t,hyp->hw_t->data,hyp->hw_t->pos);
        wtk_strbuf_push(phyp->hw_match_t,hyp->hw_match_t->data,hyp->hw_match_t->pos);
        wtk_strbuf_push(phyp->token_score,hyp->token_score->data,hyp->token_score->pos);
        phyp->log_prob = hyp->log_prob;
        phyp->acc_score = hyp->acc_score;
        phyp->am_score = hyp->am_score;
        if(searcher->net){
            phyp->context_score = hyp->context_score;
            if(searcher->cfg->keyword_detect){
                memcpy(phyp->state_id,hyp->state_id,sizeof(int)*5);
            }else{
                phyp->state_id2 = hyp->state_id2;
            }
        }
    }
}

int qtk_beam_searcher_feed_context_net(qtk_beam_searcher_t* searcher,float *f, int n,int64_t* dshape,int64_t* dinput){
    float *p = f;
    int i,j,k,ntok = searcher->ntopk,tmpa,tmpb,flag = 0;
    int *indexes = searcher->indexes;
    qtk_beam_search_hyp_t *hyp,*phyp;
    float *probs = searcher->probs;
    float hot_score = 0.0;
    searcher->index++;
    for(i = 0; i < searcher->num_toks; i++){
        for(j = 0; j < searcher->out_col; j++){
            *p += searcher->toks[i]->log_prob;
            p++;
        }
    }
    //wtk_debug("===========\n");
    qtk_beam_searcher_topk_indexes(searcher,f,n);

    //tmp_hyp = searcher->ptoks;
    //searcher->ptoks = searcher->toks;
    //searcher->toks = tmp_hyp;
    if(searcher->num_toks == 1){
        ntok = searcher->cfg->beam;
        for(i = 0; i < ntok; i++){
            hyp = searcher->toks[i];
            if(indexes[i] != 0){
                wtk_strbuf_push_int(hyp->ys,&(indexes[i]),1);
                wtk_strbuf_push_int(hyp->timestamp,&(searcher->index),1);
            }
            hyp->log_prob = probs[i];
            if(searcher->net){
                if(indexes[i] != 0){
                    hyp->state_id2 = qtk_k2_context_net_search(searcher->net,&hot_score,hyp->state_id2,indexes[i]);
                    hyp->context_score = hot_score;
                    hyp->log_prob += hot_score;
                }
            }
        }
    }else{
        for(i = 0; i < ntok; i++){
            hyp = searcher->ntoks[i];
            wtk_strbuf_reset(hyp->timestamp);
            wtk_strbuf_reset(hyp->ys);
            tmpa = indexes[i]/searcher->out_col;
            tmpb = indexes[i]%searcher->out_col;
            phyp = searcher->toks[tmpa];

            wtk_strbuf_push(hyp->ys,phyp->ys->data,phyp->ys->pos);
            wtk_strbuf_push(hyp->timestamp,phyp->timestamp->data,phyp->timestamp->pos);
            hyp->log_prob = probs[i];
            if(searcher->net){
                if(tmpb != 0){
                    hyp->state_id2 = qtk_k2_context_net_search(searcher->net,&hot_score,phyp->state_id2,tmpb);
                    hyp->context_score = hot_score;
                    hyp->log_prob += hot_score;
                }else{
                    hyp->state_id2 = phyp->state_id2;
                    hyp->context_score = phyp->context_score;
                }
            }
            if(tmpb == 0){
                flag = 1;
            }else{
                wtk_strbuf_push_int(hyp->ys,&tmpb,1);
                wtk_strbuf_push_int(hyp->timestamp,&(searcher->index),1);
            }
        }
    }
    //wtk_debug("====111====\n");
    //qtk_beam_searcher_debug(searcher,searcher->ntoks,16);
    if(flag == 1){
        for(i = 0;i < ntok - 1;i++){
            for(j = i+1;j < ntok;j++){
                hyp = searcher->ntoks[i];
                phyp = searcher->ntoks[j];
                if(hyp->ys->pos>0 && qtk_beam_searcher_buf_cmp(hyp->ys,phyp->ys)){
                    wtk_strbuf_reset(phyp->ys);
                    wtk_strbuf_reset(phyp->timestamp);
                    hyp->log_prob = wtk_logaddexp(hyp->log_prob,phyp->log_prob);
                    phyp->log_prob = 0.0;
                }
            }
        }
        for(i = 0;i < ntok - 1;i++){
            hyp = searcher->ntoks[i];
            if(hyp->ys->pos == 0){
                j = i + 1;
                k = i;
                phyp = searcher->ntoks[j];
                while(j < ntok && phyp->ys->pos!=0){
                    wtk_strbuf_push(searcher->ntoks[k]->ys,phyp->ys->data,phyp->ys->pos);
                    wtk_strbuf_push(searcher->ntoks[k]->timestamp,phyp->timestamp->data,phyp->timestamp->pos);
                    wtk_strbuf_reset(phyp->timestamp);
                    wtk_strbuf_reset(phyp->ys);
                    searcher->ntoks[k]->log_prob = phyp->log_prob;
                    searcher->ntoks[k]->state_id2 = phyp->state_id2;
                    searcher->ntoks[k]->context_score = phyp->context_score;
                    j++;
                    k++;
                }
            }
        }
        //wtk_debug("====222====\n");
        //qtk_beam_searcher_debug(searcher,searcher->ntoks,16);
    }

    if(ntok != searcher->cfg->beam){
        qtk_beam_search_topk_tok(searcher,searcher->ntoks,searcher->toks,searcher->cfg->beam,ntok,0);
    }else if(searcher->num_toks != 1){
        for(i = 0;i < ntok; i++){
            hyp = searcher->ntoks[i];
            phyp = searcher->toks[i];

            wtk_strbuf_reset(phyp->ys);
            wtk_strbuf_reset(phyp->timestamp);
            wtk_strbuf_push(phyp->ys,hyp->ys->data,hyp->ys->pos);
            wtk_strbuf_push(phyp->timestamp,hyp->timestamp->data,hyp->timestamp->pos);
            phyp->log_prob = hyp->log_prob;
            if(searcher->net){
                phyp->context_score = hyp->context_score;
                phyp->state_id2 = hyp->state_id2;
            }
        }
    }

    *dshape=0;
    int *v,cnt;
    int64_t *input = dinput;
    for(i = 0;i < searcher->cfg->beam;i++){
        hyp = searcher->toks[i];
        if(hyp->ys->pos != 0){
            v = (int*)hyp->ys->data;
            cnt = hyp->ys->pos/sizeof(int);
            *dshape += 1;
            if(cnt < 2){
                *input = 0;
            }else{
                *input = v[cnt-2];
            }
            input++;
            *input = v[cnt-1];
            input++;
        }
    }
	searcher->num_toks = *dshape;
    //wtk_debug("====333====\n");
    //qtk_beam_searcher_debug(searcher,searcher->toks,4);
    for(i = 0; i < searcher->ntopk; i++){
        *(searcher->indexes + i) = -1;
        *(searcher->probs + i) = -FLT_MAX;
        qtk_beam_search_hyp_reset(searcher->ntoks[i]);
    }

    return 0;
}

int qtk_beam_searcher_feed_normal(qtk_beam_searcher_t* searcher,float *f, int n,int64_t* dshape,int64_t* dinput){
    float *p = f;
    int i,j,k,ntok = searcher->cfg->beam,tmpa,tmpb,flag = 0;
    int *indexes = searcher->indexes;
    qtk_beam_search_hyp_t** tmp_hyp;
    qtk_beam_search_hyp_t *hyp,*phyp;
    float *probs = searcher->probs;
    searcher->index++;
    for(i = 0; i < searcher->num_toks; i++){
        for(j = 0; j < searcher->out_col; j++){
            *p += searcher->toks[i]->log_prob;
            p++;
        }
    }

    qtk_beam_searcher_topk_indexes2(searcher,f,n);

    tmp_hyp = searcher->ptoks;
    searcher->ptoks = searcher->toks;
    searcher->toks = tmp_hyp;
    if(searcher->num_toks == 1){
        for(i = 0; i < ntok; i++){
            hyp = searcher->toks[i];
            if(indexes[i] != 0){
                wtk_strbuf_push_int(hyp->ys,&(indexes[i]),1);
                wtk_strbuf_push_int(hyp->timestamp,&(searcher->index),1);
            }
            hyp->log_prob = probs[i];
        }
    }else{
        for(i = 0; i < ntok; i++){
            hyp = searcher->toks[i];
            wtk_strbuf_reset(hyp->timestamp);
            wtk_strbuf_reset(hyp->ys);
            tmpa = indexes[i]/searcher->out_col;
            tmpb = indexes[i]%searcher->out_col;
            phyp = searcher->ptoks[tmpa];

            wtk_strbuf_push(hyp->ys,phyp->ys->data,phyp->ys->pos);
            wtk_strbuf_push(hyp->timestamp,phyp->timestamp->data,phyp->timestamp->pos);
            hyp->log_prob = probs[i];
            if(tmpb == 0){
                flag = 1;
            }else{
                wtk_strbuf_push_int(hyp->ys,&tmpb,1);
                wtk_strbuf_push_int(hyp->timestamp,&(searcher->index),1);
            }
        }
    }

    if(flag == 1){
        for(i = 0;i < ntok - 1;i++){
            for(j = i+1;j < ntok;j++){
                hyp = searcher->toks[i];
                phyp = searcher->toks[j];
                if(hyp->ys->pos>0 && qtk_beam_searcher_buf_cmp(hyp->ys,phyp->ys)){
                    wtk_strbuf_reset(phyp->ys);
                    wtk_strbuf_reset(phyp->timestamp);
                    hyp->log_prob = wtk_logaddexp(hyp->log_prob,phyp->log_prob);
                    phyp->log_prob = 0.0;
                }
            }
        }
 
        for(i = 0;i < ntok - 1;i++){
            hyp = searcher->toks[i];
            if(hyp->ys->pos == 0){ 
                j = i + 1;
                k = i;
                phyp = searcher->toks[j];
                while(j < ntok && phyp->ys->pos!=0){
                    wtk_strbuf_push(searcher->toks[k]->ys,phyp->ys->data,phyp->ys->pos);
                    wtk_strbuf_push(searcher->toks[k]->timestamp,phyp->timestamp->data,phyp->timestamp->pos);
                    wtk_strbuf_reset(phyp->timestamp);
                    wtk_strbuf_reset(phyp->ys);
                    searcher->toks[k]->log_prob = phyp->log_prob;
                    j++;
                    k++;
                }
            }
        }
    }

    *dshape=0;
    int *v,cnt;
    int64_t *input = dinput;
    for(i = 0;i < ntok;i++){
        hyp = searcher->toks[i];
        if(hyp->ys->pos != 0){
            v = (int*)hyp->ys->data;
            cnt = hyp->ys->pos/sizeof(int);
            *dshape += 1;
            if(cnt < 2){
                *input = 0;
            }else{
                *input = v[cnt-2];
            }
            input++;
            *input = v[cnt-1];
            input++;
        }
    }
    searcher->num_toks = *dshape;

    //qtk_beam_searcher_debug(searcher,searcher->toks);
    for(i = 0; i < ntok; i++){
        *(searcher->indexes + i) = -1;
        *(searcher->probs + i) = -FLT_MAX;
    }
    return 0;
}
int qtk_beam_searcher_feed_keyword(qtk_beam_searcher_t* searcher,float *f, float *f2, int n,int64_t* dshape,int64_t* dinput){
    float *p = f;
    int i,j,k,ntok = searcher->ntopk,tmpa,tmpb,flag = 0,out_id = 0,tmp;
    int *indexes = searcher->indexes;
    qtk_beam_search_hyp_t *hyp,*phyp;
    float *probs = searcher->probs;
    float hot_score = 0.0;
    float am_score = 0.0;
    int *hw;
    searcher->index++;
    for(i = 0; i < searcher->ntopk; i++){
        *(searcher->indexes + i) = -1;
        *(searcher->probs + i) = -FLT_MAX;
        qtk_beam_search_hyp_reset(searcher->ntoks[i]);
    }

    for(i = 0; i < searcher->num_toks; i++){
        for(j = 0; j < searcher->out_col; j++){
            *p += searcher->toks[i]->log_prob;
            p++;
        }
    }
    //wtk_debug("======feed keyword=====\n");
    qtk_beam_searcher_topk_indexes(searcher,f,n);
    //print_float(searcher->probs,16);
    //print_int(searcher->indexes,16);
    //tmp_hyp = searcher->ptoks;
    //searcher->ptoks = searcher->toks;
    //searcher->toks = tmp_hyp;

    if(searcher->num_toks == 1){
        //ntok = searcher->cfg->beam;
        for(i = 0; i < ntok; i++){
            hyp = searcher->ntoks[i];
            if(indexes[i] != 0 && indexes[i] != 2){
                wtk_strbuf_push_int(hyp->ys,&(indexes[i]),1);
                wtk_strbuf_push_int(hyp->timestamp,&(searcher->index),1);
                wtk_strbuf_push_float(hyp->token_score,f2 + indexes[i],1);
            }
            hyp->log_prob = probs[i];
            hyp->am_score = hyp->log_prob;
            if(searcher->net){
                if(indexes[i] != 0 && indexes[i] != 2){
                    qtk_k2_context_net_search_keyword(searcher->net,&hot_score,hyp->state_id,indexes[i],&out_id,hyp->state_id);
                    wtk_strbuf_push_int(hyp->hw_t,&out_id,1);
                    if(hot_score != 0.0)//TODO
                    {
                        tmp = 1;
                        wtk_strbuf_push_int(hyp->hw_match_t,&tmp,1);
                    }else{
                        tmp = 0;
                        wtk_strbuf_push_int(hyp->hw_match_t,&tmp,1);
                    }
                    //wtk_debug("%d %f %d %d\n",hyp->state_id,hot_score,indexes[i],out_id);
                    hyp->context_score = hot_score;
                    hyp->log_prob += hot_score;
                }
            }
        }
    }else{
        for(i = 0; i < ntok; i++){
            hyp = searcher->ntoks[i];
            wtk_strbuf_reset(hyp->hw_t);
            wtk_strbuf_reset(hyp->hw_match_t);
            wtk_strbuf_reset(hyp->timestamp);
            wtk_strbuf_reset(hyp->token_score);
            wtk_strbuf_reset(hyp->ys);
            tmpa = indexes[i]/searcher->out_col;
            tmpb = indexes[i]%searcher->out_col;
            phyp = searcher->toks[tmpa];

            wtk_strbuf_push(hyp->ys,phyp->ys->data,phyp->ys->pos);
            wtk_strbuf_push(hyp->timestamp,phyp->timestamp->data,phyp->timestamp->pos);
            wtk_strbuf_push(hyp->hw_t,phyp->hw_t->data,phyp->hw_t->pos);
            wtk_strbuf_push(hyp->hw_match_t,phyp->hw_match_t->data,phyp->hw_match_t->pos);
            wtk_strbuf_push(hyp->token_score,phyp->token_score->data,phyp->token_score->pos);
            hyp->log_prob = probs[i];
            if(searcher->net){
                am_score = probs[i] - phyp->context_score - phyp->acc_score;
                if(tmpb != 0 && tmpb != 2){
                    qtk_k2_context_net_search_keyword(searcher->net,&hot_score,phyp->state_id,tmpb,&out_id,hyp->state_id);
                    // wtk_debug("%d %d %d %f %f %d %d\n",phyp->state_id,hyp->state_id,indexes[i],out_id,tmpb);
                    // wtk_debug("%d %d\n",phyp->state_id[0],hyp->state_id[0]);
                    // wtk_debug("%d %d\n",phyp->state_id[1],hyp->state_id[1]);
                    // wtk_debug("%d %d\n",phyp->state_id[2],hyp->state_id[2]);
                    // wtk_debug("%d %d\n",phyp->state_id[3],hyp->state_id[3]);
                    // wtk_debug("%d %d\n",phyp->state_id[4],hyp->state_id[4]);

                    hyp->context_score = hot_score;
                    if(hyp->hw_t->pos > 0){
                        hw = (int*)hyp->hw_t->data;
                        int pos = hyp->hw_t->pos/sizeof(int) - 1;
                        if(*(hw + pos) > 0 && phyp->context_score != 0.0 && hot_score <= searcher->net->cfg->hot_gain){//TODO
                            hyp->acc_score = phyp->acc_score + phyp->context_score;
                            //wtk_debug("%f %f\n",phyp->acc_score,phyp->context_score);
                        }else{
                            hyp->acc_score = phyp->acc_score;
                            //wtk_debug("%f %d %f %f\n",phyp->acc_score,*(hw + pos),phyp->context_score,hot_score);
                        }
                    }else{
                        hyp->acc_score = phyp->acc_score;
                    }
                    hyp->log_prob = am_score + hot_score + hyp->acc_score;
                    //wtk_debug("%f %f %f %f %f %f\n",hot_score,am_score,hyp->log_prob,probs[i],phyp->context_score,hyp->acc_score);
                }else{
                    //hyp->state_id[0] = phyp->state_id[0];
                    //hyp->state_id[1] = phyp->state_id[1];
                    memcpy(hyp->state_id,phyp->state_id,sizeof(int)*5);
                    hyp->context_score = phyp->context_score;
                    hyp->acc_score = phyp->acc_score;
                }
                hyp->am_score = am_score;
            }
            if(tmpb == 0 || tmpb == 2){
                flag = 1;
            }else{
                //wtk_debug("%f %f\n",hot_score,phyp->context_score);
                wtk_strbuf_push_int(hyp->ys,&tmpb,1);
                wtk_strbuf_push_int(hyp->timestamp,&(searcher->index),1);
                wtk_strbuf_push_int(hyp->hw_t,&(out_id),1);
                wtk_strbuf_push_float(hyp->token_score,f2 + indexes[i], 1);
                if(hot_score > 0){//phyp->context_score){//TODO
                    tmp = 1;
                    wtk_strbuf_push_int(hyp->hw_match_t,&tmp,1);
                }else{
                    tmp = 0;
                    wtk_strbuf_push_int(hyp->hw_match_t,&tmp,1);
                }
            }
                // wtk_debug("---phyp---\n");
                // qtk_beam_searcher_debug(searcher,&phyp,1);
                // wtk_debug("---hyp---\n");
                // qtk_beam_searcher_debug(searcher,&hyp,1);
        }
    }
    // wtk_debug("====111====\n");
    // qtk_beam_searcher_debug(searcher,searcher->ntoks,16);
    if(flag == 1){
        for(i = 0;i < ntok - 1;i++){
            for(j = i+1;j < ntok;j++){
                hyp = searcher->ntoks[i];
                phyp = searcher->ntoks[j];
                if(hyp->ys->pos>0 && qtk_beam_searcher_buf_cmp(hyp->ys,phyp->ys)){
                    wtk_strbuf_reset(phyp->ys);
                    wtk_strbuf_reset(phyp->timestamp);
                    wtk_strbuf_reset(phyp->token_score);
                    wtk_strbuf_reset(phyp->hw_t);
                    wtk_strbuf_reset(phyp->hw_match_t);
                    hyp->log_prob = wtk_logaddexp(hyp->log_prob,phyp->log_prob);
                    phyp->log_prob = 0.0;
                    hyp->am_score = wtk_logaddexp(hyp->am_score,phyp->am_score);
                    phyp->am_score = 0.0;
                }
            }
        }
        for(i = 0;i < ntok - 1;i++){
            hyp = searcher->ntoks[i];
            if(hyp->ys->pos == 0){
                j = i + 1;
                k = i;
                phyp = searcher->ntoks[j];
                while(j < ntok && phyp->ys->pos!=0){
                    wtk_strbuf_push(searcher->ntoks[k]->ys,phyp->ys->data,phyp->ys->pos);
                    wtk_strbuf_push(searcher->ntoks[k]->timestamp,phyp->timestamp->data,phyp->timestamp->pos);
                    wtk_strbuf_push(searcher->ntoks[k]->token_score,phyp->token_score->data,phyp->token_score->pos);
                    wtk_strbuf_push(searcher->ntoks[k]->hw_t,phyp->hw_t->data,phyp->hw_t->pos);
                    wtk_strbuf_push(searcher->ntoks[k]->hw_match_t,phyp->hw_match_t->data,phyp->hw_match_t->pos);
                    wtk_strbuf_reset(phyp->timestamp);
                    wtk_strbuf_reset(phyp->token_score);
                    wtk_strbuf_reset(phyp->hw_t);
                    wtk_strbuf_reset(phyp->hw_match_t);
                    wtk_strbuf_reset(phyp->ys);
                    searcher->ntoks[k]->log_prob = phyp->log_prob;
                    searcher->ntoks[k]->am_score = phyp->am_score;
                    searcher->ntoks[k]->acc_score = phyp->acc_score;
                    //searcher->ntoks[k]->state_id[0] = phyp->state_id[0];
                    //searcher->ntoks[k]->state_id[1] = phyp->state_id[1];
                    memcpy(searcher->ntoks[k]->state_id,phyp->state_id,sizeof(int)*5);
                    searcher->ntoks[k]->context_score = phyp->context_score;
                    j++;
                    k++;
                }
            }
        }
        // wtk_debug("====222====\n");
        // qtk_beam_searcher_debug(searcher,searcher->ntoks,16);
    }

    if(ntok != searcher->cfg->beam){
        qtk_beam_search_topk_tok(searcher,searcher->ntoks,searcher->toks,searcher->cfg->beam,ntok,1);
    }
    // else if(searcher->num_toks != 1){
    //     for(i = 0;i < ntok; i++){
    //         hyp = searcher->ntoks[i];
    //         phyp = searcher->toks[i];

    //         wtk_strbuf_reset(phyp->ys);
    //         wtk_strbuf_reset(phyp->timestamp);
    //         wtk_strbuf_push(phyp->ys,hyp->ys->data,hyp->ys->pos);
    //         wtk_strbuf_push(phyp->timestamp,hyp->timestamp->data,hyp->timestamp->pos);
    //         phyp->log_prob = hyp->log_prob;
    //         if(searcher->net){
    //             phyp->context_score = hyp->context_score;
    //             phyp->state_id = hyp->state_id;
    //         }
    //     }
    // }

    *dshape=0;
    int *v,cnt;
    int64_t *input = dinput;
    for(i = 0;i < searcher->cfg->beam;i++){
        hyp = searcher->toks[i]; 
        if(hyp->ys->pos != 0){
            v = (int*)hyp->ys->data;
            cnt = hyp->ys->pos/sizeof(int);
            *dshape += 1;
            if(cnt < 2){
                *input = 0;
            }else{
                *input = v[cnt-2];
            }
            input++;
            *input = v[cnt-1];
            input++;
        }
    }
	searcher->num_toks = *dshape;
    // wtk_debug("====333====\n");
    // qtk_beam_searcher_debug(searcher,searcher->ntoks,16);
    //qtk_beam_searcher_debug(searcher,searcher->toks,4);

    return 0;
}

int qtk_beam_searcher_feed(qtk_beam_searcher_t* searcher,float *f, float *f2, int n,int64_t* dshape,int64_t* dinput){
    if(searcher->cfg->keyword_detect){
        return qtk_beam_searcher_feed_keyword(searcher,f,f2,n,dshape,dinput);
    }else if(searcher->context_net_ok == 1){
        return qtk_beam_searcher_feed_context_net(searcher,f,n,dshape,dinput);
    }else{
        return qtk_beam_searcher_feed_normal(searcher,f,n,dshape,dinput);
    }
}

qtk_beam_searcher_result_t* qtk_beam_searcher_result_new(qtk_beam_searcher_t* searcher){
    qtk_beam_searcher_result_t *res = (qtk_beam_searcher_result_t *)wtk_vpool2_pop(searcher->pool);
    wtk_queue_push(&(searcher->keywrd.resq), &(res->q_n));
    res->kw_score_cnt = 0;
    res->tokens_cnt = 0;
    return res;
}

static int _get_sym_cnt(qtk_beam_searcher_t* searcher, int id){
    wtk_fst_sym_t *sym_out = searcher->cfg->sym2;
    wtk_string_t *resv;
    char *s,*e;
    int cnt,num = 0;
    if(searcher->net->last_outid >= 0 && id > searcher->net->last_outid){
        resv = qtk_k2_context_net_get_outsym(searcher->net,id);
    }else{
        resv = sym_out->strs[id];
    }
    s = resv->data;
    e = resv->data + resv->len;
    while (s < e){
        cnt = wtk_utf8_bytes(*s);
        num++;
        s += cnt;
    }
    return num;
}   

void qtk_beam_searcher_keyword_detect2(qtk_beam_searcher_t* searcher){
    int i,j,*data,len,flag,cnt,cnt2,*data2,flag2 = 1;
    float max = -FLT_MAX,av_am,sum,sum2;
    qtk_beam_search_hyp_t *hyp,*best_path = NULL,*ref_path = NULL;
    qtk_beam_searcher_keyword_t *keywrd = &(searcher->keywrd);
    qtk_beam_searcher_result_t *res;
    wtk_queue_init(&keywrd->resq);

    for(i = 0; i < searcher->ntopk; i++){
        hyp = searcher->ntoks[i];
        data = (int*)hyp->hw_t->data;
        len = hyp->hw_t->pos/sizeof(int);
        flag = 0;
        for(j = 0; j < len; j++){
            if(data[j] != 0){
                flag = 1;
                if(!best_path){
                    best_path = hyp;
                }
            }
        }
        if(!flag){
            if(searcher->cfg->use_hc_wakeup && flag2 == 1){
                data2 = (int*)hyp->ys->data;
                len = hyp->ys->pos/sizeof(int);
                for(j = 0; j < len; j++){
                    if(data2[j] == 890){
                        if(len - j > 3){
                            if(data2[j+1] == 492 && data2[j+2] == 1415 && data2[j+3] == 1415){
                                ref_path = hyp;
                                flag2 = 0;
                                break;
                            }
                        }
                    }
                }
            }
            av_am = hyp->am_score/(hyp->ys->pos/sizeof(int));
            if(av_am > max && flag2){
                max = av_am;
                ref_path = hyp;
            }
        }
    }
    keywrd->best_pth = NULL;
    if(best_path){
        // wtk_debug("best path:\n");
        // qtk_beam_searcher_debug(searcher,&best_path,1);
        // if(ref_path){
        //     qtk_beam_searcher_debug(searcher,&ref_path,1);
        // }
        len = best_path->timestamp->pos/sizeof(float);
        if(ref_path){
            keywrd->ref_aver_amprob = ref_path->am_score/(ref_path->ys->pos/sizeof(int));
            keywrd->ref_pth = ref_path;
        }

        float *tmp_score;
        int num = 0;
        data = (int*)best_path->hw_t->data;
        for(j = 0; j < len; j++){
            if(*data > 0){
                res = qtk_beam_searcher_result_new(searcher);
                cnt = cnt2 = 0;
                sum = sum2 = 0.0;
                res->label = *data;
                //tmp_data = (int*)(best_path->hw_match_t->data) + j;
                tmp_score = (float*)(best_path->token_score->data) + j;
                data2 = (int*)best_path->timestamp->data;
                res->end_idx = j;
                num = _get_sym_cnt(searcher,*data);
                res->start_idx = j - num + 1;
                for(i = j; i > j - num; i--){
                    //if(*tmp_data == 1){
                    sum += *tmp_score;
                    cnt += 1;
                    //}
                    //tmp_data--;
                    tmp_score--;
                }
                res->key_avg_prob = sum/cnt;
                res->kw_score_cnt = cnt;
                res->tokens_cnt = len;
                data2 = (int*)best_path->hw_match_t->data;
                tmp_score = (float*)best_path->token_score->data;
                for(i = 0; i < len; i++){
                    if(*data2 == 0){
                        sum2 += *tmp_score;
                        cnt2++;
                    }else if(i >= res->start_idx && i <= res->end_idx){
                        sum2 += *tmp_score;
                        cnt2++;
                    }
                    data2++;
                    tmp_score++;
                }
                res->whole_avg_prob = sum2/cnt2;
                //wtk_debug("%f %f\n",res->whole_avg_prob,res->key_avg_prob);
                //wtk_debug("%d %d %d\n",res->start_idx,res->end_idx,res->label);
            }
            data++;
        }
        //wtk_debug("%f %d %f\n",sum,cnt,keywrd->whole_avg_prob);
        //wtk_debug("%f %d %f\n",sum2,len,keywrd->key_avg_prob);
        //wtk_debug("%d %d %d %f %f\n",end,start,len,ref_path->am_score,best_path->am_score);
        keywrd->timestamp = best_path->timestamp;
        keywrd->aver_amprob = best_path->am_score/(best_path->hw_t->pos/sizeof(int));
        if(flag2 == 0){
            if(ref_path && keywrd->ref_aver_amprob > keywrd->aver_amprob){
                best_path = NULL;
            }
        }
        keywrd->num_toks = len;
        keywrd->best_pth = best_path;
    }
}

void qtk_beam_searcher_keyword_detect(qtk_beam_searcher_t* searcher){
    int i,j,*data,len,flag,cnt,*data2,flag2 = 1,nihao;
    float max2 = -FLT_MAX,av_log;
    float max = -FLT_MAX,av_am,*score,sum,sum2;
    qtk_beam_search_hyp_t *hyp,*best_path = NULL,*ref_path = NULL;
    qtk_beam_searcher_keyword_t *keywrd = &(searcher->keywrd);

    for(i = 0; i < searcher->ntopk; i++){
        hyp = searcher->ntoks[i];
        data = (int*)hyp->hw_t->data;
        data2 = (int*)hyp->timestamp->data;
        len = hyp->hw_t->pos/sizeof(int);
        flag = 0;
        for(j = 0; j < len; j++){
            if(data[j] != 0){
                flag = 1;
                if(!best_path){
                    best_path = hyp;
                    keywrd->keywrd = data2[j];
                }
                av_log = hyp->log_prob/(hyp->ys->pos/sizeof(int));
                if(av_log > max2){
                    max2 = av_log;
                    best_path = hyp;
                    keywrd->keywrd = data2[j];
                }
            }
        }

        if(!flag){
            nihao  = 0;
            if(searcher->cfg->use_hc_wakeup && flag2 == 1){
                data2 = (int*)hyp->ys->data;
                len = hyp->ys->pos/sizeof(int);
                for(j = 0; j < len; j++){
                    if(data2[j] == 890){
                        if(len - j > 3){
                            if(data2[j+1] == 492 && data2[j+2] == 1415 && data2[j+3] == 1415){
                                ref_path = hyp;
                                keywrd->nihao = 1;
                                flag2 = 0;
                                break;
                            }
                        }else if(len - j > 1){
                            if(data2[j+1] == 492){
                                nihao = 1;
                            }
                        }
                    }
                }
            }
            av_am = hyp->am_score/(hyp->ys->pos/sizeof(int));
            if(av_am > max && flag2){
                max = av_am;
                ref_path = hyp;
                keywrd->nihao = nihao;
            }
        }
    }
    keywrd->best_pth = NULL;
    if(best_path){
        // wtk_debug("best path:\n");
        // qtk_beam_searcher_debug(searcher,&best_path,1);
        // if(ref_path){
        //     qtk_beam_searcher_debug(searcher,&ref_path,1);
        // }
        len = best_path->timestamp->pos/sizeof(float);
        if(ref_path){
            keywrd->num_toks = ref_path->ys->pos/sizeof(int) - 2;
            keywrd->ref_aver_amprob = ref_path->am_score/keywrd->num_toks;
            keywrd->ref_pth = ref_path;
        }
        int *tmp_data;
        float *tmp_score;
        data = (int*)best_path->hw_match_t->data;
        data2 = (int*)best_path->hw_t->data;
        score = (float*)best_path->token_score->data;
        cnt = 0; sum = sum2 = 0.0;
        for(j = 0; j < len; j++){
            if(*data2 > 2){
                if((j + 1 == len) || ((j + 1 < len) && *(data2 + 1) == 0)){
                    tmp_data = data;
                    tmp_score = score;
                    for(i = j; i >= 0; i--){
                        if(*tmp_data == 1){
                            sum += *tmp_score;
                            cnt += 1;
                        }else{
                            break;
                        }
                        tmp_data--;
                        tmp_score--;
                    }
                }
            }
            sum2 += *score;
            data++;
            data2++;
            score++;
        }
        keywrd->key_avg_prob = sum2/len;
        keywrd->whole_avg_prob = sum/cnt;
        //wtk_debug("%f %d %f\n",sum,cnt,keywrd->whole_avg_prob);
        //wtk_debug("%f %d %f\n",sum2,len,keywrd->key_avg_prob);
        //wtk_debug("%d %d %d %f %f\n",end,start,len,ref_path->am_score,best_path->am_score);
        keywrd->timestamp = best_path->timestamp;
        keywrd->aver_amprob = best_path->am_score/(best_path->hw_t->pos/sizeof(int));
        if(flag2 == 0){
            if(ref_path && keywrd->ref_aver_amprob > keywrd->aver_amprob){
                // wtk_debug("%f %f\n",keywrd->ref_aver_amprob,keywrd->aver_amprob);
                // qtk_beam_searcher_debug(searcher,&best_path,1);
                // if(ref_path){
                //     qtk_beam_searcher_debug(searcher,&ref_path,1);
                // }
                best_path = NULL;
            }
        }
        //keywrd->num_wrds = len;
        keywrd->best_pth = best_path;
        //wtk_debug("%f %f %f %d %d\n",keywrd->conf,keywrd->wrd_speed,keywrd->aver_amprob,keywrd->num_toks,keywrd->num_wrds);
    }
}

void qtk_beam_search_keywrd_reset(qtk_beam_searcher_t* searcher){
    qtk_beam_searcher_keyword_t* key = &(searcher->keywrd);
    key->best_pth = NULL;
    key->ref_pth = NULL;
    key->key_avg_prob = -10.0;
    key->whole_avg_prob = -10.0;
    key->aver_amprob = -10.0;
    key->ref_aver_amprob = -100.0;
    key->keywrd  = 0;
    key->nihao = 0;
}


void qtk_beam_searcher_reset(qtk_beam_searcher_t* searcher){
    int i;

    searcher->num_toks = 1;
    searcher->index = 0;
    for(i = 0; i < searcher->ntopk; i++){
        *(searcher->indexes + i) = -1;
        *(searcher->probs + i) = -FLT_MAX;
    }

    for(i = 0; i < searcher->cfg->beam; i++){
        qtk_beam_search_hyp_reset(searcher->toks[i]);
        qtk_beam_search_hyp_reset(searcher->ptoks[i]);
    }

    if(searcher->cfg->use_context){
        for(i = 0; i < searcher->ntopk; i++){
            qtk_beam_search_hyp_reset(searcher->ntoks[i]);
        }
    }

    if(searcher->pool){
        wtk_vpool2_reset(searcher->pool);
    }

    if(searcher->net){
        qtk_k2_context_net_reset2(searcher->net);
    }
    qtk_beam_search_keywrd_reset(searcher);
}

void qtk_beam_searcher_delete(qtk_beam_searcher_t* searcher){
    int i;
    for(i = 0; i < searcher->cfg->beam; i++){
        qtk_beam_search_hyp_delete(searcher->toks[i]);
        qtk_beam_search_hyp_delete(searcher->ptoks[i]);
    }

    if(searcher->net){
        for(i = 0; i < searcher->ntopk; i++){
            qtk_beam_search_hyp_delete(searcher->ntoks[i]);
        }
        wtk_free(searcher->ntoks);
        qtk_k2_context_net_delete(searcher->net);
    }
    if(searcher->pool){
        wtk_vpool2_delete(searcher->pool);
    }
    wtk_free(searcher->indexes);
    wtk_free(searcher->probs);
    wtk_free(searcher->toks);
    wtk_free(searcher->ptoks);
    wtk_free(searcher);
}
