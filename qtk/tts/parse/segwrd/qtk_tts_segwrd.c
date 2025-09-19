#include "qtk_tts_segwrd.h"
#include "wtk/tts/tts-mer/wtk-extend/wtk_mat2.h"
#include "wtk/tts/tts-mer/wtk-extend/wtk_heap2.h"
#include "wtk/tts/tts-mer/wtk-extend/wtk_mer_source_file.h"

int qtk_tts_segwrd_cut(qtk_tts_segwrd_t *segwrd,wtk_tts_info_t *info,wtk_tts_snt_t *snt);
wtk_string_t** qtk_tts_segwrd_crf_normalize_char(qtk_tts_segwrd_t *segwrd,wtk_heap_t *heap,wtk_string_t **charts,int pos);
void qtk_tts_segwrd_charts_print(wtk_string_t **charts,int pos);
qtk_tts_segwrd_node_t *qtk_tts_segwrd_node_heap_new(wtk_heap_t *heap,int s,int pos,int isw);
void qtk_tts_segwrd_node_print(wtk_string_t **charts,qtk_tts_segwrd_node_t *node);
void qtk_tts_segwrd_queue_print(wtk_string_t **charts,wtk_queue_t *q);

qtk_tts_segwrd_t* qtk_tts_segwrd_new(qtk_tts_segwrd_cfg_t *cfg, wtk_rbin2_t* rbin)
{
    qtk_tts_segwrd_t *seg = NULL;
    wtk_source_loader_t sl;
    wtk_source_t src;
    FILE *f;
    int ret=0;

    wtk_source_loader_init_file(&sl);

    seg = wtk_malloc(sizeof(*seg));
    seg->cfg = cfg;
    wtk_queue_init(&seg->seg_qn);
    seg->modelw = wtk_vecdf_new(cfg->n_tag*cfg->n_feature+cfg->n_tag*cfg->n_tag);
    if (rbin)
    {
		f=wtk_rbin2_get_file(rbin, cfg->modelw_fn);
		wtk_source_init_fd(&src,f, ftell(f));
		ret=wtk_mer_source_read_double(&src, seg->modelw->p, seg->modelw->len, 1);
		if (ret!=0)goto end;
		wtk_source_clean_fd(&src);
		if (f)fclose(f);
    }else
    {
    	ret=wtk_mer_source_loader_load_vecdf(&sl,&src,cfg->modelw_fn,seg->modelw);
    	if(ret!=0)goto end;
    }
end:
    if (ret!=0)
    {
    	qtk_tts_segwrd_delete(seg);
    	seg=NULL;
    }
    return seg;
}

int qtk_tts_segwrd_process(qtk_tts_segwrd_t *segwrd,wtk_tts_info_t *info,wtk_tts_lab_t *lab)
{
    int ret = 0;
    int n = 0,i = 0;
    wtk_tts_snt_t **snts = NULL;

    snts = lab->snts->slot;
    n = lab->snts->nslot;
    for(i = 0; i < n;++i){
        qtk_tts_segwrd_cut(segwrd,info,snts[i]);
    }

    return ret;
}

int qtk_tts_segwrd_delete(qtk_tts_segwrd_t *segwrd)
{
    int ret = 0;
    if(segwrd->modelw) wtk_vecdf_delete(segwrd->modelw);
    wtk_free(segwrd);
    return ret;
}

int qtk_tts_segwrd_reset(qtk_tts_segwrd_t *segwrd)
{
    int ret = 0;
    wtk_queue_init(&segwrd->seg_qn);
    return ret;
}

int qtk_tts_segwrd_features_idx_get(qtk_tts_segwrd_t *segwrd,char *data,int len)
{
    int ret = -1;
    wtk_string_t *v = NULL;

    if(data == NULL)
        goto end;
    v = wtk_kdict_get(segwrd->cfg->feature_idx_dict,data,len);
    if(v){
       ret = wtk_str_atoi(v->data,v->len); 
    }
end:
    return ret;
}

int qtk_tts_segwrd_unigram_in(qtk_tts_segwrd_t *segwrd,wtk_string_t *str)
{
    int ret = -1;
    if(str == NULL)
        goto end;
    if(wtk_kdict_get(segwrd->cfg->unigram_dict,str->data,str->len)){
        ret = 1;
    }
end:
    return ret;
}

