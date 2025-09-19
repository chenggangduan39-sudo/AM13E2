#include "qtk_tts_prosody.h"

#include "qtk_tts_parse_comm.h"

int qtk_tts_prosody_is_filter(qtk_tts_prosody_t *prosody,char *sym,int len);
int qtk_tts_prosody_chn_getcwl(qtk_tts_prosody_t *prosody,wtk_tts_snt_t *snt);
int qtk_tts_prosody_chn_getswl(qtk_tts_prosody_t *prosody,wtk_tts_snt_t *snt);
int qtk_tts_prosody_dict_get_id(qtk_tts_prosody_t *prosody,char *sym,int len);
int qtk_tts_prosody_symbols2_get(wtk_array_t *arr,int i);
void qtk_tts_prosody_chn_2list(qtk_tts_prosody_t *prosody,wtk_tts_info_t *info,wtk_tts_lab_t *lab);
// int qtk_tts_prosody_is_BEMS(int i,int len);
int qtk_tts_prosody_chart_inword(char *data,int clen,char *word,int wlen);

qtk_tts_prosody_t *qtk_tts_prosody_new(qtk_tts_prosody_cfg_t *cfg, wtk_rbin2_t* rbin)
{
    qtk_tts_prosody_t *prosody = NULL;
    
    prosody = wtk_malloc(sizeof(*prosody));
    prosody->cfg = cfg;
    if(prosody->cfg->use_ncrf){
        prosody->ncrf = qtk_tts_ncrf_new2(&cfg->ncrf, rbin);
    }
    return prosody;
}

