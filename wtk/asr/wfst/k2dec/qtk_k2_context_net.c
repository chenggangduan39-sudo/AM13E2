#include "qtk_k2_context_net.h"
wtk_dict_word_t* qtk_k2_context_net_get_word(qtk_k2_context_net_t *net,char *wrd,int wrd_bytes){
    wtk_dict_word_t *dw;

    dw = wtk_kvdict_get_word(net->dict.kv,wrd,wrd_bytes);
    return dw;
}

qtk_k2_context_net_t* qtk_k2_context_net_new(qtk_k2_wrapper_cfg_t* cfg){
    qtk_k2_context_net_t *net = (qtk_k2_context_net_t*)wtk_malloc(sizeof(qtk_k2_context_net_t));
    net->cfg = cfg;
    net->heap = wtk_heap_new(4096);
    net->net = wtk_fst_net_new(&(cfg->net));
    net->tmp_net = wtk_fst_net2_new(&(cfg->net));
    wtk_slist_init(&(net->state_l));
    net->out_sym = 0;
    net->label = 0;
    if(cfg->dict_fn){
        net->label=wtk_label_new(25007);
        if(cfg->rbin2){
            wtk_rbin2_item_t *item;

            item=wtk_rbin2_get(cfg->rbin2,cfg->dict_fn,strlen(cfg->dict_fn));
            if(!item){
                wtk_debug("[%s] not found\n",cfg->dict_fn);
                return NULL;
            }
            net->dict.kv=wtk_kvdict_new2(net->label,cfg->wrd_hash_hint,cfg->phn_hash_hint,cfg->wrd_hash_hint,
                cfg->rbin2->f,item->pos,item->len);
        }else{
            net->dict.kv=wtk_kvdict_new(net->label,cfg->dict_fn,cfg->wrd_hash_hint,cfg->phn_hash_hint,cfg->wrd_hash_hint);
        }
        wtk_queue_init(&(net->keywrd_q));
    }
    net->last_outid = cfg->last_outid;
    net->xbnf = NULL;
    if(cfg->use_ebnf){
        net->xbnf = wtk_xbnfnet_new(&cfg->xbnfnet,NULL);
        net->sym.ths = net;
        net->sym.get_word=(wtk_egram_sym_get_word_f)qtk_k2_context_net_get_word;
        net->xbnf->sym = &net->sym;
        net->xbnf->use_wrd = 1;
        net->text = wtk_strbuf_new(1024,1);
        net->tmp_net2 = wtk_fst_net2_new(&(cfg->net));
    }
    return net;
}

int qtk_k2_context_net_start(qtk_k2_context_net_t* net){
    return 0;
}

void qtk_k2_context_net_reset2(qtk_k2_context_net_t* net){
    //wtk_heap_reset(net->heap);
    wtk_fst_net_reset(net->net);
    wtk_fst_net2_reset(net->tmp_net);
    wtk_slist_init(&(net->state_l));
}

void qtk_k2_context_net_reset(qtk_k2_context_net_t* net){
    wtk_heap_reset(net->heap);
    wtk_fst_net_reset(net->net);
    wtk_fst_net2_reset(net->tmp_net);
    wtk_slist_init(&(net->state_l));
    if(net->cfg->dict_fn){
        wtk_kvdict_reset(net->dict.kv);
    }
    if(net->xbnf){
        wtk_xbnfnet_reset(net->xbnf);
        wtk_strbuf_reset(net->text);
    }
}

void qtk_k2_context_net_delete(qtk_k2_context_net_t* net){
    wtk_heap_delete(net->heap);
    wtk_fst_net_delete(net->net);
    wtk_fst_net2_delete(net->tmp_net);
    if(net->cfg->dict_fn){
        wtk_kvdict_delete(net->dict.kv);
    }
    if(net->label){
        wtk_label_delete(net->label);
    }
    if(net->out_sym){
        wtk_free(net->out_sym);
    }
    if(net->xbnf){
        wtk_xbnfnet_delete(net->xbnf);
        wtk_strbuf_delete(net->text);
        wtk_fst_net2_delete(net->tmp_net2);
    }
    wtk_free(net);
}