wtk_string_t *qtk_tts_segwrd_get_slice_str(wtk_string_t **charts,wtk_heap_t *heap,int start, int usel,int len)
{
    wtk_strbuf_t *buf = NULL;
    int i = 0;
    wtk_string_t *rets = NULL;

    buf = wtk_strbuf_new(125,1.0f);
    if(start < 0 || start >= len)
       goto end;
    if((start + usel) >= (len+1))
        goto end;
    for(i = 0; i < usel;++i){
        wtk_strbuf_push(buf,charts[start+i]->data,charts[start+i]->len);
    }
    rets = wtk_heap_dup_string2(heap,buf->data,buf->pos);
end:
    wtk_strbuf_delete(buf);
    return rets;
}

wtk_veci_t* qtk_tts_segwrd_features_idx(qtk_tts_segwrd_t *segwrd,wtk_heap_t *heap,wtk_string_t **charts,int idx,int pos)
{
    wtk_veci_t *retv = NULL;
    qtk_tts_segwrd_cfg_t *cfg = segwrd->cfg;
    wtk_string_t *chart = charts[idx],*prev_c = NULL,*next_c = NULL,
        *prepre_c = NULL,*wrd = NULL;
    wtk_strbuf_t *buf_tmp = wtk_strbuf_new(256,1.0f);
    wtk_strbuf_t *tmp = wtk_strbuf_new(256,1.0f);
    int ret = 0,i = 0,j = 0,sn1 = 0,sn2 = 0;
    wtk_array_t *prelst_in = NULL,*postlst_in = NULL;
    wtk_array_t *prelst_ex = NULL,*postlst_ex = NULL;
    wtk_string_t **s1 = NULL,**s2 = NULL;

    //$$ starts feature
    ret = 0;
    wtk_strbuf_push(tmp,(char*)&ret,sizeof(ret));
    //c.
    wtk_strbuf_reset(buf_tmp);
    wtk_strbuf_push(buf_tmp,"c.",strlen("c."));
    wtk_strbuf_push(buf_tmp,chart->data,chart->len);
    ret = qtk_tts_segwrd_features_idx_get(segwrd,buf_tmp->data,buf_tmp->pos);
    if(ret > 0)
        wtk_strbuf_push(tmp,(char*)&ret,sizeof(ret));
    //c-1. || c-1c.
    if(idx > 0){
        wtk_strbuf_reset(buf_tmp);
        prev_c = charts[idx-1];
        wtk_strbuf_push(buf_tmp,"c-1.",strlen("c-1."));
        wtk_strbuf_push(buf_tmp,prev_c->data,prev_c->len);
        ret = qtk_tts_segwrd_features_idx_get(segwrd,buf_tmp->data,buf_tmp->pos);
        if(ret > 0)
            wtk_strbuf_push(tmp,(char*)&ret,sizeof(ret));
        
        wtk_strbuf_reset(buf_tmp);
        wtk_strbuf_push(buf_tmp,"c-1c.",strlen("c-1c."));
        wtk_strbuf_push(buf_tmp,prev_c->data,prev_c->len);
        wtk_strbuf_push(buf_tmp,".",strlen("."));
        wtk_strbuf_push(buf_tmp,chart->data,chart->len);
        ret = qtk_tts_segwrd_features_idx_get(segwrd,buf_tmp->data,buf_tmp->pos);
        if(ret > 0)
            wtk_strbuf_push(tmp,(char*)&ret,sizeof(ret));
    }
    //c1. || cc1.
    if(idx+1 < pos){
        next_c = charts[idx+1];
        wtk_strbuf_reset(buf_tmp);
        wtk_strbuf_push(buf_tmp,"c1.",strlen("c1."));
        wtk_strbuf_push(buf_tmp,next_c->data,next_c->len);
        ret = qtk_tts_segwrd_features_idx_get(segwrd,buf_tmp->data,buf_tmp->pos);
        if(ret > 0)
            wtk_strbuf_push(tmp,(char*)&ret,sizeof(ret));
        
        wtk_strbuf_reset(buf_tmp);
        wtk_strbuf_push(buf_tmp,"cc1.",strlen("cc1."));
        wtk_strbuf_push(buf_tmp,chart->data,chart->len);
        wtk_strbuf_push(buf_tmp,".",strlen("."));
        wtk_strbuf_push(buf_tmp,next_c->data,next_c->len);
        ret = qtk_tts_segwrd_features_idx_get(segwrd,buf_tmp->data,buf_tmp->pos);
        if(ret > 0)
            wtk_strbuf_push(tmp,(char*)&ret,sizeof(ret));
    }
    //c-2. || c-2c-2.
    if(idx>1){
        prepre_c = charts[idx-2];
        wtk_strbuf_reset(buf_tmp);
        wtk_strbuf_push(buf_tmp,"c-2.",strlen("c-2."));
        wtk_strbuf_push(buf_tmp,prepre_c->data,prepre_c->len);
        ret = qtk_tts_segwrd_features_idx_get(segwrd,buf_tmp->data,buf_tmp->pos);
        if(ret > 0)
            wtk_strbuf_push(tmp,(char*)&ret,sizeof(ret));
        
        wtk_strbuf_reset(buf_tmp);
        wtk_strbuf_push(buf_tmp,"c-2c-1.",strlen("c-2c-1."));
        wtk_strbuf_push(buf_tmp,prepre_c->data,prepre_c->len);
        wtk_strbuf_push(buf_tmp,".",strlen("."));
        wtk_strbuf_push(buf_tmp,prev_c->data,prev_c->len);
        ret = qtk_tts_segwrd_features_idx_get(segwrd,buf_tmp->data,buf_tmp->pos);
        if(ret > 0)
            wtk_strbuf_push(tmp,(char*)&ret,sizeof(ret));
    }
    //c2.
    if(idx+2 < pos){
        wtk_strbuf_reset(buf_tmp);
        wtk_strbuf_push(buf_tmp,"c2.",strlen("c2."));
        wtk_strbuf_push(buf_tmp,charts[idx+2]->data,charts[idx+2]->len);
        ret = qtk_tts_segwrd_features_idx_get(segwrd,buf_tmp->data,buf_tmp->pos);
        if(ret > 0)
            wtk_strbuf_push(tmp,(char*)&ret,sizeof(ret));
    }
    //use num/letter based features
    prelst_in = wtk_array_new_h(heap,10,sizeof(wtk_string_t*));
    for(i = cfg->wordmax; i > cfg->wordmin-1;--i){
        wrd = qtk_tts_segwrd_get_slice_str(charts,heap,idx-i+1,i,pos);
        if(qtk_tts_segwrd_unigram_in(segwrd,wrd) >= 0){
            wtk_strbuf_reset(buf_tmp);
            wtk_strbuf_push(buf_tmp,"w-1.",strlen("w-1."));
            wtk_strbuf_push(buf_tmp,wrd->data,wrd->len);
            ret = qtk_tts_segwrd_features_idx_get(segwrd,buf_tmp->data,buf_tmp->pos);
            if(ret > 0)
                wtk_strbuf_push(tmp,(char*)&ret,sizeof(ret));
            wtk_array_push2(prelst_in,&wrd);
        }else{
            wrd = wtk_heap_dup_string2(heap,"**noWord",strlen("**noWord"));
            wtk_array_push2(prelst_in,&wrd);
        }
    }
    postlst_in = wtk_array_new_h(heap,10,sizeof(wtk_string_t*));
    for(i = cfg->wordmax; i > cfg->wordmin-1;--i){
        wrd = qtk_tts_segwrd_get_slice_str(charts,heap,idx,i,pos);
        if(qtk_tts_segwrd_unigram_in(segwrd,wrd) >= 0){
            wtk_strbuf_reset(buf_tmp);
            wtk_strbuf_push(buf_tmp,"w1.",strlen("w1."));
            wtk_strbuf_push(buf_tmp,wrd->data,wrd->len);
            ret = qtk_tts_segwrd_features_idx_get(segwrd,buf_tmp->data,buf_tmp->pos);
            if(ret > 0)
                wtk_strbuf_push(tmp,(char*)&ret,sizeof(ret));
            wtk_array_push2(postlst_in,&wrd);
        }else{
            wrd = wtk_heap_dup_string2(heap,"**noWord",strlen("**noWord"));
            wtk_array_push2(postlst_in,&wrd);
        }
    } 
    prelst_ex = wtk_array_new_h(heap,10,sizeof(wtk_string_t*));
    for(i = cfg->wordmax; i > cfg->wordmin-1;--i){
        wrd = qtk_tts_segwrd_get_slice_str(charts,heap,idx-i,i,pos);
        if(qtk_tts_segwrd_unigram_in(segwrd,wrd) >= 0){
            wtk_array_push2(prelst_ex,&wrd);
        }else{
            wrd = wtk_heap_dup_string2(heap,"**noWord",strlen("**noWord"));
            wtk_array_push2(prelst_ex,&wrd);
        }
    } 
    postlst_ex = wtk_array_new_h(heap,10,sizeof(wtk_string_t*));
    for(i = cfg->wordmax; i > cfg->wordmin-1;--i){
        wrd = qtk_tts_segwrd_get_slice_str(charts,heap,idx+1,i,pos);
        if(qtk_tts_segwrd_unigram_in(segwrd,wrd) >= 0){
            wtk_array_push2(postlst_ex,&wrd);
        }else{
            wrd = wtk_heap_dup_string2(heap,"**noWord",strlen("**noWord"));
            wtk_array_push2(postlst_ex,&wrd);
        }
    }     
    s1 = prelst_ex->slot;
    sn1 = prelst_ex->nslot;
    s2 = postlst_in->slot;
    sn2 = postlst_in->nslot;
    for(i = 0; i < sn1;++i){
        for(j = 0; j < sn2;++j){
            wtk_strbuf_reset(buf_tmp);
            wtk_strbuf_push(buf_tmp,"ww.l.",strlen("ww.l."));
            wtk_strbuf_push(buf_tmp,s1[i]->data,s1[i]->len);
            wtk_strbuf_push(buf_tmp,"*",strlen("*"));
            wtk_strbuf_push(buf_tmp,s2[j]->data,s2[j]->len);
            ret = qtk_tts_segwrd_features_idx_get(segwrd,buf_tmp->data,buf_tmp->pos);
            if(ret > 0)
                wtk_strbuf_push(tmp,(char*)&ret,sizeof(ret));
        }
    }
    s1 = prelst_in->slot;
    sn1 = prelst_in->nslot;
    s2 = postlst_ex->slot;
    sn2 = postlst_ex->nslot;
    for(i = 0; i < sn1; ++i){
        for(j = 0; j < sn2; ++j){
            wtk_strbuf_reset(buf_tmp);
            wtk_strbuf_push(buf_tmp,"ww.r.",strlen("ww.r."));
            wtk_strbuf_push(buf_tmp,s1[i]->data,s1[i]->len);
            wtk_strbuf_push(buf_tmp,"*",strlen("*"));
            wtk_strbuf_push(buf_tmp,s2[j]->data,s2[j]->len);
            ret = qtk_tts_segwrd_features_idx_get(segwrd,buf_tmp->data,buf_tmp->pos);
            if(ret > 0)
                wtk_strbuf_push(tmp,(char*)&ret,sizeof(ret));
        }
    }
    
    retv = wtk_veci_heap_new(heap,tmp->pos/sizeof(int));
    memcpy(retv->p,tmp->data,retv->len*sizeof(int));
    wtk_strbuf_delete(tmp);
    wtk_strbuf_delete(buf_tmp);
    return retv;
}
//得到选择和转移矩阵
int qtk_tts_segwrd_crf_matrix(qtk_tts_segwrd_t *segwrd,wtk_veci_t **vecis,int pos,wtk_matdf_t *Y,wtk_matdf_t *YY)
{
    int i = 0,j = 0,k = 0;
    wtk_veci_t *vec = NULL;
    int n_tag = segwrd->cfg->n_tag;
    int ft = 0;
    int f = 0;
    double scalar = segwrd->cfg->scalar;
    int backoff = segwrd->cfg->n_feature*n_tag;
    memset(Y->p,0,sizeof(double)*Y->row*Y->col);
    for(i = 0; i < YY->col*YY->row;++i){
        YY->p[i] = 1.0;
    }
    //选择矩阵
    for(i = 0; i < pos;++i){
        vec = vecis[i];
        for(j = 0; j < vec->len; ++j){
            ft = vec->p[j];
            // printf("%d\n",ft);
            for(k = 0; k < n_tag;++k){
                f = ft*n_tag+k;
                Y->p[Y->col*i+k] += segwrd->modelw->p[f] * scalar;
            }
        }
    }
    //转移矩阵
    for(i = 0; i < n_tag; ++i){
        for(j = 0; j < n_tag;++j){
                f = backoff+i*n_tag+j;
                YY->p[j*YY->col+i] += segwrd->modelw->p[f] *scalar;        
            }
    }
    return 0;
}
//对数字做处理
void qtk_tts_segwrd_run_viterbi_1(double *in,wtk_matdf_t *out)
{
    int i = 0,j = 0;
    double f = 0.0;
    for(i = 0; i < out->row; ++i){
        f = in[i];
        for(j = 0; j < out->col; ++j){
            out->p[i*out->col+j] = f;
        }
    }
}

