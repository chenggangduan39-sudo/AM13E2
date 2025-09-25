#include "wtk_mer_cfg_syn.h"

int wtk_mer_cfg_syn_init(wtk_mer_cfg_syn_t *cfg)
{
    cfg->layer_type = NULL;
    cfg->layer_num = NULL;
    // cfg->covar = wtk_str_hash_new(4);
    wtk_mer_cfg_syn_dnn_init(&(cfg->dur));
    wtk_mer_cfg_syn_dnn_init(&(cfg->act));
    return 0;
};
int wtk_mer_cfg_syn_clean(wtk_mer_cfg_syn_t *cfg)
{
    int ret, i;
    wtk_mer_syn_qes_t *qes = &(cfg->qes);
    wtk_stridx_t **p = qes->discrete_larr->slot;
    for(i=0; i<qes->discrete_larr->nslot; ++i)
    {
        wtk_stridx_delete(p[i]);
    }
    // wtk_mer_covar_clean(&cfg->covar);
    wtk_heap_delete(cfg->heap);
    wtk_stridx_delete(qes->continous);
    wtk_larray_delete(qes->discrete_larr);
    ret = wtk_mer_cfg_syn_dnn_clean(&(cfg->dur));
    if (ret!=0) {goto end;}
    ret = wtk_mer_cfg_syn_dnn_clean(&(cfg->act));
    if (ret!=0) {goto end;}
    ret = 0;
end:
    return ret;
}

static void strbuf_split_comma(wtk_stridx_t *s, wtk_strbuf_t *buf, const int size)
{
    if (buf->data[0] == '{')
    {
        int total_len = buf->pos
            , now_len = 1
            , item_len = 0;
        char *p = buf->data + 1
            , *p2 = p
            , byte=0;
        int i = 0;
        while(1)
        {/* string split */
            byte = *p;
            if (byte == ',' || byte == '}')
            {
                if (item_len > 0)
                {
                    i++;
                    if (i >= size) 
                    {
                        wtk_debug("questions 解析超出限制大小: %d， 建议改大一点\n", size);
                        wtk_exit(1);
                    }
                    wtk_stridx_get_id(s, p2, item_len, 1);
                    // wtk_debug("[%.*s] %d\n", item_len, p2, item_len);
                };
                item_len = 0, now_len++, p++, p2 = p;
                continue; 
            }
            item_len++, p++, now_len++;
            if (now_len >= total_len) {break;}
        }
    }
}

