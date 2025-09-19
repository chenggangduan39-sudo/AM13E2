#include "qtk_tts_polyphone.h"

#include "qtk_tts_parse_comm.h"
#include "wtk/core/wtk_kdict.h"
#include "wtk/core/math/wtk_mat.h"
#include "tts-mer/wtk-extend/wtk_mat2.h"
#include "tts-mer/wtk-extend/wtk_heap2.h"

int qtk_tts_polyphone_search(qtk_tts_polyphone_t *ply,wtk_tts_info_t *info,wtk_tts_snt_t *snt);
int qtk_tts_polyphone_unstressed_search(qtk_tts_polyphone_t *ply,wtk_tts_info_t *info,wtk_tts_snt_t *snt);
int qtk_tts_polyphone_ply_search(qtk_tts_polyphone_t *ply,wtk_tts_info_t *info,wtk_tts_snt_t *snt);
int qtk_tts_polyphone_ply_format_search(qtk_tts_polyphone_t *ply,wtk_tts_info_t *info,wtk_tts_snt_t *snt);
wtk_veci_t** qtk_tts_polyphone_model_2vec_process(qtk_tts_polyphone_t *ply,wtk_tts_info_t *info,wtk_tts_snt_t *snt);
void qtk_tts_polyphone_arg_max(qtk_tts_polyphone_t *ply,wtk_matf_t *in,wtk_veci_t *out);
int qtk_tts_polyphone_ply_format_search(qtk_tts_polyphone_t *ply,wtk_tts_info_t *info,wtk_tts_snt_t *snt);
void qtk_tts_polyphone_model_find_change_charid(qtk_tts_polyphone_t *ply,wtk_tts_info_t *info,wtk_veci_t *veci);
int qtk_tts_polyphone_model_disambiguation(qtk_tts_polyphone_t *ply,wtk_tts_info_t *info,wtk_tts_snt_t *snt);
int qtk_tts_polyphone_modification(qtk_tts_polyphone_t *ply,wtk_tts_info_t *info,wtk_tts_snt_t *snt);
int qtk_tts_polyphone_setto(qtk_tts_polyphone_t *ply,wtk_tts_info_t *info,wtk_tts_snt_t *snt);

qtk_tts_polyphone_t *qtk_tts_polyphone_new(qtk_tts_polyphone_cfg_t *cfg, wtk_rbin2_t* rbin)
{
    qtk_tts_polyphone_t *ply = NULL;
    ply = wtk_malloc(sizeof(*ply));
    memset(ply,0,sizeof(*ply));
    ply->cfg = cfg;
    ply->qn = wtk_queue_new();
    ply->ply_qn = wtk_queue_new();
    if(cfg->use_disply){
        ply->disply = qtk_tts_dispoly_new2(&cfg->disply, rbin);
    }
    return ply;
}

int qtk_tts_polyphone_process(qtk_tts_polyphone_t *ply,wtk_tts_info_t *info,wtk_tts_lab_t *lab)
{
    int ret = -1;
    int nsnt = 0,i = 0;
    wtk_tts_snt_t **snts = NULL; 
    
    if(ply->cfg->is_en){
        return 0;
    }
    nsnt = lab->snts->nslot;
    snts = lab->snts->slot;
    for(i = 0; i < nsnt;++i){
        qtk_tts_polyphone_reset(ply);
        //查找多音字并生成拼音序列
        ply->ply_num = qtk_tts_polyphone_search(ply,info,snts[i]);
        //规则重音变轻音
        qtk_tts_polyphone_unstressed_search(ply,info,snts[i]);
        //多音字规则消岐
        if(ply->ply_num){
            qtk_tts_polyphone_ply_format_search(ply,info,snts[i]);
        }
        // qtk_tts_polyphone_print(ply);
        //modul消岐
        if(ply->ply_num){
            qtk_tts_polyphone_model_disambiguation(ply,info,snts[i]);
        }
        // qtk_tts_polyphone_print(ply);
        qtk_tts_polyphone_modification(ply,info,snts[i]);
        // qtk_tts_polyphone_print(ply);
        qtk_tts_polyphone_setto(ply,info,snts[i]);
    }
    
    ret = 0;
    return ret;
}