void qtk_tts_segwrd_run_viterbi_2(double *in,wtk_matdf_t *out)
{
    int i = 0;
    for(i = 0; i < out->row; ++i){
        memcpy(out->p+i*out->col,in,sizeof(double)*out->col);
    }
}

void qtk_tts_segwrd_run_viterbi_max(wtk_matdf_t *mat,int *max_idx,double *max)
{
    int i = 0,j = 0; 
    for(i = 0; i < mat->col; ++i){
        max_idx[i]=0;
        max[i] = mat->p[i];
        for(j = 1; j < mat->row;++j){
            if(mat->p[j*mat->col+i] > max[i]){
                max[i] = mat->p[j*mat->col+i];
                max_idx[i] = j;
            }
        }
    }
}

wtk_veci_t* qtk_tts_segwrd_run_viterbi(qtk_tts_segwrd_t *segwrd,wtk_tts_info_t *info,wtk_matdf_t *Y,wtk_matdf_t *YY)
{
    wtk_veci_t *vec_r = NULL;
    wtk_matdf_t *pre_m = NULL,*pre = NULL,*new_m = NULL;
    wtk_heap_t *heap = info->heap;
    int i = 0;
    wtk_mati_t *max_i = NULL;
    int max = 0;
    float maxf = 0;
    pre = wtk_matdf_heap_new(heap,1,Y->col);
    pre_m = wtk_matdf_heap_new(heap,Y->col,YY->col);
    new_m = wtk_matdf_heap_new(heap,Y->col,YY->col);
    max_i = wtk_mati_heap_new(heap,Y->row,Y->col);
    memcpy(pre->p,Y->p,sizeof(double)*Y->col);
    for(i = 1; i < Y->row; ++i){
        qtk_tts_segwrd_run_viterbi_1(pre->p,pre_m);
        qtk_tts_segwrd_run_viterbi_2(Y->p+i*Y->col,new_m);
        wtk_matdf_add2(new_m,YY);
        wtk_matdf_add2(new_m,pre_m);
        qtk_tts_segwrd_run_viterbi_max(new_m,max_i->p+i*max_i->col,pre->p);
    }
    vec_r = wtk_veci_heap_new(heap,Y->row);
    maxf = pre->p[0];
    max = 0;
    for(i = 1; i < pre->col;++i){
        if(pre->p[i] > maxf){
            max = i;
            maxf = pre->p[i];
        }
    }

    vec_r->p[vec_r->len-1] = max;
    for(i = vec_r->len-2; i > -1;i--){
        max = max_i->p[(i+1)*max_i->col+max]; 
        vec_r->p[i] = max;
    }

    return vec_r;
}