wtk_fst_state2_t* qtk_k2_context_net_pop_state(qtk_k2_context_net_t *net){
    wtk_fst_state2_t *state;

    state = wtk_fst_net2_pop_state(net->tmp_net);
    wtk_slist_push(&(net->state_l),&(state->q_n));
    return state;
}

static wtk_fst_state2_t* qtk_k2_context_net_find_and_merge_state(wtk_fst_net2_t* net,
wtk_fst_state2_t *from,int in_id){
    wtk_fst_trans2_t *trans;

    for(trans=(wtk_fst_trans2_t*)from->v.trans;trans;trans=(wtk_fst_trans2_t*)trans->hook.next){
        if(trans->in_id == in_id){
            return (wtk_fst_state2_t*)trans->to_state;
        }
    }
    return NULL;
}

static wtk_fst_state2_t* qtk_k2_context_net_find_and_merge_state_keyword(wtk_fst_net2_t* net,
wtk_fst_state2_t *from,int in_id){
    wtk_fst_trans2_t *trans;

    for(trans=(wtk_fst_trans2_t*)from->v.trans;trans;trans=(wtk_fst_trans2_t*)trans->hook.next){
        if(trans->in_id == in_id && trans->to_state->id != 0){
            return (wtk_fst_state2_t*)trans->to_state;
        }
    }
    return NULL;
}

void qtk_k2_context_net_dump(qtk_k2_context_net_t* net){
    if(!net->tmp_net->start){
        return;
    }
    wtk_fst_trans_t *trans,*tt;
    wtk_fst_state2_t *s2;
    wtk_fst_state_t *s;
    wtk_slist_node_t *sn;
    int vi = net->tmp_net->state_id;
    int i,cnt,e_id;

    net->net->nrbin_states = vi;
    net->net->rbin_states = (wtk_fst_state_t*)wtk_calloc(vi,sizeof(wtk_fst_state_t));
    for(i = 0;i < vi; i++){
        wtk_fst_state_init(net->net->rbin_states + i,i);
    }

    for(sn = net->state_l.prev;sn;sn = sn->prev){
        s2=data_offset(sn,wtk_fst_state2_t,q_n);
        s=net->net->rbin_states+s2->id;
        if(s2->type == WTK_FST_FINAL_STATE){
            s->type = WTK_FST_FINAL_STATE;
            s->ntrans = 0;
        }else{
            s->type=WTK_FST_NORM_STATE;
            for(cnt = 0,trans = s2->v.trans;trans;trans = trans->hook.next,++cnt);
            if(cnt > 0){
                s->ntrans = cnt;
                s->v.trans = (wtk_fst_trans_t*)wtk_heap_malloc(net->heap,cnt*sizeof(wtk_fst_trans_t));
                for(i = 0,trans = s2->v.trans;trans;trans = trans->hook.next,++i){
                    e_id = trans->to_state->id;
                    tt = s->v.trans + i;
                    tt->in_id = trans->in_id;
                    tt->out_id = trans->out_id;
                    tt->weight = trans->weight;
                    tt->hook.inst = NULL;
                    tt->to_state = net->net->rbin_states + e_id;
                }
            }else{
                s->ntrans = 0;
            }
            if(s2->type == WTK_FST_NORMAL_ARRAY_STATE){
                s->type = WTK_FST_NORMAL_ARRAY_STATE;
            }
        }
    }
    net->net->init_state = net->net->rbin_states + net->tmp_net->start->id;
}

