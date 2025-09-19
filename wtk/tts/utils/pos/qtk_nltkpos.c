/*
 * qtk_nltkpos.c
 *
 *  Created on: Mar 4, 2022
 *      Author: dm
 */
#include "qtk_nltkpos.h"
#include <ctype.h>

static char *NLTKPOS_START[] = {"-START-","-START2-"};
static char *NLTKPOS_END[] = {"-END-","-END2-"};
//static char *CLASSES[] = {"#","$","''","(",")",",",".",":","CC","CD","DT","EX","FW","IN","JJ","JJR",
//                                "JJS","LS","MD","NN","NNP","NNPS","NNS","PDT","POS","PRP","PRP$","RB","RBR",
//                                "RBS","RP","SYM","TO","UH","VB","VBD","VBG","VBN","VBP","VBZ","WDT","WP",
//                                "WP$","WRB","``"};
static char* nltkpos_tags[] = {
		"#",
		"CC", "CD", "DT", "EX", "FW", "IN","JJ","JJR", "JJS","LS",
		"MD","NN","NNP","NNPS","NNS", "PDT","POS","PRP","PRP$","RB",
		"RBR", "RBS","RP","SYM","TO","UH","VB","VBD","VBG","VBN",
		"VBP","VBZ","WDT","WP","WP$","WRB"
};

static char* pos_tags[] = {"0", "in", "to", "det", "md", "cc", "wp", "pps", "aux", "punc", "content"};
static qtk_nltkpos_tagdict_t TAGDICT[] = {
    #include "wtk/tts/cosynthesis/wtk_cosynthesis_phrase_tagdict.cd"
};

// static qtk_nltkpos_weight_t WEIGHTS[] = {
//     #include "wtk_cosynthesis_phrase_model_weight.cd"
// };

qtk_nltkpos_t* qtk_nltkpos_new(char *path,wtk_heap_t *heap)
{
	qtk_nltkpos_t *pos;
    int i = 0;

    pos = wtk_malloc(sizeof(qtk_nltkpos_t));
    // int weight_len = sizeof(WEIGHTS)/sizeof(wtk_cosynthesis_phrase_weight_t);
    // pos->weights = wtk_str_hash_new(weight_len*2);
    // for(i = 0; i < weight_len; ++i){
    //     wtk_str_hash_add(pos->weights,WEIGHTS[i].k,strlen(WEIGHTS[i].k),WEIGHTS+i);
    // }
    pos->weight_kv = wtk_fkv_new(path,17003);
    int dict_len = sizeof(TAGDICT)/sizeof(qtk_nltkpos_tagdict_t);
    pos->tagdict = wtk_str_hash_new(dict_len*3);
    for(i = 0; i < dict_len; ++i){
        wtk_str_hash_add(pos->tagdict,TAGDICT[i].k,strlen(TAGDICT[i].k),TAGDICT+i);
    }
    pos->heap = heap;

    return pos;
}

int qtk_nltkpos_delete(qtk_nltkpos_t *pos)
{
    wtk_str_hash_reset(pos->tagdict);
    wtk_str_hash_delete(pos->tagdict);
    if(pos->weight_kv) wtk_fkv_delete(pos->weight_kv);
    // wtk_str_hash_reset(pos->weights);
    // wtk_str_hash_delete(pos->weights);
    wtk_free(pos);
    return 0;
}

int qtk_nltkpos_word_case1(char *word)
{
    int i = 0;
    int len = strlen(word);
    if(word[0] == '-')
        return 0;
    for(i = 1; i < len; ++i){
        if(word[i] == '-')
            return 1;
    }
    return 0;
}

int qtk_nltkpos_post_word_case2(char *word)
{
    int i = 0;
    if(strlen(word) != 4)
        return 0;
    for(i = 0; i < 4; ++i){
        if(!isdigit(word[i])) return 0;
    }
    return 1;
}
static void qtk_nltkpos_tolower(char *src,int len,char *dist)
{
    int i = 0;
    for(i = 0; i < len; ++i){
        dist[i] = tolower(src[i]);
    }
    return;
}

char* qtk_nltkpos_word_norm(wtk_heap_t *heap,char *word)
{
    char *ret = NULL;

    if(qtk_nltkpos_word_case1(word)){
        ret = "!HYPHEN";
    }else if(qtk_nltkpos_post_word_case2(word)){
        ret = "!YEAR";
    }else if(isdigit(word[0])){
        ret = "!DIGITS";
    }else{
        ret = wtk_heap_zalloc(heap,strlen(word)+1);
        qtk_nltkpos_tolower(word,strlen(word),ret);
    }

    return ret;
}