wtk_queue_t* qtk_tts_segwrd_crf_cut(qtk_tts_segwrd_t *segwrd,wtk_tts_info_t *info,wtk_string_t **charts,int shift,int pos)
{
    wtk_queue_t *queue = NULL;
    int i = 0,s = 0,ep = 0;
    wtk_heap_t *heap = info->heap;
    wtk_veci_t **vecs = NULL;
    wtk_matdf_t *Y = NULL,*YY=NULL;
    wtk_veci_t *vec = NULL;
    qtk_tts_segwrd_node_t *node;

    queue = wtk_heap_malloc(heap,sizeof(wtk_queue_t));
    wtk_queue_init(queue);
    vecs = wtk_heap_malloc(heap,sizeof(wtk_veci_t*)*pos);
    charts = qtk_tts_segwrd_crf_normalize_char(segwrd,info->heap,charts,pos);   //做规范化
    for(i = 0; i < pos; ++i){
        vecs[i] = qtk_tts_segwrd_features_idx(segwrd,heap,charts,i,pos);
        // wtk_veci_print(vecs[i]);
    }
    Y = wtk_matdf_heap_new(heap,pos,segwrd->cfg->n_tag);
    YY = wtk_matdf_heap_new(heap,segwrd->cfg->n_tag,segwrd->cfg->n_tag);
    qtk_tts_segwrd_crf_matrix(segwrd, vecs, pos, Y, YY);
    vec=qtk_tts_segwrd_run_viterbi(segwrd,info,Y,YY);
    // wtk_veci_print(vec);
    //{0: 'B', 1: 'B_single', 2: 'I', 3: 'I_end', 4: 'I_first'}
    s = 0;
    ep = 1;
    for(i = 1;i<vec->len;++i){
        if(vec->p[i] == 0 || vec->p[i] == 1){
            node = qtk_tts_segwrd_node_heap_new(heap,s+shift,ep,1);
            wtk_queue_push(queue,&node->q_n);
            s = i;
            ep = 1;
        }else{
            ep++;
        }
    }
    if(pos > 0){
        node = qtk_tts_segwrd_node_heap_new(heap,s+shift,ep,1);
        wtk_queue_push(queue,&node->q_n);
    }
    return queue;
}