int qtk_tts_polyphone_delete(qtk_tts_polyphone_t *ply)
{
    int ret = 0;
    if(ply->ply_qn)
        wtk_queue_delete(ply->ply_qn);
    if(ply->qn)
        wtk_queue_delete(ply->qn);
    if(ply->disply)
        qtk_tts_dispoly_delete(ply->disply);
    if(ply)
        wtk_free(ply);
    return ret;
}

int qtk_tts_polyphone_reset(qtk_tts_polyphone_t *ply)
{
    wtk_queue_init(ply->qn);
    wtk_queue_init(ply->ply_qn);
    ply->ply_num = 0;
    return 0;
}

int qtk_tts_polyphone_model_disambiguation(qtk_tts_polyphone_t *ply,wtk_tts_info_t *info,wtk_tts_snt_t *snt)
{
    wtk_veci_t **vecs = NULL;
    vecs = qtk_tts_polyphone_model_2vec_process(ply,info,snt);
    wtk_matf_t *disply_out = wtk_matf_heap_new(info->heap,snt->chars->nslot,ply->disply->poly_mask->col);
    qtk_tts_dispoly_process(ply->disply,vecs,disply_out);
    wtk_veci_t *model_out = wtk_veci_heap_new(info->heap,snt->chars->nslot);
    qtk_tts_polyphone_arg_max(ply,disply_out,model_out);
    qtk_tts_polyphone_model_find_change_charid(ply,info,model_out);
    return 0;
}

qtk_tts_pron_t *qtk_tts_polyphone_pron_new(wtk_heap_t *heap,char *data,int len,wtk_tts_wrd_pron_t *pron,int idx)
{
    int i = 0;
    qtk_tts_pron_t *np = wtk_heap_malloc(heap,sizeof(qtk_tts_pron_t));
    np->idx = idx;
    np->wchar = wtk_heap_dup_string(heap,data,len);
    wtk_tts_wrd_pron_t *new_pron,*pron_n = NULL;
    new_pron = wtk_heap_malloc(heap,sizeof(wtk_tts_wrd_pron_t));
    new_pron->nsyl = pron->nsyl;
    new_pron->npron = pron->npron; //head vaile
    new_pron->syls = wtk_heap_malloc(heap,sizeof(wtk_tts_syl_t)*new_pron->nsyl);
    for(i = 0; i < new_pron->nsyl;++i){
        new_pron->syls[i].tone = pron->syls[i].tone;
        new_pron->syls[i].v = wtk_heap_dup_string(heap,pron->syls[i].v->data,pron->syls[i].v->len);
    }
    new_pron->next = NULL;
    np->pron = new_pron;
    while(pron->next){
        pron = pron->next;
        pron_n = wtk_heap_malloc(heap,sizeof(wtk_tts_wrd_pron_t));
        pron_n->nsyl = pron->nsyl;
        pron_n->npron = 1;
        pron_n->syls = wtk_heap_malloc(heap,sizeof(wtk_tts_syl_t)*pron_n->nsyl);
        for(i = 0; i < pron_n->nsyl;++i){
            pron_n->syls[i].tone = pron->syls[i].tone;
            pron_n->syls[i].v = wtk_heap_dup_string(heap,pron->syls[i].v->data,pron->syls[i].v->len);
        }
        new_pron->next = pron_n;
        new_pron = pron_n;
        new_pron->next = NULL;
    }
    // printf("idx:%d %.*s\n",np->idx,np->wchar->len,np->wchar->data);
    // wtk_tts_wrd_pron_print(np->pron);
    return np;
}

qtk_tts_ply_idx_t *qtk_tts_polyphone_ply_idx_new(wtk_heap_t *heap,int idx)
{
    qtk_tts_ply_idx_t *np = wtk_heap_malloc(heap,sizeof(qtk_tts_ply_idx_t));
    np->idx = idx;
    return np;
}