int qtk_nltkpos_norm(qtk_nltkpos_t *pos,char **seq_word,int word_len)
{
    wtk_heap_t *heap = pos->heap;
    int i = 0,j = 0;
    pos->context_len = word_len+4;
    pos->context = wtk_heap_malloc(heap,sizeof(char*)*(word_len+4));
    pos->context[0] = NLTKPOS_START[0];
    pos->context[1] = NLTKPOS_START[1];
    for(i = 0,j = 2; i < word_len; ++i,++j){
        pos->context[j] = qtk_nltkpos_word_norm(heap,seq_word[i]);
    }
    pos->context[j] = NLTKPOS_END[0];
    pos->context[j+1] = NLTKPOS_END[1];
    return 0;
}


void qtk_nltkpos_model_features_add(qtk_nltkpos_t *pos, char *feature, int feature_len)
{
    int i = 0;
    int n = pos->features_len;
    wtk_heap_t *heap = pos->heap;
    qtk_nltkpos_features_t *features = pos->features;

    for(i = 0; i < n; ++i){
        if(!wtk_string_cmp(features[i].k,feature,feature_len)){
            features[i].v += 1;
            return;
        }
    }
    features[i].k = wtk_heap_dup_string(heap,feature,feature_len);
    features[i].v = 1;
    pos->features_len += 1;
    // printf("%d\n",pos->features_len);
    return;
}

void qtk_nltkpos_predict_scores(wtk_heap_t *heap,qtk_nltkpos_w_t **scoress,char *lable, int value, float weight)
{
    int i = 0;
    for(i = 0; i < 38 && scoress[i]; ++i){
        if((strlen(scoress[i]->k) == strlen(lable)) && !strcmp(scoress[i]->k,lable)){
            scoress[i]->v += value *weight;
            return;
        }
    }
    scoress[i] = wtk_heap_malloc(heap,sizeof(qtk_nltkpos_w_t));
    scoress[i]->k = wtk_heap_dup_str(heap,lable);
    scoress[i]->v = value * weight;
    return;
}

char *qtk_nltkpos_predict_max(qtk_nltkpos_w_t **scores)
{
    int i = 0;
    float v = scores[0]->v;
    char *pos = scores[0]->k;
    for(i = 1; i < 38 && scores[i]; ++i){
        if(v < scores[i]->v){
            pos = scores[i]->k;
            v = scores[i]->v;
        }
    }
    return pos;
}

char *qtk_nltkpos_predict(qtk_nltkpos_t *pos)
{
    wtk_heap_t *heap = pos->heap;
    qtk_nltkpos_features_t *features = pos->features;
    int n = pos->features_len, i = 0,j = 0;
    wtk_string_t *k = NULL;
    qtk_nltkpos_w_t **scores = wtk_heap_zalloc(heap,sizeof(qtk_nltkpos_w_t *)*38);
    int v = 0;
    wtk_strbuf_t *buf1 = wtk_strbuf_new(256,1.0f);
    wtk_strbuf_t *buf2 = wtk_strbuf_new(256,1.0f);
    wtk_string_t *w_s = NULL;
    char num_s[3] = {0};
    char pos_s[5] = {0};
    float wf = 0.0f;
    // for(i = 0; i < n; ++i){
    //     k = features[i].k;
    //     v = features[i].v;
    //     w = wtk_str_hash_find(pos->weights,k->data,k->len);
    //     if(w == NULL) continue;
    //     ws = w->weight;
    //     for(j = 0; j < w->w_len; ++j){
    //         wtk_cosynthesis_phrase_pos_predict_scores(heap,scores,ws[j].k,v,ws[j].v);
    //     }
    // }
    for(i = 0; i < n; ++i){
        wtk_strbuf_reset(buf1);
        k = features[i].k;
        v = features[i].v;
        //先处理k
        for(j = 0; j < k->len; ++j){
            if(k->data[j] == ' '){
                wtk_strbuf_push_c(buf1,'_');
            }else{
                wtk_strbuf_push_c(buf1,k->data[j]);
            }
        }
        for(j = 0; j < 38; ++j){
            wtk_strbuf_reset(buf2);
            wtk_strbuf_push(buf2,buf1->data,buf1->pos);
            wtk_strbuf_push_c(buf2,'_');
            sprintf(num_s,"%d",j);
            wtk_strbuf_push(buf2,num_s,strlen(num_s));
            w_s = wtk_fkv_get_str(pos->weight_kv,buf2->data,buf2->pos);
            if(w_s == NULL){
                break;
            }
            wtk_strbuf_reset(buf2);
            wtk_strbuf_push(buf2,w_s->data,w_s->len);
            wtk_strbuf_push_c(buf2,'\0');
            sscanf(buf2->data,"%s %f",pos_s,&wf);
            //printf("%.*s %s %f\n",k->len,k->data,pos_s,wf);
            qtk_nltkpos_predict_scores(heap,scores,pos_s,v,wf);
        }
    }
    // for(i = 0; i < 38 && scores[i]; ++i){
    //     printf("%s %f\n",scores[i]->k,scores[i]->v);
    // }
    wtk_strbuf_delete(buf1);
    wtk_strbuf_delete(buf2);
    return qtk_nltkpos_predict_max(scores);
}