wtk_string_t* qtk_tts_segwrd_crf_keyword_name(qtk_tts_segwrd_t *segwrd,wtk_heap_t *heap,wtk_string_t *chart)
{
    char *key_words[] = {"-",".","_",",","|","/","*",":"};
    char *key_words_translate[]={"&","&","&","&","&","&","&","&"};
    int n = sizeof(key_words)/sizeof(char*),i = 0;
    wtk_string_t *rchart = NULL;

    for(i = 0; i < n; ++i){
        if(chart->len == strlen(key_words[i]) &&0 == strncmp(chart->data,key_words[i],chart->len)){
            break;
        }
    }

    if(i == n){
        rchart = chart;
    }else{
        rchart = wtk_heap_dup_string(heap,key_words_translate[i],strlen(key_words_translate[i]));
    }

    return rchart;
}

wtk_string_t* qtk_tts_segwrd_crf_numletter_norm(qtk_tts_segwrd_t *segwrd,wtk_heap_t *heap,wtk_string_t *chart)
{
    wtk_string_t *re_str = chart;
    if(wtk_array_str_in(segwrd->cfg->norm_num,chart->data,chart->len)){
        re_str = wtk_heap_dup_string(heap,"**Num",strlen("**Num"));
    }
    if(wtk_array_str_in(segwrd->cfg->norm_letter,chart->data,chart->len)){
        re_str = wtk_heap_dup_string(heap,"**Letter",strlen("**Letter"));
    }
    return re_str;
}