/* 
BMES 格式 忽略词性
我 [posi]Sin [pos]r [cpf]1 [cpb]1 [cws_pf]0 [cws_pb]5 <pad> <pad> <pad>
妹 [posi]Beg [pos]n [cpf]1 [cpb]2 [cws_pf]1 [cws_pb]4 <pad> <pad> <pad>
妹 [posi]End [pos]n [cpf]2 [cpb]1 [cws_pf]1 [cws_pb]4 <pad> <pad> <pad>
很 [posi]Sin [pos]d [cpf]1 [cpb]1 [cws_pf]2 [cws_pb]3 <pad> <pad> <pad>
好 [posi]Sin [pos]a [cpf]1 [cpb]1 [cws_pf]3 [cws_pb]2 <pad> <pad> <pad>
谢 [posi]Beg [pos]v [cpf]1 [cpb]2 [cws_pf]4 [cws_pb]1 <pad> <pad> <pad>
谢 [posi]End [pos]v [cpf]2 [cpb]1 [cws_pf]4 [cws_pb]1 <pad> <pad> <pad>
关 [posi]Beg [pos]v [cpf]1 [cpb]2 [cws_pf]5 [cws_pb]0 <pad> <pad> <pad>
心 [posi]End [pos]v [cpf]2 [cpb]1 [cws_pf]5 [cws_pb]0 <pad> <pad> <pad>
*/
int qtk_tts_prosody_2symbols_id(qtk_tts_prosody_t *prosody,wtk_tts_info_t *info,wtk_tts_lab_t  *lab)
{
    int ret = -1,sw_len = 0,sc_len = 0,retw = 0,step=0;
    wtk_tts_syl_t *s = NULL;
    wtk_tts_snt_t **snts = NULL,*snt = NULL;
    wtk_tts_wrd_t *wrd = NULL,**wrds = NULL;
    int nsnt = 0,i = 0,j = 0;
    int nwrd = 0;
    wtk_mati_t **matis = NULL,*mati = NULL;
    int chl = 0,u=0,charn = 0,wordn = 0;;
    int *chi = NULL;
    wtk_string_t **w_chars = NULL,*w_char = NULL;

    nsnt = lab->snts->nslot;
    snts = lab->snts->slot;
    matis = wtk_heap_malloc(info->heap,sizeof(wtk_mati_t*)*nsnt);
    prosody->nids = nsnt;

    for(i = 0; i < nsnt; ++i){
        snt = snts[i];
        sw_len = qtk_tts_prosody_chn_getswl(prosody,snt);
        sc_len = qtk_tts_prosody_chn_getcwl(prosody,snt);
        mati = wtk_mati_heap_new(info->heap,sc_len,6); //L*6 
        nwrd = snt->wrds->nslot;
        wrds = snt->wrds->slot;
        chl = 0;wordn = 0;
        w_chars = snt->chars->slot;
        if (nwrd > 44)
        {
        	wtk_debug("sentence is too long without snt-end punctuation, maybe over max number of words[44]\n");
        	ret=-1;
        	goto end;
        }
        for(j = 0; j < nwrd; ++j){
            wrd = wrds[j];
            // wtk_debug("%.*s\n",wrd->v->len,wrd->v->data);
            if(qtk_tts_prosody_is_filter(prosody,wrd->v->data,wrd->v->len)){
                chl+=wrd->pron->pron->nsyl;
                continue;
            }
            for(u=0,s=wrd->pron->pron->syls;u < wrd->pron->pron->nsyl;u+=step,++s)
            {
                chi = mati->p+mati->col*charn;
                w_char = w_chars[chl];
                // wtk_debug("%.*s\n",w_char->len,w_char->data);
                if(qtk_tts_prosody_is_filter(prosody,w_char->data,w_char->len)){
                    step=1;
                    retw = qtk_tts_prosody_chart_inword(w_char->data,w_char->len,wrd->v->data,wrd->v->len);
                    if(retw < 0){   //可能出现再句子里有但是分词里面没有的符号
                        step=0;
                    }
                    chl++;
                    continue;
                }
                chi[0] = qtk_tts_prosody_dict_get_id(prosody,w_char->data,w_char->len);  //char id
                //得到BMES
                chi[1] = qtk_tts_parse_isBEMS(u+1,wrd->pron->pron->nsyl)+3;    //BMES id
                chi[2] = qtk_tts_prosody_symbols2_get(prosody->cfg->symbols2,u);   //在词中的位置
                chi[3] = qtk_tts_prosody_symbols2_get(prosody->cfg->symbols3,wrd->pron->pron->nsyl - u -1);   //反向在词中的位置
                //printf("wordn=%d %d\n", wordn, ((int*)(prosody->cfg->symbols4->slot))[wordn]);
                chi[4] = qtk_tts_prosody_symbols2_get(prosody->cfg->symbols4,wordn);   //词在句中的位置
                //printf("sw_len - wordn=%d %d\n", sw_len - wordn, ((int*)(prosody->cfg->symbols5->slot))[sw_len - wordn-1]);
                chi[5] = qtk_tts_prosody_symbols2_get(prosody->cfg->symbols5,sw_len - wordn - 1); //词在句中的位置反向
                chl++;
                charn++;
                step=1;
                //printf("[%.*s] %d %d %d %d %d %d\n",w_char->len,w_char->data,chi[0],chi[1],chi[2],chi[3],chi[4],chi[5]);
            }
            wordn++;
        }
        matis[i] = mati;
        if(charn != sc_len){
            wtk_debug("%d %d char num error\n",charn,sc_len);
            goto end;
        }
        charn = 0; //使用了多少个字符
    }
    ret = 0;
    prosody->chn_symbols_id = matis;
end:
    return ret;
}

int qtk_tts_prosody_id2ncrf(qtk_tts_prosody_t *prosody,wtk_tts_info_t *info)
{
    wtk_veci_t **vecs = NULL,*vec = NULL;
    int nids = prosody->nids,i = 0;
    wtk_mati_t **matis = NULL,*mati = NULL;
    
    vecs = wtk_heap_malloc(info->heap,sizeof(wtk_veci_t*)*nids);
    matis = prosody->chn_symbols_id;
    for(i = 0; i < nids; ++i){
        mati = matis[i];
        vec = wtk_veci_heap_new(info->heap,mati->row);
        if(vec->len > 0){
            qtk_tts_ncrf_process(prosody->ncrf,mati,vec);
        }
        vecs[i] = vec;
    }
    prosody->rhythm_vec = vecs;
    return 0;
}