int qtk_k2_context_net_build(qtk_k2_context_net_t* net,char *data, int len){

    if(net->net->rbin_states){
        wtk_free(net->net->rbin_states);
        net->net->rbin_states = NULL;
    }

    wtk_array_t *a;
    wtk_string_t **strs;
    wtk_string_t v;
    int i,j,n,in_id,cnt=0;
    char *s,*e;
    wtk_fst_net2_t* tmp_net = net->tmp_net;
    wtk_fst_state2_t *state,*from,*prev;
    wtk_fst_sym_t *sym = net->cfg->sym;
    float gain = net->cfg->hot_gain * 1.0;

    state = qtk_k2_context_net_pop_state(net);
    tmp_net->start = state;
	tmp_net->end = qtk_k2_context_net_pop_state(net);
	tmp_net->end->type = WTK_FST_FINAL_STATE;

    if(len < 1){
        return 0;
    }

    a = wtk_str_to_array(net->heap, data, len, '|');
    strs = (wtk_string_t**) a->slot;
    for(i = 0,j = 0; i < a->nslot; i++){
		s = strs[i]->data;
		e = s + strs[i]->len;
        from = tmp_net->start;
        prev = tmp_net->start;
        j = 0;
		while(s < e){
            j++;
            //wtk_debug("==========%d\n",j);
			n = wtk_utf8_bytes(*s);
            v.data = s;
            v.len = n;
            in_id = wtk_fst_sym_get_index(sym,&v);
            //wtk_debug("%.*s %d %d %d\n",n,s,in_id,n,strs[i]->len);
            s += n;
            if(i + 1 == a->nslot){
                cnt += n;
                if(cnt + 1 == strs[i]->len){
                    s += 1;
                }
            }
            if(in_id == -1){
                return -1;
            }
            //wtk_debug("====== %p %d\n",from,prev->id);
            if(from){
                from = qtk_k2_context_net_find_and_merge_state(tmp_net,from,in_id);
                //wtk_debug("====== %p\n",from);
                if(!from){
                    state = qtk_k2_context_net_pop_state(net);
                    wtk_fst_net2_link_state(tmp_net,prev,state,0,in_id,in_id,0.0,gain);
                    //wtk_debug("%d %d %d %d %f\n",prev->id,state->id,in_id,in_id,gain);
                    wtk_fst_net2_link_state(tmp_net,state,tmp_net->start,0,0,0,0.0,-1.0*j*gain);
                    //wtk_debug("%d %d %d %d %f\n",state->id,tmp_net->start->id,0,0,-1.0*j*gain);
                    prev = state;
                }else{
                    prev = from;
                    //wtk_debug("%d\n",from->id);
                }
            }else{
                state = qtk_k2_context_net_pop_state(net);
                wtk_fst_net2_link_state(tmp_net,prev,state,0,in_id,in_id,0.0,gain);
                //wtk_debug("%d %d %d %d %f\n",prev->id,state->id,in_id,in_id,gain);
                if(s < e){ 
                    wtk_fst_net2_link_state(tmp_net,state,tmp_net->start,0,0,0,0.0,-1.0*j*gain);
                    //wtk_debug("%d %d %d %d %f\n",state->id,tmp_net->start->id,0,0,-1.0*j*gain);
                }else{
                    wtk_fst_net2_link_state(tmp_net,state,tmp_net->start,0,0,0,0.0,0.0);
                    //wtk_debug("%d %d %d %d %f\n",state->id,tmp_net->start->id,0,0,0.0);
                    state->type = WTK_FST_NORMAL_ARRAY_STATE;
                }
                prev = state;
            }
        }
    }
    qtk_k2_context_net_dump(net);
    return 0;
}

int qtk_k2_context_net_search(qtk_k2_context_net_t* net, float *score, int stateID, int id){
    wtk_fst_state_t *state;
    wtk_fst_trans_t *trans;
    int i;
    float eps_score = 0.0;
    state = net->net->rbin_states + stateID;
    for (i = 0, trans = state->v.trans; i < state->ntrans; ++i, ++trans){
        if(trans->in_id == id){
            *score = trans->weight;
            if(trans->to_state->type == WTK_FST_NORMAL_ARRAY_STATE){
                return net->net->init_state->id;
            }else{
                return trans->to_state->id;
            }
        }
        if(trans->in_id == 0){
            eps_score = trans->weight;
        }
    }
    *score = eps_score;
    return net->net->init_state->id;
}