wtk_string_t** qtk_tts_segwrd_crf_normalize_char(qtk_tts_segwrd_t *segwrd,wtk_heap_t *heap,wtk_string_t **charts,int pos)
{
    int i = 0;
    wtk_string_t **ret_charts = NULL,*chart = NULL;

    ret_charts = wtk_heap_malloc(heap,sizeof(wtk_string_t*)*pos);
    for(i = 0; i < pos; ++i){
        chart = charts[i];
        chart = qtk_tts_segwrd_crf_keyword_name(segwrd,heap,chart);
        ret_charts[i] = qtk_tts_segwrd_crf_numletter_norm(segwrd,heap,chart);
    }
    return ret_charts;
}

void qtk_tts_segwrd_queue_move(wtk_queue_t *in1,wtk_queue_t *in2)
{
    wtk_queue_node_t *node = NULL;
    if(in1 == NULL || in2 == NULL){
        return;
    }
    for(node = wtk_queue_pop(in2);node;node = wtk_queue_pop(in2)){
        wtk_queue_push(in1,node);
    }
    return;
}
int qtk_tts_segwrd_crf(qtk_tts_segwrd_t *segwrd,wtk_tts_info_t *info,wtk_tts_snt_t *snt)
{
    wtk_string_t **charts = NULL;
    wtk_queue_t flist,*re_q;
    wtk_queue_node_t *qn;
    qtk_tts_segwrd_node_t *node = NULL;
    int ret = -1;

    charts = snt->chars->slot;
    wtk_queue_init(&flist);
    while(1){
        qn = wtk_queue_pop(&segwrd->seg_qn);
        if(!qn) break;
        node = data_offset2(qn,qtk_tts_segwrd_node_t,q_n);
        if(node->isw == 1){
            wtk_queue_push(&flist,&node->q_n);
            continue;
        }
        re_q = qtk_tts_segwrd_crf_cut(segwrd,info,charts+node->s,node->s,node->pos);
        if(re_q == NULL) printf("no seqwrd\n");
        // qtk_tts_segwrd_queue_print(charts,re_q);
        qtk_tts_segwrd_queue_move(&flist,re_q);
    }
    qtk_tts_segwrd_queue_move(&segwrd->seg_qn,&flist);
    return ret;
}

int qtk_tts_segwrd_cut_start(qtk_tts_segwrd_t *segwrd,wtk_heap_t *heap,wtk_tts_snt_t *snt)
{
    int ret = 0;
    qtk_tts_segwrd_node_t *node = qtk_tts_segwrd_node_heap_new(heap,0,snt->chars->nslot,0);
    wtk_queue_init(&segwrd->seg_qn);
    wtk_queue_push(&segwrd->seg_qn,&node->q_n);
    return ret;
}