char* qtk_nltkpos_model_process(qtk_nltkpos_t *pos, int i, char *word, char *prev, char *prev2)
{
    wtk_heap_t *heap = pos->heap;
    //int context_len = pos->context_len;
    char **context = pos->context;
    char *ret = NULL;
    int word_len = strlen(word);
    static char *fheads[] = {
        "bias","i suffix ","i pref1 ","i-1 tag ","i-2 tag ",
        "i tag+i-2 tag ","i word ","i-1 tag+i word ","i-1 word ",
        "i-1 suffix ","i-2 word ","i+1 word ","i+1 suffix ","i+2 word ",
    };
    int cl = 0;
    i += sizeof(NLTKPOS_START)/sizeof(char*);

    wtk_strbuf_t *buf = wtk_strbuf_new(256,1.0f);
    pos->features = wtk_heap_malloc(heap,sizeof(qtk_nltkpos_features_t)*14);
    pos->features_len = 0;
    memset(pos->features,0,sizeof(qtk_nltkpos_features_t)*14);

    wtk_strbuf_push(buf,fheads[0],strlen(fheads[0]));
    qtk_nltkpos_model_features_add(pos,buf->data,buf->pos);
    wtk_strbuf_reset(buf);

    wtk_strbuf_push(buf,fheads[1],strlen(fheads[1]));
    wtk_strbuf_push(buf,word+word_len-3,3);
    qtk_nltkpos_model_features_add(pos,buf->data,buf->pos);
    wtk_strbuf_reset(buf);

    wtk_strbuf_push(buf,fheads[2],strlen(fheads[2]));
    wtk_strbuf_push_c(buf,word[0]);
    qtk_nltkpos_model_features_add(pos,buf->data,buf->pos);
    wtk_strbuf_reset(buf);

    wtk_strbuf_push(buf,fheads[3],strlen(fheads[3]));
    wtk_strbuf_push(buf,prev,strlen(prev));
    qtk_nltkpos_model_features_add(pos,buf->data,buf->pos);
    wtk_strbuf_reset(buf);

    wtk_strbuf_push(buf,fheads[4],strlen(fheads[4]));
    wtk_strbuf_push(buf,prev2,strlen(prev2));
    qtk_nltkpos_model_features_add(pos,buf->data,buf->pos);
    wtk_strbuf_reset(buf);

    wtk_strbuf_push(buf,fheads[5],strlen(fheads[5]));
    wtk_strbuf_push(buf,prev,strlen(prev));
    wtk_strbuf_push_c(buf,' ');
    wtk_strbuf_push(buf,prev2,strlen(prev2));
    qtk_nltkpos_model_features_add(pos,buf->data,buf->pos);
    wtk_strbuf_reset(buf);

    wtk_strbuf_push(buf,fheads[6],strlen(fheads[6]));
    wtk_strbuf_push(buf,context[i],strlen(context[i]));
    qtk_nltkpos_model_features_add(pos,buf->data,buf->pos);
    wtk_strbuf_reset(buf);

    wtk_strbuf_push(buf,fheads[7],strlen(fheads[7]));
    wtk_strbuf_push(buf,prev,strlen(prev));
    wtk_strbuf_push_c(buf,' ');
    wtk_strbuf_push(buf,context[i],strlen(context[i]));
    qtk_nltkpos_model_features_add(pos,buf->data,buf->pos);
    wtk_strbuf_reset(buf);

    wtk_strbuf_push(buf,fheads[8],strlen(fheads[8]));
    wtk_strbuf_push(buf,context[i-1],strlen(context[i-1]));
    qtk_nltkpos_model_features_add(pos,buf->data,buf->pos);
    wtk_strbuf_reset(buf);

    cl = strlen(context[i-1]);
    wtk_strbuf_push(buf,fheads[9],strlen(fheads[9]));
    wtk_strbuf_push(buf,context[i-1]+cl-3,3);
    qtk_nltkpos_model_features_add(pos,buf->data,buf->pos);
    wtk_strbuf_reset(buf);

    wtk_strbuf_push(buf,fheads[10],strlen(fheads[10]));
    wtk_strbuf_push(buf,context[i-2],strlen(context[i-2]));
    qtk_nltkpos_model_features_add(pos,buf->data,buf->pos);
    wtk_strbuf_reset(buf);

    wtk_strbuf_push(buf,fheads[11],strlen(fheads[11]));
    wtk_strbuf_push(buf,context[i+1],strlen(context[i+1]));
    qtk_nltkpos_model_features_add(pos,buf->data,buf->pos);
    wtk_strbuf_reset(buf);

    cl = strlen(context[i+1]);
    wtk_strbuf_push(buf,fheads[12],strlen(fheads[12]));
    wtk_strbuf_push(buf,context[i+1]+cl-3,3);
    qtk_nltkpos_model_features_add(pos,buf->data,buf->pos);
    wtk_strbuf_reset(buf);

    wtk_strbuf_push(buf,fheads[13],strlen(fheads[13]));
    wtk_strbuf_push(buf,context[i+2],strlen(context[i+2]));
    qtk_nltkpos_model_features_add(pos,buf->data,buf->pos);
    wtk_strbuf_reset(buf);

    // wtk_cosynthesis_phrase_features_print(pos);
    //process model
    ret = qtk_nltkpos_predict(pos);
    wtk_strbuf_delete(buf);
    return ret;
}