int qtk_tts_polyphone_search(qtk_tts_polyphone_t *ply,wtk_tts_info_t *info,wtk_tts_snt_t *snt)
{
    int n = 0,i = 0,ret = 0;
    wtk_string_t **chars = NULL,*chart = NULL;
    wtk_tts_wrd_pron_t *pron;
    wtk_heap_t *heap = info->heap;
    qtk_tts_pron_t *np = NULL;
    qtk_tts_ply_idx_t *idx_p = NULL;

    n = snt->chars->nslot;
    chars = snt->chars->slot;
    for(i = 0;i < n;++i){
        chart = chars[i];
        pron = wtk_kdict_get(ply->cfg->pron_dict,chart->data,chart->len);
        if(pron == NULL)    continue;   //如果是标点符号的话查不到
        //两个字要特殊处理
        if(chart->len == strlen("不")&& 0 == strncmp("不",chart->data,strlen("不"))){
            pron = wtk_heap_malloc(heap,sizeof(*pron));
            pron->npron = 1;
            pron->syls = wtk_heap_malloc(heap,sizeof(wtk_tts_syl_t));
            pron->nsyl = 1;
            pron->next = NULL;
            pron->syls->v = wtk_heap_dup_string_s(heap,"bu");
            pron->syls->tone = 4;
        }else if(chart->len == strlen("一")&& 0 == strncmp("一",chart->data,strlen("一"))){
            pron = wtk_heap_malloc(heap,sizeof(*pron));
            pron->npron = 1;
            pron->syls = wtk_heap_malloc(heap,sizeof(wtk_tts_syl_t));
            pron->nsyl = 1;
            pron->next = NULL;
            pron->syls->v = wtk_heap_dup_string_s(heap,"yi");
            pron->syls->tone = 1;
        }
        if(pron->npron > 1){
            idx_p = qtk_tts_polyphone_ply_idx_new(heap,i);
            wtk_queue_push(ply->ply_qn,&idx_p->q_n);
            ++ret;
        }
        np = qtk_tts_polyphone_pron_new(heap,chart->data,chart->len,pron,i);
        wtk_queue_push(ply->qn,&np->q_n);
    }
    return ret;
}

int qtk_tts_polyphone_queue_cmp(int *i,qtk_tts_pron_t *pron)
{
    return pron->idx-(*i);
}

qtk_tts_pron_t *qtk_tts_polyphone_queue_get(qtk_tts_polyphone_t *ply,int i)
{
    qtk_tts_pron_t *pron = NULL;
    pron = wtk_queue_find(ply->qn,offsetof(qtk_tts_pron_t,q_n),(wtk_cmp_handler_t)qtk_tts_polyphone_queue_cmp,&i);

    return pron;
}

int qtk_tts_polyphone_unstressed_search(qtk_tts_polyphone_t *ply,wtk_tts_info_t *info,wtk_tts_snt_t *snt)
{
    // int ret = 0;
    wtk_tts_wrd_t **wrds=NULL,*wrd=NULL;
    int wrdn = 0,i = 0,j = 0,n = 0;
    int charn = 0;
    wtk_tts_wrd_pron_t *pron = NULL;
    qtk_tts_pron_t *qn = NULL;
    wtk_heap_t *heap = info->heap;
    
    wrdn = snt->wrds->nslot;
    wrds = snt->wrds->slot;
    
    for(i = 0,charn = 0; i < wrdn;++i,charn+=wrd->pron->pron->nsyl){
        wrd = wrds[i];
        pron = wtk_kdict_get(ply->cfg->unstressed_dict,wrd->v->data,wrd->v->len);
        if(pron == NULL) continue;
        n = pron->nsyl;
        for(j = 0; j < n; ++j){
            qn = qtk_tts_polyphone_queue_get(ply,charn+j);
            if(qn == NULL){wtk_debug("error queue num\n");exit(1);}
            qn->pron->next = NULL;
            qn->pron->npron = 1;
            qn->pron->nsyl = 1;
            qn->pron->syls[0].tone = pron->syls[j].tone;
            qn->pron->syls[0].v = wtk_heap_dup_string(heap,pron->syls[j].v->data,pron->syls[j].v->len);
        }
    }
    return 0;
}