wtk_string_t *qtk_tts_segwrd_cut_post_merged(wtk_queue_t *queue,wtk_heap_t *heap,int s,int e,wtk_string_t **charts)
{
    wtk_string_t *merge = NULL;
    int i = 0,j = 0;
    wtk_queue_node_t *qn = NULL;
    qtk_tts_segwrd_node_t *node = NULL;
    wtk_strbuf_t *buf = wtk_strbuf_new(256,1.0f);

    for(i = s; i < e;++i){
        qn = wtk_queue_peek(queue,i);
        node = data_offset2(qn,qtk_tts_segwrd_node_t,q_n);
        for(j = node->s; j <= node->e;++j){
            wtk_strbuf_push(buf,charts[j]->data,charts[j]->len);
        }
    }
    merge = wtk_heap_dup_string2(heap,buf->data,buf->pos);
    wtk_strbuf_delete(buf);

    return merge;
}

int qtk_tts_segwrd_cut_post_merged_isin(qtk_tts_segwrd_t *segwrd,char *data,int len)
{
    int ret = -1;

    if(wtk_kdict_get(segwrd->cfg->post_cut_dict,data,len)){
        ret = 1;
    }
    return ret;
}
//查看每一个词是不是都在表中
int qtk_tts_segwrd_cut_post_check_seqerated(qtk_tts_segwrd_t *segwrd,int s,int e,wtk_string_t **charts)
{
    int ret = -1;
    int i = 0,j = 0;
    wtk_queue_t *queue = &segwrd->seg_qn;
    wtk_queue_node_t *qn = NULL;
    qtk_tts_segwrd_node_t *node = NULL;
    wtk_strbuf_t *buf = wtk_strbuf_new(256,1.0f);

    for(i = s; i < e;++i){
        qn = wtk_queue_peek(queue,i);
        node = data_offset2(qn,qtk_tts_segwrd_node_t,q_n);
        wtk_strbuf_reset(buf);
        for(j = node->s; j <= node->e;++j){
            wtk_strbuf_push(buf,charts[j]->data,charts[j]->len);
        }
        ret = qtk_tts_segwrd_cut_post_merged_isin(segwrd,buf->data,buf->pos);
        if(ret < 0){
            break;
        }
    }
    wtk_strbuf_delete(buf);
    return ret;
}

void qtk_tts_segwrd_cut_post_merged_new(qtk_tts_segwrd_t *segwrd,wtk_heap_t *heap,int s,int e)
{
    int i = 0;
    wtk_queue_t *queue = &segwrd->seg_qn;
    wtk_queue_node_t *qn = NULL;
    qtk_tts_segwrd_node_t *node = NULL;
    int start = 0,end = 0;

    for(i = s;i < e; ++i){
        qn = wtk_queue_peek(queue,s);
        wtk_queue_remove(queue,qn);
        node = data_offset2(qn,qtk_tts_segwrd_node_t,q_n);
        if(i == s){
            start = node->s;
        }
    }
    end = node->e;
    node = qtk_tts_segwrd_node_heap_new(heap,start,end-start+1,1);
    qn = wtk_queue_peek(queue,s);
    if(qn){
        wtk_queue_insert_before(queue,qn,&node->q_n);
    }else{
        qn = wtk_queue_peek(queue,s-1);
        wtk_queue_insert_to(queue,qn,&node->q_n);
    }
    return;
}

//这边是使用词去merge的 成为一个词然后去查询
int qtk_tts_segwrd_cut_post(qtk_tts_segwrd_t *segwrd,wtk_tts_info_t *info,wtk_tts_snt_t *snt)
{
    int ret = 0;
    wtk_string_t **charts = snt->chars->slot,*merged = NULL;
    int i = 0,end=0,j = 0;
    wtk_queue_t *qu = &segwrd->seg_qn;
    wtk_heap_t *heap = info->heap;

    for(i = 7; i > 1; i--){
        end = qu->length - i;
        if(end < 0) continue;
        j = 0;
        while(j < end + 1){
            merged = qtk_tts_segwrd_cut_post_merged(&segwrd->seg_qn,heap,j,j+i,charts);
            if(qtk_tts_segwrd_cut_post_merged_isin(segwrd,merged->data,merged->len) > 0){
                if(qtk_tts_segwrd_cut_post_check_seqerated(segwrd,j,j+i,charts) < 0){   //不是所有的词都在表中
                    qtk_tts_segwrd_cut_post_merged_new(segwrd,heap,j,j+i);
                    end = qu->length-i;
                }
            }
            ++j;
        }
    }
    // qtk_tts_segwrd_queue_print(charts,qu);
    return ret;
}