int qtk_tts_prosody_process(qtk_tts_prosody_t *prosody,wtk_tts_info_t *info,wtk_tts_lab_t *lab)
{
    int ret = -1;
    if(prosody->cfg->use_ncrf){
        ret = qtk_tts_prosody_2symbols_id(prosody,info,lab);
        if(ret != 0) goto end;
        qtk_tts_prosody_id2ncrf(prosody,info);
        qtk_tts_prosody_chn_2list(prosody,info,lab);
    }
    ret = 0;
end:
    return ret;
}

int qtk_tts_prosody_delete(qtk_tts_prosody_t *prosody)
{
    if(prosody == NULL)
        return -1;
    if(prosody->cfg->use_ncrf){
        qtk_tts_ncrf_delete(prosody->ncrf);
    }
    wtk_free(prosody);
    return 0;
}

// 测单句的词个数
int qtk_tts_prosody_chn_getswl(qtk_tts_prosody_t *prosody,wtk_tts_snt_t *snt)
{
    int ret = 0;
    int i = 0;
    wtk_tts_wrd_t **wrds = NULL;
    int nwrds =0;

    nwrds = snt->wrds->nslot;
    wrds = snt->wrds->slot;

    for(i = 0; i < nwrds; ++i){
        if(0 == qtk_tts_prosody_is_filter(prosody,wrds[i]->v->data,wrds[i]->v->len)){
            ret ++;
        }
    }

    return ret;
}
// 字个数
int qtk_tts_prosody_chn_getcwl(qtk_tts_prosody_t *prosody,wtk_tts_snt_t *snt)
{
    int ret = 0;
    int i = 0;
    int n = 0;
    wtk_string_t **string = NULL,*str = NULL;

    string = snt->chars->slot;
    n = snt->chars->nslot;

    for(i = 0; i < n; ++i){
        str = string[i];
        if(0 == qtk_tts_prosody_is_filter(prosody,str->data,str->len)){
            ret ++;
        }
    }

    return ret;
}

int qtk_tts_prosody_is_filter(qtk_tts_prosody_t *prosody,char *sym,int len)
{
    int ret = 0;
    wtk_string_t **strs = NULL;
    int i = 0,nstr = 0;
    nstr = prosody->cfg->filter->nslot;
    strs = prosody->cfg->filter->slot;
    for(i = 0;i < nstr; ++i){
        if(len == strs[i]->len && !wtk_string_cmp(strs[i],sym,len)){
            ret = 1;
            break;
        }
    }
    return ret;
}

int qtk_tts_prosody_dict_get_id(qtk_tts_prosody_t *prosody,char *sym,int len)
{
    int ret = 0;
    wtk_string_t *str = NULL;

    str = wtk_kdict_get(prosody->cfg->dict,sym,len);
    if(str){
        ret = wtk_str_atoi(str->data,str->len);
    }else{
        ret = 1;
    }
    return ret;
}

int qtk_tts_prosody_symbols2_get(wtk_array_t *arr,int i)
{
    int *ids = 0 ,id = 0;
    ids = arr->slot;

    id = ids[i];
    return id;
}