int qtk_nltkpos_features_print(qtk_nltkpos_t *pos)
{
    int i = 0;
    //wtk_consynthesis_pos_features_t *features = pos->features;
    int n = pos->features_len;
    for(i=0; i < n; ++i){
        printf("features [%.*s] %d\n",pos->features[i].k->len,pos->features[i].k->data,pos->features[i].v);
    }
    return 0;
}

int qtk_nltkpos_process(qtk_nltkpos_t *pos,char **seq_word,int word_len)
{
    int i = 0;
    qtk_nltkpos_tagdict_t *dg = NULL;
    char *word = NULL;
    char *prev = NLTKPOS_START[0];
    char *prev2 = NLTKPOS_START[1];
    char *tag = NULL;

    pos->pos = wtk_heap_malloc(pos->heap,word_len*sizeof(char*));
    //do with correct format.
    qtk_nltkpos_norm(pos,seq_word,word_len);
    for(i = 0; i < word_len; ++i){
        word = seq_word[i];
        dg = wtk_str_hash_find(pos->tagdict,word,strlen(word));
        if(!dg){
            tag = qtk_nltkpos_model_process(pos,i,word,prev,prev2);
        }else{
            tag = dg->v;
        }
        pos->pos[i] = tag;
        prev2 = prev;
        prev = tag;
        //printf("wrd[%d]=%s\n",i,tag);
    }
    return 0;
}

int qtk_nltkpos_getidx2(char *cls, int len)
{
	wtk_string_t s;
	s.data=cls;
	s.len=len;
	return qtk_nltkpos_getidx(&s);
}

int qtk_nltkpos_getidx(wtk_string_t *cls)
{
	int i, size;

	size = sizeof(nltkpos_tags)/sizeof(nltkpos_tags[0]);

	for (i=0; i < size; i++)
	{
		if(0==wtk_string_cmp(cls, nltkpos_tags[i], strlen(nltkpos_tags[i])))
			break;
	}
	if (i < size)
		return i;
	else
		return -1;
}

int qtk_pos_getidx(wtk_string_t *cls)
{
	int i, size;

	size = sizeof(pos_tags)/sizeof(pos_tags[0]);

	for (i=0; i < size; i++)
	{
		if(0==wtk_string_cmp(cls, pos_tags[i], strlen(pos_tags[i])))
			break;
	}
	if (i < size)
		return i;
	else
		return -1;
}