void qtk_k2_context_net_search_keyword(qtk_k2_context_net_t* net, float *score, int* stateID, int id, int *out, int* stateID2){
    wtk_fst_state_t *state;
    wtk_fst_trans_t *trans;
    int i,j,k = 1,m,flag = 0;
    float eps_score = 0.0;
    *out = 0;
    *score = eps_score;
    for(j = 0; j < 5; j++){
        if(stateID[j] >= 0){
            flag = 0;
            state = net->net->rbin_states + stateID[j];
            *out = 0;
            for (i = 0, trans = state->v.trans; i < state->ntrans; ++i, ++trans){
                //wtk_debug("%d %d %d %d\n",stateID[j],trans->to_state->id,trans->in_id,id);
                if(trans->in_id == id){
                    *score = trans->weight;
                    if(trans->out_id != 0){
                        *out = trans->out_id;
                        //wtk_debug("%d\n",*out);
                    }
                    // if(trans->to_state->type == WTK_FST_NORMAL_ARRAY_STATE){
                    //     wtk_debug("%d %d\n",stateID,trans->to_state->id);
                    //     return net->net->init_state->id;
                    // }else{
                    for(m = 0; m < k; m++){
                        if(stateID2[m] == trans->to_state->id){
                            //wtk_debug("%d %d %d\n",stateID2[m],trans->to_state->id,m);
                            flag = 1;
                        }
                    }
                    if(!flag){
                        stateID2[k] = trans->to_state->id;
                        //wtk_debug("%d %d\n",k,trans->to_state->id);
                        k++;
						if(k>4){
                           wtk_debug("??????????? %d\n",k);
						}
                    }
                    // if(k == 0){
                    //     stateID2[k] = trans->to_state->id;
                    //     k++;
                    // }else{
                    //     if(stateID2[0] != trans->to_state->id){
                    //         stateID2[k] = trans->to_state->id;
                    //     }
                    // }
                    //}
                }
                if(trans->in_id == 0){
                    eps_score = trans->weight;
                }
            }
            //*score = eps_score;
        }
    }

    if(k == 1 && flag == 0){
        //stateID2[k] == 0;
        //wtk_debug("%f %f\n",*score,eps_score);
        *score = eps_score;
    }
}

int qtk_k2_context_net_build_keyword(qtk_k2_context_net_t* net,char *data, int len){

    if(net->net->rbin_states){
        wtk_free(net->net->rbin_states);
        net->net->rbin_states = NULL;
    }
    wtk_array_t *a;
    wtk_string_t **strs;
    int i,in_id,out_id;
    wtk_fst_net2_t* tmp_net = net->tmp_net;
    wtk_fst_state2_t *state,*from,*prev;
    wtk_fst_sym_t *sym = net->cfg->sym;
    float gain = net->cfg->hot_gain * 1.0;

    state = qtk_k2_context_net_pop_state(net);
    tmp_net->start = state;
    //tmp_net->end = qtk_k2_context_net_pop_state(net );
    //tmp_net->end->type = WTK_FST_FINAL_STATE;

    wtk_strbuf_t* buf = wtk_strbuf_new(100,1);
    char *s = data,*e = data + len;

    while(s < e){
        if(*s == '\n' || *s == EOF){
            a = wtk_str_to_array(net->heap, buf->data, buf->pos, ' ');
            strs = (wtk_string_t**) a->slot;

            from = tmp_net->start;
            prev = tmp_net->start;
            for(i = 0; i < a->nslot; i++){
                //wtk_debug("%d\n",i);
                if(i == 0){
                    out_id = wtk_str_atoi(strs[i]->data,strs[i]->len);
                    //wtk_debug("%d\n",out_id);
                }else{
                    in_id = wtk_fst_sym_get_index(sym,strs[i]);
                    //wtk_debug("====== %p %d\n",from,prev->id);
                    if(from){
                        from = qtk_k2_context_net_find_and_merge_state_keyword(tmp_net,from,in_id);
                        //wtk_debug("====== %p\n",from);
                        if(!from){
                            if(i < a->nslot - 1){
                                state = qtk_k2_context_net_pop_state(net);
                                wtk_fst_net2_link_state(tmp_net,prev,state,0,in_id,0,0.0,i*gain);
                                //wtk_debug("%d %d %d %d %f\n",prev->id,state->id,in_id,0,i*gain);
                            }else{
                                wtk_fst_net2_link_state(tmp_net,prev,tmp_net->start,0,in_id,out_id,0.0,i*gain);
                                //wtk_debug("%d %d %d %d %f\n",prev->id,tmp_net->start->id,in_id,out_id,i*gain);
                            }
                            prev = state;
                        }else{
                            if(i == a->nslot - 1){
                                wtk_fst_net2_link_state(tmp_net,prev,tmp_net->start,0,in_id,out_id,0.0,i*gain);
                                //wtk_debug("%d %d %d %d %f\n",prev->id,tmp_net->start->id,in_id,out_id,i*gain);
                            }
                            prev = from;
                            //wtk_debug("%d\n",from->id);
                        }
                    }else{
                        if(i < a->nslot - 1){
                            state = qtk_k2_context_net_pop_state(net);
                            wtk_fst_net2_link_state(tmp_net,prev,state,0,in_id,0,0.0,i*gain);
                            //wtk_debug("%d %d %d %d %f\n",prev->id,state->id,in_id,0,i*gain);
                        }else{
                            wtk_fst_net2_link_state(tmp_net,prev,tmp_net->start,0,in_id,out_id,0.0,i*gain);
                            //wtk_debug("%d %d %d %d %f\n",prev->id,tmp_net->start->id,in_id,out_id,i*gain);
                            state->type = WTK_FST_NORMAL_ARRAY_STATE;
                        }
                        prev = state;
                    }
                }
            }
            wtk_strbuf_reset(buf);
        }else{
            wtk_strbuf_push(buf,s,1);
        }
        s++;
    }
    wtk_strbuf_delete(buf);
    net->last_outid = out_id;
    //qtk_k2_context_net_dump(net);
    return 0;
}


