#include "wtk_cosynthesis_phrase.h"
#include <ctype.h>
#include "wtk/core/wtk_strbuf.h"

void wtk_cosynthesis_phrase_fandb(wtk_cosynthesis_phrase_t *phrase, int len);
void wtk_cosynthesis_phrase_crf_process(wtk_cosynthesis_phrase_t *phrase, char **word_seq, int len);
char* wtk_cosynthesis_phrase_new_string(wtk_cosynthesis_phrase_t *phrase,char **word_seq,int nword);

wtk_cosynthesis_phrase_t* wtk_cosynthesis_phrase_new(wtk_cosynthesis_phrase_cfg_t* cfg)
{
    wtk_cosynthesis_phrase_t *phrase = NULL;
    phrase = wtk_malloc(sizeof(*phrase));
    memset(phrase,0,sizeof(wtk_cosynthesis_phrase_t));
    
    phrase->heap = wtk_heap_new(2048);
     phrase->pos = qtk_nltkpos_new(cfg->weight_path,phrase->heap);
    phrase->crfpp = wtk_crf_new(cfg->crf_path);

    return phrase;
}

int wtk_cosynthesis_phrase_delete(wtk_cosynthesis_phrase_t* phrase)
{
	qtk_nltkpos_delete(phrase->pos);
    wtk_heap_delete(phrase->heap);
    wtk_crf_delete(phrase->crfpp);
    wtk_free(phrase);
    return 0;
}

int wtk_cosynthesis_phrase_process(wtk_cosynthesis_phrase_t* phrase, char **seq_word, int len)
{
	wtk_heap_reset(phrase->heap);
	qtk_nltkpos_process(phrase->pos, seq_word, len);
    wtk_cosynthesis_phrase_fandb(phrase,len);
    wtk_cosynthesis_phrase_crf_process(phrase,seq_word,len);
    phrase->output = wtk_cosynthesis_phrase_new_string(phrase,seq_word,len);
    wtk_fkv_reset(phrase->pos->weight_kv);
    return 0;
}

//前向和反向进度
void wtk_cosynthesis_phrase_fandb(wtk_cosynthesis_phrase_t *phrase, int len)
{
    wtk_heap_t *heap = phrase->heap;
    phrase->forward = wtk_heap_malloc(heap,sizeof(int)*len);
    phrase->backward = wtk_heap_malloc(heap,sizeof(int)*len);
    int i = 0;
    for(i = 0; i < len; ++i){
        phrase->forward[i] = i;
        phrase->backward[i] = len - i - 1;
    }
    return;
}

///////// CRF 计算BMSE
void wtk_cosynthesis_phrase_crf_process(wtk_cosynthesis_phrase_t *phrase, char **word_seq, int len)
{
    wtk_heap_t *heap = phrase->heap;
    char *data = NULL;
    int i = 0;
    char **pos = phrase->pos->pos;
    const char **BMES = wtk_heap_malloc(heap,sizeof(char*)*len);
    for(i = 0; i < len; ++i){
        data = wtk_heap_malloc(heap,256);
        sprintf(data,"%s\t%s\t%d\t%d",word_seq[i],pos[i],phrase->forward[i],phrase->backward[i]);
        wtk_crf_add(phrase->crfpp,data);
    }
    wtk_crf_process(phrase->crfpp);
    // wtk_crf_get_result(phrase->crfpp);
    // wtk_debug("%s\n",result);
    int n = wtk_crf_nresult(phrase->crfpp);
    for(i = 0; i < n; ++i){
        BMES[i] = wtk_crf_get(phrase->crfpp,i);
    }
    for(; i < len; ++i){
        BMES[i] = "S";
    }
    wtk_crf_reset(phrase->crfpp);
    phrase->BMES = BMES;
    return;
}

//生成新的句子
char* wtk_cosynthesis_phrase_new_string(wtk_cosynthesis_phrase_t *phrase,char **word_seq,int nword)
{
    wtk_strbuf_t *buf = wtk_strbuf_new(1024,1.0f);
    int i = 0;
    const char **BMES = phrase->BMES;
    int len = 0;
    char c = 0;
    char *ret = NULL;
    for(i = 0; i < nword; ++i){
        len = strlen(word_seq[i]);
        wtk_strbuf_push(buf,word_seq[i],len);
        if(!strcmp(BMES[i],"E") || !strcmp(BMES[i],"S")){
            c = word_seq[i][len-1];
            if(isalpha(c)){
               if(i == nword - 1){
                    wtk_strbuf_push_c(buf,'.');
               }else{
                    wtk_strbuf_push_c(buf,',');
               }
            }
        }
        wtk_strbuf_push_c(buf,' ');
    }
    ret = wtk_heap_dup_str2(phrase->heap,buf->data,buf->pos);
    //wtk_debug("%s\n",ret);
    wtk_strbuf_delete(buf);
    return ret;
}


void wtk_cosynthesis_phrase_to_path(void)
{
    // int len = sizeof(WEIGHTS)/sizeof(wtk_cosynthesis_phrase_weight_t);
    // int i = 0;
    // wtk_strbuf_t *buf1 = wtk_strbuf_new(256,1.0f);
    // wtk_strbuf_t *buf2 = wtk_strbuf_new(256,1.0f);
    // char num[10] = {0,};
    // int n = 0;
    // int k_len = 0;
    // for(i = 0; i < len; ++i){
    //     k_len = strlen(WEIGHTS[i].k);
    //     wtk_strbuf_reset(buf1);
    //     n = 0;
    //     while(n < k_len){
    //         if(WEIGHTS[i].k[n] == ' '){
    //             wtk_strbuf_push(buf1,"_",strlen("_"));
    //         }else{
    //             wtk_strbuf_push(buf1,WEIGHTS[i].k+n,1);
    //         }
    //         n++;
    //     }
    //     for(n = 0; n < WEIGHTS[i].w_len; ++n){
    //         sprintf(num,"%d",n);
    //         wtk_strbuf_reset(buf2);
    //         wtk_strbuf_push(buf2,buf1->data,buf1->pos);
    //         wtk_strbuf_push(buf2,"_",strlen("_"));
    //         wtk_strbuf_push(buf2,num,strlen(num));
    //         wtk_strbuf_push_c(buf2,' ');
    //         wtk_strbuf_push(buf2,WEIGHTS[i].weight[n].k,strlen(WEIGHTS[i].weight[n].k));
    //         wtk_strbuf_push_c(buf2,' ');
    //         sprintf(num,"%f",WEIGHTS[i].weight[n].v);
    //         wtk_strbuf_push(buf2,num,strlen(num));
    //         printf("%.*s\n",buf2->pos,buf2->data);
    //     }
    // }
    return;
}