static int qes_load_call(wtk_mer_syn_qes_t *qes, wtk_source_t *src)
{/* 解析读取的文本字符串, \n \t 分割符
    qs_flag = ["CQS", "QS"][index]
 */
    int qs_flag = 1
      , ret=-1;
    wtk_stridx_t *continous =  wtk_stridx_new( 200);
    wtk_larray_t *discrete_larr = wtk_larray_new(1454, sizeof(void*));
    wtk_strbuf_t *buf = wtk_strbuf_new(1024,1);
    memset(buf->data,0,buf->length);
    
    qes->continous = continous;
    qes->discrete_larr = discrete_larr;
    while(1)
    {
        ret=wtk_mer_source_read_string(src, buf);
        if(ret!=0){ret=0;goto end;}

        if (wtk_str_equal_s(buf->data, buf->pos, "CQS"))
        {qs_flag = 0;} 
        else if (wtk_str_equal_s(buf->data, buf->pos, "QS"))
        {qs_flag = 1;}
        
        if (qs_flag == 0)
        {
            if (buf->data[0] == '{')
            {
                strbuf_split_comma(continous, buf, 80);
            }
        }
        else if (qs_flag == 1)
        {
            if (buf->data[0] == '{')
            {
                int size = 50; // 这边内存可以优化
                wtk_stridx_t *s = wtk_stridx_new( size);
                strbuf_split_comma(s, buf, size);
                wtk_larray_push2(discrete_larr, &s);
                // wtk_debug("[%.*s]\n",buf->pos,buf->data);
            }
        }
    }
    end:
        wtk_strbuf_delete(buf);
        return ret;
}
static wtk_vecf_t* wtk_mer_covar_add( wtk_heap_t *heap, wtk_mer_covar_t *covar, char *k)
{
    wtk_vecf_t *v;
    if (strncmp(k, "bap", 3)==0) {
        v=wtk_vecf_heap_new( heap, 3);
        covar->bap=v;
    } else if (strncmp(k, "mgc", 3)==0) {
        v=wtk_vecf_heap_new( heap,180);
        covar->mgc=v;
    } else if (strncmp(k, "vuv", 3)==0) {
        v=wtk_vecf_heap_new( heap,1);
        covar->vuv=v;
    } else if (strncmp(k, "lf0", 3)==0) {
        v=wtk_vecf_heap_new( heap, 3);
        covar->lf0=v;
    } else {
        wtk_debug("未找到匹配的数据结构 \n");
        wtk_exit(1);
    }
    return v;
}
int wtk_mer_cfg_syn_update_local(wtk_mer_cfg_syn_t *cfg, wtk_local_cfg_t *syn_lc)
{
    wtk_heap_t *heap = wtk_heap_new(4096);
    wtk_local_cfg_t *lc;
    wtk_string_t *v;
    int ret
      , layer_size
      , *layer_num;

    cfg->heap=heap;
    cfg->use_dur=1; // 默认使用dnn预测dur
    wtk_cfg_def_param_require(wtk_local_cfg_update_cfg_str, syn_lc, cfg, qes_fn, v);
    wtk_local_cfg_update_cfg_i( syn_lc, cfg, use_dur, v);
    cfg->layer_type = wtk_local_cfg_find_array_s(syn_lc, "layer_type");
    // cfg->use_dur=1;
    cfg->layer_num = wtk_local_cfg_find_int_array_s(syn_lc, "layer_num");
    layer_num = cfg->layer_num->slot;
    layer_size = cfg->layer_type->nslot;

    // wtk_string_t *qes_fn;
    // qes_fn = wtk_local_cfg_find_string_s(syn_lc, "qes_fn");
    // wtk_source_load_file( &(cfg->qes), (wtk_source_load_handler_t) qes_load_call, qes_fn->data);

    if (layer_size != cfg->layer_num->nslot) 
    {wtk_debug("配置有误, layer_type 和 layer_num 数量不一致\n");wtk_exit(1);}

    lc = wtk_local_cfg_find_lc_s(syn_lc, "covar");
    if (lc)
    {
        wtk_mer_covar_t *covar = &cfg->covar;
        wtk_mer_covar_add( heap, covar, "bap");
        wtk_mer_covar_add( heap, covar, "lf0");
        wtk_mer_covar_add( heap, covar, "mgc");
        wtk_mer_covar_add( heap, covar, "vuv");
        covar->fn_bap = wtk_local_cfg_find_string_s(lc,"bap")->data;
        covar->fn_lf0 = wtk_local_cfg_find_string_s(lc,"lf0")->data;
        covar->fn_mgc = wtk_local_cfg_find_string_s(lc,"mgc")->data;
        covar->fn_vuv = wtk_local_cfg_find_string_s(lc,"vuv")->data;
    }
    lc = wtk_local_cfg_find_lc_s(syn_lc, "dur");
    if (lc && cfg->use_dur)
    {
        ret=wtk_mer_cfg_syn_dnn_update_local(&(cfg->dur), lc, layer_size, layer_num);
        if(ret!=0){goto end;}
    }
    lc = wtk_local_cfg_find_lc_s(syn_lc, "act");
    if (lc)
    {
        ret=wtk_mer_cfg_syn_dnn_update_local(&(cfg->act), lc, layer_size, layer_num);
        if(ret!=0){goto end;}
    }
    ret=0;
end:
    return ret;
}
void wtk_mer_cfg_syn_update2(wtk_mer_cfg_syn_t *syn, wtk_source_loader_t *sl)
{
    wtk_source_t s,*src=&s;
    wtk_mer_covar_t *covar = &(syn->covar);

    wtk_mer_source_loader_with_as(sl, src, syn->qes_fn,
    {
        qes_load_call(&(syn->qes), src);
        int n_qes = syn->qes.discrete_larr->nslot + syn->qes.continous->used;
        int n_in=syn->act.n_in;
        if (n_qes+9 != n_in)
        {wtk_debug("questions.hed 和 矩阵模型不匹配 %d %d\n", n_qes, n_in);wtk_exit(1);}
    });
    wtk_mer_source_loader_load_vecf(sl, src, covar->fn_bap, covar->bap);
    wtk_mer_source_loader_load_vecf(sl, src, covar->fn_lf0, covar->lf0);
    wtk_mer_source_loader_load_vecf(sl, src, covar->fn_mgc, covar->mgc);
    wtk_mer_source_loader_load_vecf(sl, src, covar->fn_vuv, covar->vuv);
    if (syn->use_dur) { wtk_mer_cfg_syn_dnn_update2(&(syn->dur), sl); }
    wtk_mer_cfg_syn_dnn_update2(&(syn->act), sl);
}