qtk_k2_keywrd_q_t* qtk_k2_context_net_kq_new(qtk_k2_context_net_t* net){
    qtk_k2_keywrd_q_t* kq = (qtk_k2_keywrd_q_t*)wtk_heap_malloc(net->heap,sizeof(qtk_k2_keywrd_q_t));
    wtk_queue_push(&(net->keywrd_q),&(kq->q_n));
    kq->num_phn = 1;
    kq->wrd_cnt = 0;
    return kq;
}

int qtk_k2_context_net_build_keyword_plus(qtk_k2_context_net_t* net,char *data, int len){
    if(!net->dict.kv || len < 1){
        return -1;
    }
    if(net->net->rbin_states){
        wtk_free(net->net->rbin_states);
        net->net->rbin_states = NULL;
    }
    if(net->out_sym){
        wtk_free(net->out_sym);
    }
    int i,j,k,in_id,out_id;
    unsigned short* ids;
    wtk_fst_net2_t* tmp_net = net->tmp_net;
    wtk_dict_word_t *dw;
    wtk_dict_pron_t *pron;
    qtk_k2_keywrd_q_t *kq;
    wtk_strbuf_t *out_word = wtk_strbuf_new(512,1);
    float gain = net->cfg->hot_gain * 1.0;
    char *s = data,*e = data + len;
    int cnt, wrd_cnt = 0,*p,tmpnum;
    int a,b,c,d,tmp;//a*b*c = kq->num_phn

    out_id = net->last_outid + 1;
    kq = qtk_k2_context_net_kq_new(net);
    kq->out_id = out_id;
    while(s < e){
        cnt = wtk_utf8_bytes(*s);
        if(*s == '\n' || *s == EOF){

            kq->wrd_cnt = wrd_cnt;
            //wtk_debug("%p %d %d\n",kq,wrd_cnt,kq->num_phn);
            kq->phns = (int*)wtk_heap_malloc(net->heap,sizeof(int)*wrd_cnt*kq->num_phn);
            kq->olab.data = (char*)wtk_heap_malloc(net->heap,sizeof(char)*out_word->pos);
            memcpy(kq->olab.data,out_word->data,out_word->pos);
            kq->olab.len = out_word->pos;
            tmpnum = kq->num_phn;
            //wtk_debug("num_phn:%d %p\n",kq->num_phn,kq);
            for(j = 0; j < wrd_cnt; j++){
                if(kq->ids[j][0] == 1){
                    for(k = 0; k < kq->num_phn; k++){
                        *(kq->phns + wrd_cnt*k + j) = kq->ids[j][1];
                        //wtk_debug("%d\n",kq->ids[j][1]);
                        //wtk_debug("%d\n",kq->ids[1][0]);
                    }
                }else{
                    tmpnum /= kq->ids[j][0];
                    a = tmpnum;
                    b = kq->ids[j][0];
                    c = kq->num_phn/(a*b);
                    tmp = 0;
                    for(k = 0; k < c; k++){
                        for(i = 0; i < b; i++){
                            for(d = 0; d < a; d++){
                                *(kq->phns + wrd_cnt*tmp + j) = kq->ids[j][i+1];
                                tmp++;
                            }
                        }
                    }
                }
            }
            /*
            for(j=0;j<kq->num_phn;j++){
            for(k=0;k<kq->wrd_cnt;k++){
            wtk_debug("phn[%d][%d]=%d\n",j,k,*(kq->phns+k+j*wrd_cnt));
                }
            }*/
            out_id++;
            kq = qtk_k2_context_net_kq_new(net);
            kq->out_id = out_id;
            wrd_cnt = 0;
            wtk_strbuf_reset(out_word);
        }else{
            wtk_strbuf_push(out_word,s,cnt);
            dw = wtk_kvdict_get_word(net->dict.kv,s,cnt);
            if(!dw){
                qtk_k2_context_net_dump(net);
                wtk_strbuf_delete(out_word);
                return -1;
            }
            //wtk_debug("%.*s %d\n",dw->name->len,dw->name->data,dw->npron);
            if(wrd_cnt >= 40){
                qtk_k2_context_net_dump(net);
                wtk_strbuf_delete(out_word);
                return -1;
            }
            for(i = 0,pron = dw->pron_list;pron;pron = pron->next){
                ids = (unsigned short*)pron->pPhones;
                in_id = *ids;
                //wtk_debug("%d\n",in_id);
                if(i + 1 > 5){
                    qtk_k2_context_net_dump(net);
                    wtk_strbuf_delete(out_word);
                    return -1;
                }
                kq->ids[wrd_cnt][i+1] = in_id;
                //wtk_debug("%d %d %d\n",wrd_cnt,i+1,in_id);
                i++;
            }
            kq->ids[wrd_cnt][0] = i;
            wrd_cnt++;
            kq->num_phn *= i;
            //wtk_debug("num_phn:%d %p\n",kq->num_phn,kq);
        }
        s+=cnt;
    }

    wtk_queue_node_t *qn;
    wtk_fst_state2_t *state,*from,*prev;
    net->out_sym = (wtk_string_t**)wtk_malloc(sizeof(wtk_string_t*)*net->keywrd_q.length);
    for(k = 0,qn = net->keywrd_q.pop; qn; qn = qn->next,k++){
        kq = data_offset2(qn, qtk_k2_keywrd_q_t, q_n);
        if(kq->wrd_cnt == 0){
            break;
        }
        *(net->out_sym + k) = &(kq->olab);
        //wtk_debug("%.*s\n",kq->olab.len,kq->olab.data);
        p = kq->phns;
        out_id = kq->out_id;
        for(j = 0; j < kq->num_phn; j++){
            from = tmp_net->start;
            prev = tmp_net->start;
            for(i = 1; i <= kq->wrd_cnt; i++){
                //wtk_debug("%d %d\n",j,i);
                //wtk_debug("%d\n",*p);
                in_id = *p;
                if(from){
                    from = qtk_k2_context_net_find_and_merge_state_keyword(tmp_net,from,in_id);
                    //wtk_debug("====== %p\n",from);
                    if(!from){
                        if(i < kq->wrd_cnt){
                            state = qtk_k2_context_net_pop_state(net);
                            wtk_fst_net2_link_state(tmp_net,prev,state,0,in_id,0,0.0,i*gain);
                            //wtk_debug("%d %d %d %d %f\n",prev->id,state->id,in_id,0,i*gain);
                        }else{
                            wtk_fst_net2_link_state(tmp_net,prev,tmp_net->start,0,in_id,out_id,0.0,i*gain);
                            //wtk_debug("%d %d %d %d %f\n",prev->id,tmp_net->start->id,in_id,out_id,i*gain);
                        }
                        prev = state;
                    }else{
                        if(i == kq->wrd_cnt){
                            wtk_fst_net2_link_state(tmp_net,prev,tmp_net->start,0,in_id,out_id,0.0,i*gain);
                            //wtk_debug("%d %d %d %d %f\n",prev->id,tmp_net->start->id,in_id,out_id,i*gain);
                        }
                        prev = from;
                        //wtk_debug("%d\n",from->id);
                    }
                }else{
                    if(i < kq->wrd_cnt){
                        state = qtk_k2_context_net_pop_state(net);
                        wtk_fst_net2_link_state(tmp_net,prev,state,0,in_id,0,0.0,i*gain);
                        //wtk_debug("%d %d %d %d %f\n",prev->id,state->id,in_id,0,i*gain);
                    }else{
                        wtk_fst_net2_link_state(tmp_net,prev,tmp_net->start,0,in_id,out_id,0.0,i*gain);
                        //wtk_debug("%d %d %d %d %f\n",prev->id,tmp_net->start->id,in_id,out_id,i*gain);
                        state->type = WTK_FST_NORMAL_ARRAY_STATE;
                    }
                    prev = state;
                }
                p++;
            }
        }
    }
    qtk_k2_context_net_dump(net);
    wtk_strbuf_delete(out_word);
    return 0;
}