int qtk_tts_polyphone_ply_queue_cmp(int *i,qtk_tts_ply_idx_t *idx_q)
{
    return idx_q->idx-*i;
}

int qtk_tts_polyphone_ply_queue_pop(qtk_tts_polyphone_t *ply,int idx)
{
    qtk_tts_ply_idx_t *idx_q = NULL;
    
    idx_q = wtk_queue_find(ply->ply_qn,offsetof(qtk_tts_ply_idx_t,q_n),(wtk_cmp_handler_t)qtk_tts_polyphone_ply_queue_cmp,&idx);
    if(idx_q){
        wtk_queue_remove(ply->ply_qn,&idx_q->q_n);
        return 1;
    }else{
        return 0;
    }
}

qtk_tts_ply_idx_t* qtk_tts_polyphone_ply_queue_find(qtk_tts_polyphone_t *ply,int idx)
{
    qtk_tts_ply_idx_t *idx_q = NULL;
    
    idx_q = wtk_queue_find(ply->ply_qn,offsetof(qtk_tts_ply_idx_t,q_n),(wtk_cmp_handler_t)qtk_tts_polyphone_ply_queue_cmp,&idx);
    return idx_q;
}

int qtk_tts_polyphone_ply_format_search(qtk_tts_polyphone_t *ply,wtk_tts_info_t *info,wtk_tts_snt_t *snt)
{
    // int ret = 0;
    wtk_tts_wrd_t **wrds=NULL,*wrd=NULL;
    int wrdn = 0,i = 0,j = 0,n = 0;
    int charn = 0;
    wtk_tts_wrd_pron_t *pron = NULL;
    qtk_tts_pron_t *qn = NULL;
    wtk_heap_t *heap = info->heap;
    
    wrdn = snt->wrds->nslot;
    wrds = snt->wrds->slot;
    
    for(i = 0,charn = 0; i < wrdn;++i,charn+=wrd->pron->pron->nsyl){
        wrd = wrds[i];
        pron = wtk_kdict_get(ply->cfg->ply_dict,wrd->v->data,wrd->v->len);
        if(pron == NULL) continue;
        n = pron->nsyl;
        for(j = 0; j < n; ++j){
            qn = qtk_tts_polyphone_queue_get(ply,charn+j);
            if(qn == NULL){wtk_debug("not get queue num %.*s\n",wrd->v->len,wrd->v->data);continue;}
            qn->pron->next = NULL;
            qn->pron->npron = 1;
            qn->pron->nsyl = 1;
            qn->pron->syls[0].tone = pron->syls[j].tone;
            qn->pron->syls[0].v = wtk_heap_dup_string(heap,pron->syls[j].v->data,pron->syls[j].v->len);
            if(qtk_tts_polyphone_ply_queue_pop(ply,charn+j)){
                ply->ply_num--;
            }
        }
    }

    return 0;
}

void qtk_tts_polyphone_idx_print(qtk_tts_ply_idx_t *idx)
{
    printf("ply:%d\n",idx->idx);
    return;
}

void qtk_tts_polyphone_pron_print(qtk_tts_pron_t *pron)
{
    printf("%d:%.*s\n",pron->idx,pron->wchar->len,pron->wchar->data);
    wtk_tts_wrd_pron_print(pron->pron);
    return;
}

int qtk_tts_polyphone_model_char_ids(qtk_tts_polyphone_t *ply,char *data,char len)
{
    wtk_string_t *id = NULL;
    int ret = 0;
    id = wtk_kdict_get(ply->cfg->char_dict,data,len);
    if(id){
        ret = wtk_str_atoi(id->data,id->len);
    }
    return ret;
}