int qtk_tts_segwrd_cut_change_wtk(qtk_tts_segwrd_t *segwrd,wtk_tts_info_t *info,wtk_tts_snt_t *snt)
{
    wtk_string_t **charts = snt->chars->slot,*str = NULL;
    wtk_tts_wrd_t *w = NULL;
    wtk_queue_t *queue = &segwrd->seg_qn;
    wtk_queue_node_t *qn = NULL;
    qtk_tts_segwrd_node_t *node = NULL;
    wtk_strbuf_t *buf = wtk_strbuf_new(256,1.0f);
    int i = 0;
    
    snt->wrds = wtk_array_new_h(info->heap,segwrd->seg_qn.length,sizeof(void*));
    while(1){
        qn = wtk_queue_pop(queue);
        if(qn == NULL) break;
        node = data_offset2(qn,qtk_tts_segwrd_node_t,q_n);
        wtk_strbuf_reset(buf);
        for(i = node->s;i <= node->e;++i){
            wtk_strbuf_push(buf,charts[i]->data,charts[i]->len);
        }
        str = wtk_heap_dup_string2(info->heap,buf->data,buf->pos);
        w = wtk_tts_wrd_new(info->heap,str);
        w->snt = snt;
        w->pron = wtk_heap_zalloc(info->heap,sizeof(wtk_tts_wrd_xpron_t));
        w->pron->pron = wtk_heap_zalloc(info->heap,sizeof(wtk_tts_wrd_pron_t));
        w->pron->pron->nsyl = node->pos;
        // w->pron->pron->syls = wtk_heap_malloc(info->heap,sizeof(wtk_tts_syl_t)*node->pos);
        wtk_array_push2(snt->wrds,&w);
    }
    wtk_strbuf_delete(buf);
    return 0;
}

int qtk_tts_segwrd_cut(qtk_tts_segwrd_t *segwrd,wtk_tts_info_t *info,wtk_tts_snt_t *snt)
{
    // wtk_heap_t *heap = NULL;
    // int n = 0,i = 0;
    qtk_tts_segwrd_cut_start(segwrd,info->heap,snt);
    //前处理    前分词
    //pass
    //crf 分词
    qtk_tts_segwrd_crf(segwrd,info,snt);
    //后处理    后分词
    qtk_tts_segwrd_cut_post(segwrd,info,snt);
    // qtk_tts_segwrd_queue_print(snt->chars->slot,&segwrd->seg_qn);
    //按照snt的格式搞在一起
    qtk_tts_segwrd_cut_change_wtk(segwrd,info,snt);
    return 0;
}



qtk_tts_segwrd_node_t *qtk_tts_segwrd_node_heap_new(wtk_heap_t *heap,int s,int pos,int isw)
{
    qtk_tts_segwrd_node_t *node = NULL;
    
    node = wtk_heap_malloc(heap,sizeof(qtk_tts_segwrd_node_t));
    
    node->s = s;
    node->e = s+pos-1;
    node->pos = pos;
    node->isw = isw;
    return node;
}

void qtk_tts_segwrd_charts_print(wtk_string_t **charts,int pos)
{
    int i = 0;
    for(i = 0; i < pos; ++i){
        printf("%.*s ",charts[i]->len,charts[i]->data);
    }
    puts("");
}
void qtk_tts_segwrd_node_print(wtk_string_t **charts,qtk_tts_segwrd_node_t *node)
{
    int i = 0;
    for(i = node->s;i <= node->e; ++i){
        printf("%.*s",charts[i]->len,charts[i]->data);
    }
    printf(" isw:%d\n",node->isw);
}
void qtk_tts_segwrd_queue_print(wtk_string_t **charts,wtk_queue_t *q)
{
    wtk_queue_node_t *qn = NULL;
    qtk_tts_segwrd_node_t *node = NULL;
    if(q == NULL)
        return;
    for(qn = q->pop;qn;qn = qn->next){
        node = data_offset2(qn,qtk_tts_segwrd_node_t,q_n);
        qtk_tts_segwrd_node_print(charts,node);
    }
}