void qtk_tts_prosody_chn_2list(qtk_tts_prosody_t *prosody,wtk_tts_info_t *info,wtk_tts_lab_t *lab)
{
    int i = 0,j = 0,k = 0,n = 0;
    wtk_veci_t **rhy_vecs = NULL,*vec = NULL;
    wtk_tts_snt_t **snts = NULL,*snt = NULL;
	wtk_tts_wrd_t **wrds,*wrd;
    wtk_tts_syl_t *s = NULL;
    wtk_strbuf_t *buf = NULL;
    
    snts = lab->snts->slot;
    rhy_vecs = prosody->rhythm_vec;
    buf = wtk_strbuf_new(1024,1.0f);
    prosody->prosody_list = wtk_heap_malloc(info->heap,sizeof(wtk_string_t*)*prosody->nids);
    for(i = 0; i < prosody->nids; i++){
        snt = snts[i];
        vec = rhy_vecs[i];
        n = 0;
        wtk_strbuf_reset(buf);
        if(vec->len == 0){   //如果出现空句子就调过
            printf("prosody skip emtpy string\n");
            prosody->prosody_list[i] = NULL;
            continue;
        }
        if(prosody->cfg->use_start_sil){
            wtk_strbuf_push(buf,"SIL ",strlen("SIL "));
            //wtk_strbuf_push(buf,"_ ",strlen("_ "));
            wtk_strbuf_push(buf, prosody->cfg->split_sym, strlen(prosody->cfg->split_sym));
        }
        for(j = 0; j < snt->wrds->nslot; ++j){
            wrds = snt->wrds->slot;
            for(k = 0,wrd = wrds[j];k < wrd->pron->pron->nsyl;++k){
                if(qtk_tts_prosody_is_filter(prosody,wrd->v->data,wrd->v->len)){
                    if(wrd->v->len == strlen("、") && !strncmp(wrd->v->data,"、",wrd->v->len) && buf->pos > 2){ //提升下顿号的等级
                        buf->pos-=2;
                        wtk_strbuf_push(buf,"# ",strlen("# "));
                    }
                    continue;
                }
                s = wrd->pron->pron->syls+k;
                wtk_strbuf_push(buf,s->v->data,s->v->len);
                if(s->v->len != strlen("SIL") || strncmp(s->v->data,"SIL",s->v->len)){
                    wtk_strbuf_push_c(buf,s->tone+'0');
                }
                wtk_strbuf_push_c(buf,' ');
                if(vec->p[n] == 2){
                    wtk_strbuf_push(buf,"@ ",strlen("@ "));
                }else if(vec->p[n] == 3){
                    wtk_strbuf_push(buf,"# ",strlen("# "));
                }else if(vec->p[n] == 4){
                    wtk_strbuf_push(buf,"$ ",strlen("$ "));
                }else{
                    //wtk_strbuf_push(buf,"_ ",strlen("_ "));
                    wtk_strbuf_push(buf, prosody->cfg->split_sym, strlen(prosody->cfg->split_sym));
                }
                n++;
            }
            if (prosody->cfg->use_segwrd_sym)
            	wtk_strbuf_push(buf,"| ",strlen("| "));
        }

        if(prosody->cfg->use_end_sil){
            wtk_strbuf_push(buf,"SIL ",strlen("SIL "));
            //wtk_strbuf_push(buf,"_ ",strlen("_ "));
            wtk_strbuf_push(buf, prosody->cfg->split_sym, strlen(prosody->cfg->split_sym));
        }

        if(prosody->cfg->use_tail_sym)
        {
        	wtk_strbuf_push(buf,"~ ",strlen("~ ")); //补充一个结尾
        }
        // wtk_debug("list: [%.*s]\n",buf->pos,buf->data);
        prosody->prosody_list[i] = wtk_heap_dup_string(info->heap,buf->data,buf->pos);
    }
    wtk_strbuf_delete(buf);
    return;
}

int qtk_tts_prosody_print(qtk_tts_prosody_t *prosody)
{
    wtk_mati_t **ids = NULL;
    int i = 0,j = 0;
    wtk_mati_t *mati = NULL;
    wtk_veci_t **rhy_vecs = NULL,*vec = NULL;
    int *all_id = NULL;

    if(prosody->cfg->use_ncrf == 0 ) return 0;
    ids = prosody->chn_symbols_id;
    rhy_vecs = prosody->rhythm_vec;
    for(i = 0; i < prosody->nids; i++){
        mati = ids[i];
        for(j = 0; j < mati->row; ++j){
            all_id = mati->p+mati->col*j;
            printf("id %d posi %d cpf %d cpb %d cws_pf %d cws_pb %d \n",
                all_id[0],all_id[1],all_id[2],all_id[3],all_id[4],all_id[5]);
        }
        vec = rhy_vecs[i];
        for(j = 0; j < vec->len;++j){
            printf("%d ",vec->p[j]);
        }
        puts("");
        if(prosody->prosody_list[i]) printf("prosody:[%.*s]\n",prosody->prosody_list[i]->len,prosody->prosody_list[i]->data);
    }

    return 0;
}

int qtk_tts_prosody_chart_inword(char *data,int clen,char *word,int wlen)
{
    char *s,*e;
    int n = 0,ret = -1,l = 0;
    s = word;e=s+wlen;
    
    while(s < e){
        n = wtk_utf8_bytes(*s);
        if(n == clen && !strncmp(data,word,n)){
            ret = l;
            break;
        }
        s += n;
        l++;
    }
    return ret;
}