int qtk_tts_polyphone_flag2id(char *data,int len)
{
    int ret = 0;
    if(len == strlen("2") && 0==strncmp(data,"2",len)){
        ret = 1;
    }else if(len == strlen("1") && 0==strncmp(data,"1",len)){
        ret = 2;
    }else if(len == strlen("0") && 0==strncmp(data,"0",len)){
        ret = 3;
    }
    return ret;
}

int qtk_tts_polyphone_pp2id(qtk_tts_polyphone_t *ply,char *data,char len)
{
    wtk_string_t *id = NULL;
    int ret = 0;
    id = wtk_kdict_get(ply->cfg->pp_dict,data,len);
    if(id){
        ret = wtk_str_atoi(id->data,id->len);
    }
    return ret;
}

int qtk_tts_polyphone_ppandflag_is(qtk_tts_polyphone_t *ply,char *data,char len,int idx,int *flag,int *pp)
{
    wtk_tts_wrd_pron_t *pron = NULL;
    wtk_string_t *pp_s = NULL,*flag_s = NULL;
    pron = wtk_kdict_get(ply->cfg->pron_dict,data,len);

    if((len == strlen("一") && 0 ==  strncmp(data,"一",len))||(len==strlen("不") && 0 ==  strncmp(data,"不",len))){
        pp_s = wtk_string_dup_data("O",strlen("O"));
        flag_s = wtk_string_dup_data("2",strlen("2"));
    }else if(pron == NULL){
        pp_s = wtk_string_dup_data("O",strlen("O"));
        flag_s = wtk_string_dup_data("2",strlen("2"));
    }else if(pron->npron == 1){
        pp_s = wtk_string_dup_data("O",strlen("O"));
        flag_s = wtk_string_dup_data("2",strlen("2"));
    }else if(pron->npron > 1){
        pp_s = wtk_string_dup_data(data,len);
        if(qtk_tts_polyphone_ply_queue_find(ply,idx)){
            flag_s = wtk_string_dup_data("1",strlen("1"));
        }else{
            flag_s = wtk_string_dup_data("0",strlen("0"));
        }
    }
    *flag = qtk_tts_polyphone_flag2id(flag_s->data,flag_s->len);
    *pp = qtk_tts_polyphone_pp2id(ply,pp_s->data,pp_s->len);
// end:
    if(pp_s) wtk_string_delete(pp_s);
    if(flag_s) wtk_string_delete(flag_s);
    return 0;
}

/*
char_ids
cws_ids
pp_ids
flag_ids
不计算词性序列
*/
wtk_veci_t ** qtk_tts_polyphone_model_2vec_process(qtk_tts_polyphone_t *ply,wtk_tts_info_t *info,wtk_tts_snt_t *snt)
{
    wtk_heap_t *heap = info->heap;
    int charn = 0;//,j = 0;
    wtk_string_t **charts = NULL;
    int wrdn = 0,i = 0;
    wtk_tts_wrd_t **wrds = NULL,*wrd = NULL;
    wtk_veci_t **vecs = NULL;//,*vec = NULL;
    int k = 0,charln = 0;
    int pp = 0,flag = 0;

    charn = snt->chars->nslot;
    charts = snt->chars->slot;
    wrdn = snt->wrds->nslot;
    wrds = snt->wrds->slot;
    vecs = wtk_heap_malloc(info->heap,sizeof(wtk_veci_t*)*4);
    for(i = 0; i < 4;++i){
        vecs[i] = wtk_veci_heap_new(heap,charn);
    }
    for(i = 0,charln=0;i < wrdn;++i,charln+=wrd->pron->pron->nsyl){
        wrd = wrds[i];
        for(k = 0;k<wrd->pron->pron->nsyl;++k){
            vecs[0]->p[charln+k] = qtk_tts_polyphone_model_char_ids(ply,charts[charln+k]->data,charts[charln+k]->len);
            vecs[1]->p[charln+k] = qtk_tts_parse_isBEMS(k+1,wrd->pron->pron->nsyl);
            qtk_tts_polyphone_ppandflag_is(ply,charts[charln+k]->data,charts[charln+k]->len,charln+k,&flag,&pp);
            vecs[2]->p[charln+k] = pp;
            vecs[3]->p[charln+k] = flag;
            // printf("%d %d %d %d\n",vecs[0]->p[charln+k],vecs[1]->p[charln+k],vecs[2]->p[charln+k],vecs[3]->p[charln+k]);
        }
    }
    // ply->veci = vecs;
    return vecs;
}