wtk_string_t* qtk_k2_context_net_get_outsym(qtk_k2_context_net_t* net, int out_id){
    int o = out_id - net->last_outid - 1;
    if(o >= 0){
        //wtk_debug("%d %p\n",o,net->out_sym + o);
        return *(net->out_sym + o);
    }
    return NULL;
}

void qtk_k2_context_xbnfnet_update(qtk_k2_context_net_t* net,wtk_fst_state2_t *state,wtk_strbuf_t *buf){
    wtk_fst_trans2_t *trans;
    wtk_dict_word_t *dw;
    wtk_strbuf_t *buf2 = NULL;
    for(trans=(wtk_fst_trans2_t*)state->v.trans;trans;trans=(wtk_fst_trans2_t*)trans->hook.next){
        dw = trans->hook2;
        // wtk_debug("%d %d %d %d\n",trans->from_state->id,trans->to_state->id,trans->in_id,trans->out_id);
        // if(dw){
        //     wtk_debug("%.*s\bn",dw->name->len,dw->name->data);
        // }
        if(trans->to_state->type != WTK_FST_FINAL_STATE){
            if(!trans->to_state->hook){
                buf2 = wtk_strbuf_new(1024,1);
                trans->to_state->hook = buf2;
            }else{
                buf2 = trans->to_state->hook;
                wtk_strbuf_reset(buf2);
            }

            if(buf){
                wtk_strbuf_push(buf2,buf->data,buf->pos);
            }
            if(dw){
                wtk_strbuf_push(buf2,dw->name->data,dw->name->len);
            }
            qtk_k2_context_xbnfnet_update(net,(wtk_fst_state2_t*)trans->to_state,buf2);
        }else{
            if(buf){
                wtk_strbuf_push(net->text,buf->data,buf->pos);
            }
            if(dw){
                wtk_strbuf_push(net->text,dw->name->data,dw->name->len);
            }
            wtk_strbuf_push_f(net->text,"\n");
            //wtk_debug("final state:%d %.*s\n",trans->to_state->id,net->text->pos,net->text->data);
        }
    }
}

void qtk_k2_context_net_xbnf_clean_hook(wtk_xbnfnet_t *xbnf)
{
	wtk_slist_node_t *sn;
	wtk_fst_state2_t *s;

	for(sn=xbnf->state_l.prev;sn;sn=sn->prev)
	{
		s=data_offset(sn,wtk_fst_state2_t,q_n);
        if(s->hook){
            wtk_strbuf_delete((wtk_strbuf_t*)s->hook);
        }
		s->hook=NULL;
	}
}

int qtk_k2_context_net_build_keyword_plus_xbnf(qtk_k2_context_net_t* net,char *data, int len){
	wtk_string_t v;
	int ret=0;
	wtk_string_set(&(v),data,len);
    wtk_xbnfnet_process(net->xbnf,&(v),net->tmp_net2);
    qtk_k2_context_xbnfnet_update(net,net->tmp_net2->start,NULL);
    qtk_k2_context_net_xbnf_clean_hook(net->xbnf);
    qtk_k2_context_net_build_keyword_plus(net,net->text->data,net->text->pos);
    if(net->net->rbin_states){
        wtk_free(net->net->rbin_states);
        net->net->rbin_states = NULL;
    }
    qtk_k2_context_net_dump(net);
    return ret;
}