int wtk_mer_cfg_syn_dnn_init(wtk_mer_cfg_syn_dnn_t *dnn_cfg)
{
    dnn_cfg->heap = NULL;
    dnn_cfg->w_arr = NULL;
    dnn_cfg->b_arr = NULL;
    return 0;
}
int wtk_mer_cfg_syn_dnn_clean(wtk_mer_cfg_syn_dnn_t *dnn_cfg)
{
    if(dnn_cfg->heap) wtk_heap_delete(dnn_cfg->heap);
    return 0;
}
int wtk_mer_cfg_syn_dnn_update_local(wtk_mer_cfg_syn_dnn_t *dnn_cfg, wtk_local_cfg_t *lc, int layer_size, int layer_num[])
{
    wtk_heap_t *heap = wtk_heap_new(4096);
    wtk_string_t *v;
    wtk_matf_t *tf;
    wtk_vecf_t *vf;
    int ret, i, n_in, n_out, row, col;
    char fn[512];

    wtk_cfg_def_param_require(wtk_local_cfg_update_cfg_i, lc, dnn_cfg, n_in, v);
    n_in = dnn_cfg->n_in;
    n_out = atoi(wtk_local_cfg_find_string_s(lc, "n_out")->data);

    dnn_cfg->w_arr = wtk_array_new_h(heap, layer_size, sizeof(void*));
    dnn_cfg->b_arr = wtk_array_new_h(heap, layer_size, sizeof(void*));
    dnn_cfg->norm = wtk_vecf_heap_new( heap, n_in*2);
    // wtk_cfg_def_vecf_load(lc, fn, v, dnn_cfg->norm, "norm_fn", n_in);
    wtk_cfg_def_param_require_str(heap, lc, fn, v, dnn_cfg->norm_fn, "norm_fn", n_in);
    dnn_cfg->mvn = wtk_vecf_heap_new( heap, n_out*2);
    // wtk_cfg_def_vecf_load(lc, fn, v, dnn_cfg->mvn, "mvn_fn", n_out);
    wtk_cfg_def_param_require_str(heap, lc, fn, v, dnn_cfg->mvn_fn, "mvn_fn", n_out);

    dnn_cfg->w_fn = wtk_heap_malloc( heap, sizeof(char*)*layer_size);
    dnn_cfg->b_fn = wtk_heap_malloc( heap, sizeof(char*)*layer_size);

    for (i=0; i<layer_size; ++i)
    {
        row = i==0 ? n_in: layer_num[i];
        col = i==(layer_size-1) ? n_out: layer_num[i];

        tf = wtk_matf_heap_new(heap, col, row);
        // wtk_cfg_def_matf_load(lc, fn, v, tf, "w_fn", i);
        wtk_array_push2(dnn_cfg->w_arr, &tf);
        wtk_cfg_def_param_require_str(heap, lc, fn, v, dnn_cfg->w_fn[i], "w_fn", i);

        vf = wtk_vecf_heap_new(heap, col);
        // wtk_cfg_def_vecf_load(lc, fn, v, vf, "b_fn", i);
        wtk_array_push2(dnn_cfg->b_arr, &vf);
        wtk_cfg_def_param_require_str(heap, lc, fn, v, dnn_cfg->b_fn[i], "b_fn", i);
    }
    dnn_cfg->heap = heap;
    ret=0;
// end:
    return ret;
}
void wtk_mer_cfg_syn_dnn_update2(wtk_mer_cfg_syn_dnn_t *dnn_cfg, wtk_source_loader_t *sl)
{
    int layer_size = dnn_cfg->w_arr->nslot, i;
    wtk_source_t s,*src=&s;
    wtk_matf_t **mf = (wtk_matf_t**)dnn_cfg->w_arr->slot;
    wtk_vecf_t **vf = (wtk_vecf_t**)dnn_cfg->b_arr->slot;
    wtk_mer_source_loader_load_vecf(sl, src, dnn_cfg->mvn_fn, dnn_cfg->mvn);
    wtk_mer_source_loader_load_vecf(sl, src, dnn_cfg->norm_fn, dnn_cfg->norm);
    for(i=0; i<layer_size; ++i)
    {
        wtk_mer_source_loader_load_matf(sl, src, dnn_cfg->w_fn[i], mf[i]);
        wtk_mer_source_loader_load_vecf(sl, src, dnn_cfg->b_fn[i], vf[i]);
    }
}