void qtk_tts_polyphone_print(qtk_tts_polyphone_t *ply)
{
    if(ply->cfg->is_en){
        return;
    }
    if(ply->qn->length > 0){
        wtk_queue_print(ply->qn,offsetof(qtk_tts_pron_t,q_n),(wtk_print_handler_t)qtk_tts_polyphone_pron_print);
    }else{
        wtk_debug("dont have pron");
    }
    if(ply->ply_num > 0){
        printf("have ply %d\n",ply->ply_num);
        wtk_queue_print(ply->ply_qn,offsetof(qtk_tts_ply_idx_t,q_n),(wtk_print_handler_t)qtk_tts_polyphone_idx_print);
    }else{
        wtk_debug("dont have ply idx\n");
    }
    return;
}

void qtk_tts_polyphone_arg_max(qtk_tts_polyphone_t *ply,wtk_matf_t *in,wtk_veci_t *out)
{
    int len = out->len,i = 0;
    for(i = 0; i < len; ++i){
        out->p[i] = wtk_float_argmax(wtk_matf_at(in,i,0),in->col);
    }
    return;
}

qtk_tts_polyphone_char_t* qtk_tts_polyphone_model_charid_get(qtk_tts_polyphone_t *ply,int id)
{
    qtk_tts_polyphone_char_t *chart = NULL;
    char ids[126] = {0,};
    sprintf(ids,"%d",id);
    chart = wtk_kdict_get(ply->cfg->poly_id_dict,ids,strlen(ids));
    return chart;
}

void qtk_tts_polyphone_model_find_change_charid(qtk_tts_polyphone_t *ply,wtk_tts_info_t *info,wtk_veci_t *veci)
{
    qtk_tts_ply_idx_t *idx = NULL;
    wtk_queue_node_t *qn = NULL;
    qtk_tts_polyphone_char_t *chart=NULL;
    qtk_tts_pron_t *pron = NULL;
    while(1){
        qn = wtk_queue_pop(ply->ply_qn);
        if(qn == NULL) break;
        ply->ply_num--;
        idx = data_offset(qn,qtk_tts_ply_idx_t,q_n);
        chart = qtk_tts_polyphone_model_charid_get(ply,veci->p[idx->idx]);
        if(chart == NULL){
            wtk_debug("error err idx %d\n",veci->p[idx->idx]);
            continue;
        }
        //change
        pron = qtk_tts_polyphone_queue_get(ply,idx->idx);
        if(wtk_string_cmp2(pron->wchar,chart->chart)){
            wtk_debug("error char is diff [%.*s] [%.*s]\n",pron->wchar->len,pron->wchar->data,chart->chart->len,chart->chart->data);
            continue;
        }
        pron->pron->next = NULL;
        pron->pron->npron = 1;
        pron->pron->nsyl = 1;
        pron->pron->syls[0].v = wtk_heap_dup_string(info->heap,chart->pron->syls[0].v->data,chart->pron->syls[0].v->len);
        pron->pron->syls[0].tone = chart->pron->syls[0].tone;
    }
    return;
}

/*
    使用的发音变调规则
    协同发音 or 语流音变: 33 -> 23（将两个连续的第三声改为，第一个二声，第二个第三声） 、 333 -> 223
    不的特殊规则: 四声之前变二声，一二三声之前变四声。
    一的特殊规则: 四声之前变二声，一二三声之前变四声。/ 做序数词用的时候不变调
*/
//一的特殊发音需要判断是不是字符
int qtk_tts_polyphone_modefication_is_num(char *data, int len)
{
    static char *nums[] = {"一","二","三","四","五","六","七","八","九","十","零"};
    int i = 0;
    int n = sizeof(nums)/sizeof(char*);
    for(i = 0; i < n; ++i){
        if(len == strlen(nums[i]) && strncmp(nums[i],data,len)){
            return 1;
        }
    }
    return 0;
}

int qtk_tts_polyphone_modification(qtk_tts_polyphone_t *ply,wtk_tts_info_t *info,wtk_tts_snt_t *snt)
{
    qtk_tts_pron_t *pron = NULL,*pron_pre = NULL,*pron_pre_pre = NULL;
    int n = snt->chars->nslot,i = 0;
    wtk_tts_syl_t *syl = NULL;
    
    for(i = 0; i < n; ++i){
        pron = qtk_tts_polyphone_queue_get(ply,i);
        if(pron == NULL){   //因为chars可能有标点符号，但是拼音序列没有木有收录标点
            pron_pre = NULL;
            continue;
        }
        if(pron_pre == NULL){
            pron_pre = pron;
            continue;
        }
        // wtk_debug("%.*s\n",pron_pre->wchar->len,pron_pre->wchar->data);
        syl = pron_pre->pron->syls+0;
        if(0==wtk_string_cmp_s(pron_pre->wchar,"不")){
            if(pron->pron->syls[0].tone == 4){
                syl->tone = 2;
            }else{
                syl->tone = 4;
            }
        }else if( 0==wtk_string_cmp_s(pron_pre->wchar,"一")){
            if(i > 1 && qtk_tts_polyphone_modefication_is_num(pron_pre_pre->wchar->data,pron_pre_pre->wchar->len)){
                break;
            }else if(pron->pron->syls[0].tone == 4){
                syl->tone = 2;
            }else{
                syl->tone = 4;
            }
        }else if(syl->tone == 3 && pron->pron->syls[0].tone == 3){
            syl->tone = 2;
        }
        pron_pre_pre = pron_pre;
        pron_pre = pron;
    }
    return 0;
}
//新生成的拼音放到wrd里面
int qtk_tts_polyphone_setto(qtk_tts_polyphone_t *ply,wtk_tts_info_t *info,wtk_tts_snt_t *snt)
{
    int ret = 0;
    int n = snt->wrds->nslot,i = 0,j = 0,step = 0;
    wtk_tts_wrd_t **wrds = NULL,*wrd = NULL;
    wtk_tts_wrd_pron_t *pron = NULL;
    qtk_tts_pron_t *pron_q = NULL;
    int npron = 0,charid = 0,charids = 0;

    charids = snt->chars->nslot;
    wrds = snt->wrds->slot;
    for(i = 0,charid = 0; i < n && charid < charids; i+=step){
        wrd = wrds[i];
        npron = wrd->pron->pron->nsyl;  //表示一个词里有多少个chart
        if(wrd->sil){   //本词跳过
            step = 1;
            charid += npron;
            continue;
        }
        pron = wtk_heap_malloc(info->heap,sizeof(wtk_tts_wrd_pron_t));
        pron->npron = 1;
        pron->next = NULL;
        pron->nsyl = npron;
        pron->syls = wtk_heap_malloc(info->heap,sizeof(wtk_tts_syl_t)*npron);
        for(j= 0;j<npron;++j){
            pron_q = qtk_tts_polyphone_queue_get(ply,charid+j);
            if(pron_q == NULL){ //如果没有音标就使用sil标志 （一般出现这种情况是标点符号）
                pron->syls[j].v = wtk_heap_dup_string(info->heap,"SIL",strlen("SIL"));
                pron->syls[j].tone = 1;
            }else{
                pron->syls[j].tone = pron_q->pron->syls[0].tone;
                pron->syls[j].v = wtk_heap_dup_string(info->heap,pron_q->pron->syls[0].v->data,pron_q->pron->syls[0].v->len);
            }
        }
        wrd->pron->pron = pron;
        charid+=npron;
        step = 1;
    }

    return ret;
}
