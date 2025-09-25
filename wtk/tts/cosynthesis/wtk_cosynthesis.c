#include "wtk_cosynthesis.h"
#include "wtk_cosynthesis_likehood.h"
#include "wtk/core/wtk_sort.h"
#include "wtk/core/wtk_str.h"

#ifdef WIN32
#define __FLT_MAX__ FLT_MAX 
#endif

void wtk_cosynthesis_token_clean(wtk_cosynthesis_t *cs);
void wtk_cosynthesis_audio_notify(wtk_cosynthesis_t *cs,short *data, int len);
void wtk_mtrietree_delete(wtk_mtrietree_t* mtrie);

const char* lphone_list[]={
       "ih","iy","ey","ay","oy",
       "eh","er",
       "aa","ae","ah",
       "ao","ow",
       "aw","uh","uw",
       "b","d","g",
       "p","t","k",
       "dh","th",
       "f","v",
       "s","z",
       "sh","zh",
       "hh","r","l",
       "w","y",
       "ng","n","m",
       "ch","jh",
       "x", "sil","pau",
};
const char* ltype_list[]={
       "L_IVowel","L_IVowel","L_IVowel","L_IVowel","L_IVowel",
       "L_EVowel","L_EVowel",
       "L_AVowel","L_AVowel","L_AVowel",
       "L_OVowel","L_OVowel",
       "L_UVowel","L_UVowel","L_UVowel",
       "voiced_plosive","voiced_plosive","voiced_plosive",
       "unvoiced_plosive","unvoiced_plosive","unvoiced_plosive",
       "lingua_fricative","lingua_fricative",
       "labiodental","labiodental",
       "alveolar","alveolar",
       "back_alveolar","back_alveolar",
       "others","others","others",
       "glide","glide",
       "nasal","nasal","nasal",
       "affricate","affricate",
       "silence","silence","silence"
}; 
const char* rphone_list[]={
       "ih","iy",
       "ey","eh","er",
       "aa","ae","aw","ah","ay",
       "ao","ow","oy",
       "uh","uw",
       "b","d","g",
       "p","t","k",
       "dh","th",
       "f","v",
       "s","z",
       "sh","zh",
       "hh","r","l",
       "w","y",
       "ng","n","m",
       "ch","jh",
       "x","pau", "sil"
};
const char* rtype_list[]={
       "R_IVowel","R_IVowel",
       "R_EVowel","R_EVowel","R_EVowel",
       "R_AVowel","R_AVowel","R_AVowel","R_AVowel","R_AVowel",
       "R_OVowel","R_OVowel","R_OVowel",
       "R_UVowel","R_UVowel",
       "voiced_plosive","voiced_plosive","voiced_plosive",
       "unvoiced_plosive","unvoiced_plosive","unvoiced_plosive",
       "lingua_fricative","lingua_fricative",
       "labiodental","labiodental",
       "alveolar","alveolar",
       "back_alveolar","back_alveolar",
       "others","others","others",
       "glide","glide",
       "nasal","nasal","nasal",
       "affricate","affricate",
       "silence","silence","silence"
};

wtk_mtrietree_t* wtk_mtrietree_new(wtk_trietree_cfg_t *cfg, wtk_heap_t *heap)
{
	wtk_mtrietree_t* mtrie;
	int ret=0;

	mtrie = (wtk_mtrietree_t*)wtk_malloc(sizeof(*mtrie));
	mtrie->cfg=cfg;
	mtrie->heap=heap;
    if (0==cfg->load_all)
    {
    	if (cfg->rbin)
    	{
    		mtrie->trie_fp = fopen(cfg->rbin->fn,"rb");
    		mtrie->trie_inset_fp = fopen(cfg->rbin->fn,"rb");
    	}else
    	{
            if(!(mtrie->trie_fp = fopen(cfg->trie_fn,"rb")))
            {
                printf("Open %s failed\n",cfg->trie_fn);
                ret=-1;goto end;
            }
            if(!(mtrie->trie_inset_fp = fopen(cfg->trie_inset_fn,"rb")))
            {
                printf("Open %s failed\n",cfg->trie_inset_fn);
                ret=-1;goto end;
            }
    	}
    }else
    {
    	mtrie->trie_fp=NULL;
    	mtrie->trie_inset_fp=NULL;
    }

end:
	if (ret!=0)
		wtk_mtrietree_delete(mtrie);
    return mtrie;
}

int wtk_mtrietree_reset(wtk_mtrietree_t* mtrie)
{
    if (0==mtrie->cfg->load_all)
    {
        mtrie->root = (wtk_trieroot_t**)wtk_heap_zalloc(mtrie->heap, mtrie->cfg->nwrds * sizeof(wtk_trieroot_t*));
        mtrie->root_inset = (wtk_trieroot_t**)wtk_heap_zalloc(mtrie->heap, mtrie->cfg->nwrds * sizeof(wtk_trieroot_t*));
    }
    else
    {
    	mtrie->root = NULL;
    	mtrie->root_inset = NULL;
    }

    return 0;
}

void wtk_mtrietree_delete(wtk_mtrietree_t* mtrie)
{
    if (mtrie->trie_fp)
    	fclose(mtrie->trie_fp);
    if (mtrie->trie_inset_fp)
    	fclose(mtrie->trie_inset_fp);

    wtk_free(mtrie);
}

wtk_cosynthesis_t *wtk_cosynthesis_new(wtk_cosynthesis_cfg_t *cfg)
{
	int ret=0;

    wtk_cosynthesis_t* cs;

    cs =wtk_malloc(sizeof(wtk_cosynthesis_t));
    cs->cfg = cfg;
    cs->heap = wtk_heap_new(1024);
    cs->dec_heap = wtk_heap_new(1024);
    cs->lexicon = wtk_cosynthesis_lexicon_new(&cfg->lex_cfg);
    cs->mtrie = wtk_mtrietree_new(&(cfg->trie_cfg), cs->heap);
    if(ret!=0)goto end;
    cs->cur_token_q = wtk_queue_new();
    cs->pre_token_q = wtk_queue_new();
    cs->tok_pool = wtk_vpool2_new(sizeof(wtk_cosynthesis_token_t),40);
    cs->front_end = init_engine();
    set_user_lexicon(cs->front_end,cfg->user_lexicon);
    cs->wsola = wtk_wsola_new(&cfg->wsola_cfg);
    cs->back_end = wtk_cosynthesis_backend_new(&cfg->be_cfg);
#ifdef USE_PHRASE
    cs->phrase = wtk_cosynthesis_phrase_new(&(cfg->phr_cfg));
#endif

    wtk_wsola_set_notify(cs->wsola,cs,(wtk_wsola_notity_f)wtk_cosynthesis_audio_notify);
    cs->raw_txt = wtk_strbuf_new(128,1);
    cs->proc_txt = wtk_strbuf_new(128,1);
    cs->buf = wtk_strbuf_new(128,1);
    cs->output = wtk_cosynthesis_output_new();
    cs->lexicon->output = cs->output;
    cs->lab = NULL;

    wtk_cosynthesis_reset(cs);
end:
	if (ret!=0)
	{
		wtk_cosynthesis_delete(cs);
		cs=NULL;
	}
    return cs;
}

void wtk_cosynthesis_delete(wtk_cosynthesis_t *cs)
{
    // wtk_cosynthesis_token_clean(cs);
	wtk_cosynthesis_cleaninfo(cs);        //Note: depend cs->lexicon.
    wtk_cosynthesis_lexicon_delete(cs->lexicon);
    wtk_heap_delete(cs->heap);
    wtk_heap_delete(cs->dec_heap);
    wtk_queue_delete(cs->cur_token_q);
    wtk_queue_delete(cs->pre_token_q);
    wtk_vpool2_delete(cs->tok_pool);
    free_user_lexicon(cs->front_end);
    release_engine(cs->front_end);
    wtk_wsola_delete(cs->wsola);
    wtk_cosynthesis_backend_delete(cs->back_end);
    wtk_strbuf_delete(cs->raw_txt);
    wtk_strbuf_delete(cs->proc_txt);
    wtk_strbuf_delete(cs->buf);
    if(cs->output)
    {
    	wtk_cosynthesis_output_delete(cs->output);
    }
#ifdef USE_PHRASE
    wtk_cosynthesis_phrase_delete(cs->phrase);
#endif
    wtk_mtrietree_delete(cs->mtrie);

    wtk_free(cs);
}

void wtk_cosynthesis_reset(wtk_cosynthesis_t *cs)
{
    if(cs->best_path)
        cs->best_path =NULL;
    cs->best_cost = __FLT_MAX__;
    wtk_cosynthesis_token_clean(cs);
    wtk_cosynthesis_cleaninfo(cs);
    wtk_cosynthesis_lexicon_reset(cs->lexicon);
    wtk_heap_reset(cs->heap);
    wtk_mtrietree_reset(cs->mtrie);
    wtk_cosynthesis_unit_decode_reset(cs);
    wtk_wsola_reset(cs->wsola);
    wtk_strbuf_reset(cs->buf);
    wtk_cosynthesis_backend_reset(cs->back_end);
    if (cs->output->isend)
    {
    	wtk_strbuf_reset(cs->raw_txt);
    	wtk_strbuf_reset(cs->proc_txt);
    }
    wtk_cosynthesis_output_reset(cs->output);
    if (cs->lab){
        free_htslab(cs->lab);
        cs->lab=NULL;
    }
}

void wtk_cosynthesis_unit_decode_reset(wtk_cosynthesis_t *cs)
{
    wtk_queue_init(cs->cur_token_q);
    wtk_queue_init(cs->pre_token_q);
	wtk_heap_reset(cs->dec_heap);
	wtk_vpool2_reset(cs->tok_pool);
}

int wtk_cosynthesis_process_wsola(wtk_cosynthesis_t *cs, int inset)
{
    short *best_path = (short*)cs->best_path;
    int nwrd = cs->lexicon->ninput;
    int wrd_idx;//,unit_idx;
    short *audio;
    int audio_len;
    short *sil;
    int sil_len;
    wtk_cosynthesis_lexicon_cfg_t *lex_cfg = cs->lexicon->cfg;
    wtk_word_t *lex_wrds=cs->lexicon->wrds;
    int i;
    int rate = 4;
    int ret=0;

    if(best_path)
    {
        for(i = 0;i<nwrd-1;++i)
        {
            wrd_idx = cs->lexicon->input[i].word_id;
            audio_len = lex_wrds[wrd_idx].unit[best_path[i]].data_len;
            audio = (short*)lex_wrds[wrd_idx].unit[best_path[i]].data;
            //wtk_debug("best_path[%d]=%d raw audio_len=%d\n",i, best_path[i], audio_len);
            if (NULL==audio && lex_cfg->use_fp){
                audio = wtk_cosynthesis_lexicon_getaudio(cs->lexicon, &(lex_wrds[wrd_idx].unit[best_path[i]]), &audio_len);
            }
            if (NULL==audio)
            {
            	wtk_debug("unit data is null\n");
            	ret=-1;goto end;
            }
            sil_len=(int)(lex_wrds[wrd_idx].unit[best_path[i]].sil_prev_l * cs->lexicon->cfg->samples);
            //wtk_debug("wrd_idx=%d unit_id=%d sil_l=%f sil_len=%d audio_len=%d raw_audio_len=%d\n", wrd_idx, best_path[i], lex_wrds[wrd_idx].unit[best_path[i]].sil_prev_l, sil_len, audio_len, lex_wrds[wrd_idx].unit[best_path[i]].raw_audio_len);
            if (inset && sil_len > 0)
            {
                sil = (short*)wtk_calloc(sil_len, sizeof(short));
                wtk_wsola_feed(cs->wsola,sil,sil_len,0);
                wtk_free(sil);
            }
            wtk_wsola_feed(cs->wsola,audio,audio_len,0);
            if(!inset && cs->lexicon->input[i].finfo->sil_flabel!=NULL)
            {
            	//wtk_debug("sil_tot_dur=%d\n", cs->lexicon->input[i].sil_tot_dur);
                sil_len = cs->lexicon->input[i].sil_tot_dur * 5 * 16 * 2 /rate;
                sil = (short*)wtk_calloc(sil_len,sizeof(short));
                wtk_wsola_feed(cs->wsola,sil,sil_len,0);
                wtk_free(sil);
            }
        }
        //wtk_debug("best_path[%d]=%d\n", nwrd-1,best_path[nwrd-1]);
        wrd_idx = cs->lexicon->input[nwrd-1].word_id;
        audio_len = lex_wrds[wrd_idx].unit[best_path[nwrd-1]].data_len;
        audio = (short*)lex_wrds[wrd_idx].unit[best_path[nwrd-1]].data;
        if (NULL==audio && lex_cfg->use_fp){
            audio = wtk_cosynthesis_lexicon_getaudio(cs->lexicon, &(lex_wrds[wrd_idx].unit[best_path[nwrd-1]]), &audio_len);
        }
        if (NULL==audio)
        {
        	wtk_debug("unit data is null\n");
        	ret=-1;goto end;
        }
        wtk_wsola_feed(cs->wsola,audio,audio_len,1);
    }

 end:
	return ret;
}

/**
 * find unit for every word.
 * tri_type:
 *     0: default
 *     1: node contain it's all child info list.
 * in_set: for different feats.
 * return 0 success, -1 failed
 */
int wtk_cosynthesis_process_trietree(wtk_cosynthesis_t *cs, wtk_trieroot_t **root, int in_set)
{
    int ret=0;
    int i,deep;
    wtk_strbuf_t *input_feat;
    int input_len[4], size;
    uint16_t nunit=0;
    uint16_t* units=NULL;
    uint16_t* units2=NULL;
    uint16_t nunit2=0;
    wtk_cosynthesis_lexicon_input_t *input = cs->lexicon->input;
    char c;

    input_feat=wtk_strbuf_new(128,1);
    for(i=0;i<cs->lexicon->ninput;++i)
    {
    	//wtk_debug("i=%d\n", i);
        units=NULL;
        units2=NULL;
    	wtk_strbuf_reset(input_feat);
    	size=0;
    	if (in_set)
    	{
            wtk_strbuf_push(input_feat,input[i].finfo->left_phone_nosil->data,input[i].finfo->left_phone_nosil->len);
            input_len[size] = input[i].finfo->left_phone_nosil->len;
            size++;
            wtk_strbuf_push(input_feat,input[i].finfo->right_phone_nosil->data,input[i].finfo->right_phone_nosil->len);
            input_len[size] = input[i].finfo->right_phone_nosil->len;
            size++;
            if (1==cs->lexicon->ninput)
            	c = 's';
            else if (0==i)
            	c = 'b';
            else if (i==cs->lexicon->ninput-1)
            	c = 'e';
            else
            	c = 'm';
            wtk_strbuf_push_c(input_feat,c);
            input_len[size] = 1;
            size++;
    	}else
    	{
    		if (0==wtk_string_cmp_s(input[i].finfo->left_type, "others"))
    		{
    			wtk_strbuf_push(input_feat,input[i].finfo->left_phone->data,input[i].finfo->left_phone->len);
    			input_len[size] = input[i].finfo->left_phone->len;
    		}else{
                wtk_strbuf_push(input_feat,input[i].finfo->left_type->data,input[i].finfo->left_type->len);
                input_len[size] = input[i].finfo->left_type->len;
    		}
    		size++;
    		if (0==wtk_string_cmp_s(input[i].finfo->right_type, "others"))
    		{
                wtk_strbuf_push(input_feat,input[i].finfo->right_phone->data,input[i].finfo->right_phone->len);
                input_len[size] = input[i].finfo->right_phone->len;
    		}else
    		{
                wtk_strbuf_push(input_feat,input[i].finfo->right_type->data,input[i].finfo->right_type->len);
                input_len[size] = input[i].finfo->right_type->len;
    		}

    		if (!(0==wtk_string_cmp_s(input[i].finfo->left_type, "others") || 0==wtk_string_cmp_s(input[i].finfo->left_type, "silence")))
    		{
    			size++;
                wtk_strbuf_push(input_feat,input[i].finfo->left_phone->data,input[i].finfo->left_phone->len);
                input_len[size] = input[i].finfo->left_phone->len;
    		}

    		if (!(0==wtk_string_cmp_s(input[i].finfo->right_type, "others") || 0==wtk_string_cmp_s(input[i].finfo->right_type, "silence")))
    		{
    			size++;
                wtk_strbuf_push(input_feat,input[i].finfo->right_phone->data,input[i].finfo->right_phone->len);
                input_len[size] = input[i].finfo->right_phone->len;
    		}
    	}
    	//wtk_debug("feat: %.*s\n", input_feat->pos, input_feat->data);
        if (in_set)
        {
            units = wtk_trietree_unit_get(root[input[i].word_id],input_feat->data,input_len, size, &nunit);
            if(units==NULL)
            {
            	//no units. means not in set.
            	ret=-1; goto end;
            }
        }else
        {
            for(deep=-1;deep<size;++deep)
            {
                if(deep==-1)
                {
                    units2 = wtk_trietree_unit_get_root(root[input[i].word_id],&nunit2);
                    if(nunit2<cs->cfg->n_context_presele) break;
                }else
                {
                    units = wtk_trietree_unit_get_any(root[input[i].word_id],input_feat->data,input_len,&nunit,deep);
                    if(units==NULL) break;
                    if(nunit<cs->cfg->n_context_presele) break;
                }
            }
        }
        if(units==NULL)
        {
        	input[i].unit_id = units2;
            input[i].nunit = nunit2;
        }else
        {
        	input[i].unit_id = units;
            input[i].nunit = nunit;
        }
    }

end:
	wtk_strbuf_delete(input_feat);

    return ret;
}

int update_input_flabel(wtk_cosynthesis_lexicon_input_t *input,hts_lab *lab,int is_right)
{
	int ret=0;
    char *bound;
    char *lphone;
    char *rphone;
    int rlen,llen;
    int idx=-1,i;
    if(is_right)
    {
        rphone = strstr(lab->lab,"+");
        rphone+=1;
        bound = strstr(lab->lab,"=");
        rlen = bound-rphone;
        //wtk_debug("lab=%s\n", lab->lab);
        //wtk_debug("rphone=%.*s rlen=%d\n", rlen,rphone, rlen);
        input->finfo->right_phone=wtk_string_dup_data(rphone,rlen);
        for(i=0;i<sizeof(rphone_list)/sizeof(rphone_list[0]);++i)
        {
            if(wtk_string_cmp(input->finfo->right_phone,(char*)rphone_list[i],strlen(rphone_list[i]))==0)
            {
                idx = i;
                break;
            }
        }
        if(idx<0)
        {
            wtk_debug("error phone\n");
            ret=-1; goto end;
        }
        input->finfo->right_type=wtk_string_dup_data((char*)rtype_list[idx],strlen(rtype_list[idx]));
        if (0==wtk_data_cmp(rphone, rlen, "pau", strlen("pau")))
        {
            rphone = strstr(lab->lab,"=");
            rphone+=1;
            bound = strstr(lab->lab,"@");
            rlen = bound-rphone;
            input->finfo->right_phone_nosil=wtk_string_dup_data(rphone,rlen);
        }else
        	input->finfo->right_phone_nosil=wtk_string_dup_data(rphone,rlen);
    }else
    {
        lphone = strstr(lab->lab,"^");
        bound = strstr(lab->lab,"-");
        lphone+=1;
        llen = bound - lphone;
        //wtk_debug("lab=%s\n", lab->lab);
        //wtk_debug("lphone=%.*s llen=%d\n", llen,lphone, llen);
        input->finfo->left_phone=wtk_string_dup_data(lphone,llen);
        for(i=0;i<sizeof(lphone_list)/sizeof(lphone_list[0]);++i)                                
        {
            if(wtk_string_cmp(input->finfo->left_phone, (char*)lphone_list[i],strlen(lphone_list[i]))==0)
            { 
                idx = i;
                break;
            } 
        }
        if(idx<0)
        {
            wtk_debug("error phone\n");
            ret=-1; goto end;
        }
        input->finfo->left_type=wtk_string_dup_data((char*)ltype_list[idx],strlen(ltype_list[idx]));
        if (0==wtk_data_cmp(lphone, llen, "pau", strlen("pau")))
        {
            lphone = lab->lab;
            bound = strstr(lab->lab,"^");
            llen = bound-lphone;
            input->finfo->left_phone_nosil=wtk_string_dup_data(lphone,llen);
        }else
        	input->finfo->left_phone_nosil=wtk_string_dup_data(lphone,llen);
    }

end:
	return ret;
}

void wtk_cosynthesis_update_trie_input(wtk_cosynthesis_t *cs,hts_lab *lab)
{
    wtk_cosynthesis_lexicon_input_t *input;
    int ninput = cs->lexicon->ninput;
    int i,j=0,k,len;
    // wtk_larray_t *a;
    char* xpos;
    int xl;
    char pos,tmp;
    char lst_pos;
    hts_lab *lab2 = lab;
    hts_lab *lst_lab;
    int **phone_range;

    phone_range=(int**)wtk_calloc(cs->lexicon->cfg->nwrds, sizeof(int*));
    for(i=0; i<cs->lexicon->cfg->nwrds; i++)
    {
    	phone_range[i]=(int*)wtk_calloc(3,sizeof(int));
    }
    input = cs->lexicon->input;

    for(i=0;lab != NULL; ++i,lab = lab->next)
    {
    	//wtk_debug("lab=%s\n", lab->lab);
        xpos = strstr(lab->lab,"E:");
        xpos = strstr(xpos,"@");
        xl = strstr(xpos,"+") - xpos -1;
        if(xl==1)
        {
            pos = *(xpos+1);
        }else
        {
            pos = *(xpos+1);
            tmp = *(xpos+2);
            pos = pos*10 + tmp;
        }
        if(i==0)
        {
            lst_pos = pos;
        }
        if(i>0)
        {
            if(pos!=lst_pos)
            {
                if(lst_pos!='x')
                {
                    if(j>0)
                    {
                        update_input_flabel(&input[j-1],lst_lab,1);
                        phone_range[j-1][1]=i-1;
                    }
                }
                lst_pos = pos;
                if(pos =='x')
                {
                    phone_range[j-1][2]=i;//sil pos
                    continue;
                }
                update_input_flabel(&input[j],lab,0);
                phone_range[j][0]=i;
                j++;
            }
        }
        lst_lab = lab;
    }
    j=0;
    for(i=0;lab2 != NULL; ++i,lab2 = lab2->next)
    {
        if(i==0)continue;
        if(j>0 && phone_range[j-1][2] == i)
        {
            if(j!=ninput)
            {
                input[j-1].finfo->sil_flabel = wtk_string_dup_data(lab2->lab,strlen(lab2->lab));
                // wtk_debug("[%d] %.*s\n",j-1,input[j-1].finfo->sil_flabel->len,input[j-1].finfo->sil_flabel->data);
            }
            continue;
        } 
        if(phone_range[j][0]<=i && phone_range[j][1]>=i)
        {
            if(input[j].finfo->flabel==NULL)
            {
                k=0;
                len = phone_range[j][1] - phone_range[j][0] + 1;
                input[j].finfo->flabel = (wtk_string_t**)wtk_malloc(sizeof(wtk_string_t*)*len);
                input[j].finfo->nphone = len;
            }
            input[j].finfo->flabel[k++] = wtk_string_dup_data(lab2->lab,strlen(lab2->lab));
            // wtk_debug("[%d] len=%d %.*s\n",j,input[j].finfo->nphone,input[j].finfo->flabel[k-1]->len,input[j].finfo->flabel[k-1]->data);
            len--;
            if(len==0)
            {
                j++;
            }
        }
    }  
    for(i=0; i<cs->lexicon->cfg->nwrds; i++)
    {
    	wtk_free(phone_range[i]);
    }
    wtk_free(phone_range);
}
//
//char *flab[]=
//{
//"x^x-pau+ay=l@1_1/A:0_0_0/B:x-x-x@x-x&x-x#x-x$x-x!x-x;x-x|x/C:1+0+1/D:0_0/E:x+x@x+x&x+x#x+x/F:content_1/G:0_0/H:x=x^0=2|L-L%/I:4=4/J:4+4-1",
//"x^pau-ay+l=ah@1_1/A:0_0_0/B:1-0-1@1-1&1-4#1-3$0-0!0-1;0-0|ay/C:1+0+3/D:0_0/E:content+1@1+4&1+3#0+1/F:content_1/G:0_0/H:4=4^1=1|L-L%/I:0=0/J:4+4-1",
//"pau^ay-l+ah=v@1_3/A:1_0_1/B:1-0-3@1-1&2-3#2-2$0-0!1-2;0-0|ah/C:0+0+2/D:content_1/E:content+1@2+3&2+2#1+2/F:det_1/G:0_0/H:4=4^1=1|L-L%/I:0=0/J:4+4-1",
//"ay^l-ah+v=dh@2_2/A:1_0_1/B:1-0-3@1-1&2-3#2-2$0-0!1-2;0-0|ah/C:0+0+2/D:content_1/E:content+1@2+3&2+2#1+2/F:det_1/G:0_0/H:4=4^1=1|L-L%/I:0=0/J:4+4-1",
//"l^ah-v+dh=ah@3_1/A:1_0_1/B:1-0-3@1-1&2-3#2-2$0-0!1-2;0-0|ah/C:0+0+2/D:content_1/E:content+1@2+3&2+2#1+2/F:det_1/G:0_0/H:4=4^1=1|L-L%/I:0=0/J:4+4-1",
//"ah^v-dh+ah=k@1_2/A:1_0_3/B:0-0-2@1-1&3-2#2-1$0-0!1-1;0-0|ah/C:1+0+3/D:content_1/E:det+1@3+2&2+1#1+1/F:content_1/G:0_0/H:4=4^1=1|L-L%/I:0=0/J:4+4-1",
//"v^dh-ah+k=ae@2_1/A:1_0_3/B:0-0-2@1-1&3-2#2-1$0-0!1-1;0-0|ah/C:1+0+3/D:content_1/E:det+1@3+2&2+1#1+1/F:content_1/G:0_0/H:4=4^1=1|L-L%/I:0=0/J:4+4-1",
//"dh^ah-k+ae=t@1_3/A:0_0_2/B:1-0-3@1-1&4-1#3-1$0-0!2-0;0-0|ae/C:0+0+0/D:det_1/E:content+1@4+1&3+1#2+0/F:0_0/G:0_0/H:4=4^1=1|L-L%/I:0=0/J:4+4-1",
//"ah^k-ae+t=pau@2_2/A:0_0_2/B:1-0-3@1-1&4-1#3-1$0-0!2-0;0-0|ae/C:0+0+0/D:det_1/E:content+1@4+1&3+1#2+0/F:0_0/G:0_0/H:4=4^1=1|L-L%/I:0=0/J:4+4-1",
//"k^ae-t+pau=x@3_1/A:0_0_2/B:1-0-3@1-1&4-1#3-1$0-0!2-0;0-0|ae/C:0+0+0/D:det_1/E:content+1@4+1&3+1#2+0/F:0_0/G:0_0/H:4=4^1=1|L-L%/I:0=0/J:4+4-1",
//"ae^t-pau+x=x@1_1/A:1_0_3/B:x-x-x@x-x&x-x#x-x$x-x!x-x;x-x|x/C:0+0+0/D:content_1/E:x+x@x+x&x+x#x+x/F:0_0/G:0_0/H:x=x^1=1|L-L%/I:0=0/J:4+4-1",
//};

int wtk_cosynthesis_front_end(wtk_cosynthesis_t *cs,char* data)
{
    int ret=0;
#ifdef USE_PHRASE
    //printf("data:%s\n", data);
    wtk_cosynthesis_phrase_process(cs->phrase,cs->lexicon->word_seq,cs->lexicon->ninput);
    data = cs->phrase->output;
    //printf("phrase: %s\n",cs->phrase->output);
#endif
    if (cs->lab)
    {
    	goto end;
    }
    cs->lab = syn_speech_to_hts_lab(cs->front_end,data);
    if(!cs->lab)
    {
        ret = -1;
        printf("front end err!\n");
        goto end;
    }
    //print_htslab2(cs->lab,NULL);
    wtk_cosynthesis_update_trie_input(cs,cs->lab);
end:
    return ret;
}

typedef struct
{
    float kld;
    uint16_t unit_id;
}kld_unit_pair_t;

int kld_unit_pair_compare(const void*x,const void *y)
{
    kld_unit_pair_t *a = (kld_unit_pair_t*)x;
    kld_unit_pair_t *b = (kld_unit_pair_t*)y;
    return a->kld - b->kld;
}

float kld_unit_pair_compare2(void *app_data, void*x, void *y)
{
    kld_unit_pair_t *a = (kld_unit_pair_t*)x;
    kld_unit_pair_t *b = (kld_unit_pair_t*)y;
    //wtk_debug("a->kld=%f b->kld=%f\n", a->kld, b->kld);
    //exit(0);
    return a->kld - b->kld;
}

int wtk_cosynthesis_back_end(wtk_cosynthesis_t *cs)
{
    int ret=0;
    char *flab;
    int len;
    int i,j,k;//,kk;
    wtk_queue_node_t *qn;
    wtk_cosynthesis_hmm_lc_t *lh;
    
    for(i=0;i<cs->lexicon->ninput;++i)
    {
    	//wtk_debug("nphone=%d sil_flabel=%p\n",cs->lexicon->input[i].finfo->nphone, cs->lexicon->input[i].finfo->sil_flabel)
        for(j=0;j<cs->lexicon->input[i].finfo->nphone;++j)
        {
            flab = cs->lexicon->input[i].finfo->flabel[j]->data;
            len = cs->lexicon->input[i].finfo->flabel[j]->len;
            //wtk_debug("flab=%.*s\n", len, flab);
            wtk_cosynthesis_backend_process(cs->back_end,flab,len,&cs->lexicon->input[i].wrd_q);
        }
        if(cs->lexicon->input[i].finfo->sil_flabel!=NULL)
        {
            flab = cs->lexicon->input[i].finfo->sil_flabel->data;
            len = cs->lexicon->input[i].finfo->sil_flabel->len;
            //wtk_debug("flab=%.*s\n", len, flab);
            wtk_cosynthesis_backend_process(cs->back_end,flab,len,&cs->lexicon->input[i].wrd_q);
        }
        //exit(0);
    }

    float *dur_mean=NULL;
    float *dur_var=NULL;
    float *lf0_mean=NULL;
    float *lf0_var=NULL;
    float *lf0_weight=NULL;
    float *mcep_mean=NULL;
    float *mcep_var=NULL;
    float *conca_lf0_mean=NULL;
    float *conca_lf0_var=NULL;
    float *conca_lf0_weight=NULL;
    float *conca_mcep_mean=NULL;
    float *conca_mcep_var=NULL;

    float *candi_dur_mean=NULL;
    float *candi_dur_var=NULL;
    float *candi_lf0_mean=NULL;
    float *candi_lf0_var=NULL;
    float *candi_lf0_weight=NULL;
    float *candi_mcep_mean=NULL;
    float *candi_mcep_var=NULL;

    float *sil_dur_mean=NULL;
    float *sil_dur_var=NULL;
    float *sil_lf0_mean=NULL;
    float *sil_lf0_var=NULL;
    float *sil_lf0_weight=NULL;
    float *sil_mcep_mean=NULL;
    float *sil_mcep_var = NULL;
    int nphone, nphn;
    uint16_t candi_uid;
    int wrd_idx;
    wtk_unit_t candi_unit;
    float kld;
    uint16_t *tmp_units;

    int cur_nunit;
    short *cur_unit_ids;
    int pre_nunit;
    short *pre_unit_ids;
    int idx;
    wtk_unit_t *cur_unit=NULL;
    wtk_unit_t *pre_unit=NULL;
    int sil_unit[1]={-1};
    unsigned sil_flg=0;

    float spec_w = cs->back_end->cfg->spec_weight;
    float dur_w = cs->back_end->cfg->dur_weight;
    float pitch_w = cs->back_end->cfg->pitch_weight;
    float conca_spec_w = cs->back_end->cfg->conca_spec_weight;
    float conca_pitch_w = cs->back_end->cfg->conca_pitch_weight;

    for(i=0;i<cs->lexicon->ninput;++i)
    {
    	//wtk_debug("input[%d]\n", i);
        nphone =  cs->lexicon->input[i].finfo->nphone;
        //wtk_debug("input[%d] nphone=%d\n", i,nphone);
        wtk_cosynthesis_mdl_data_new(&dur_mean,&dur_var,&lf0_mean,&lf0_var,&lf0_weight,
        &mcep_mean,&mcep_var,&conca_lf0_mean,&conca_lf0_var,&conca_lf0_weight,&conca_mcep_mean,
        &conca_mcep_var,nphone,&cs->lexicon->cfg->feat_cfg);
        j=0;
        //wtk_debug("input[%d]\n", i);
        for(qn=cs->lexicon->input[i].wrd_q.pop;qn;qn=qn->next)
        {
            lh = data_offset2(qn,wtk_cosynthesis_hmm_lc_t,q_n);
            //wtk_debug("j=%d lh=%p mcepmean=%p mcepvariance=%p\n", j, lh, lh->mcepmean, lh->mcepvariance);
            //print_float(lh->mcepvariance[1]+1, lh->hmm->mcepvsize);
            if(qn->next==NULL && cs->lexicon->input[i].finfo->sil_flabel!=NULL)
            {
                cs->lexicon->input[i].sil_tot_dur = lh->totaldur;
                sil_dur_mean = lh->durmean[1]+1;
                sil_dur_var = lh->durvariance[1]+1;
                sil_lf0_mean = lh->lf0mean[1]+1;
                sil_lf0_var = lh->lf0variance[1]+1;
                sil_lf0_weight = lh->lf0weight[1]+1;
                sil_mcep_mean = lh->mcepmean[1]+1;
                sil_mcep_var = lh->mcepvariance[1] + 1; //ssy 不知道按照上面改的
            }else
            {
                //wtk_debug("mcep_len=%d %d, %d %d\n",lh->hmm->mcepvsize,lh->hmm->conca_mcepvsize,lh->hmm->lf0stream,lh->hmm->conca_lf0stream);
                memcpy(dur_mean+lh->hmm->nstate*j,lh->durmean[1]+1,sizeof(float)*lh->hmm->nstate);
                memcpy(dur_var+lh->hmm->nstate*j,lh->durvariance[1]+1,sizeof(float)*lh->hmm->nstate);
                memcpy(lf0_mean+lh->hmm->lf0stream*j,lh->lf0mean[1]+1,sizeof(float)*lh->hmm->lf0stream);
                memcpy(lf0_var+lh->hmm->lf0stream*j,lh->lf0variance[1]+1,sizeof(float)*lh->hmm->lf0stream);
                memcpy(lf0_weight+lh->hmm->lf0stream*j,lh->lf0weight[1]+1,sizeof(float)*lh->hmm->lf0stream);
                memcpy(mcep_mean+lh->hmm->mcepvsize*j,lh->mcepmean[1]+1,sizeof(float)*lh->hmm->mcepvsize);
                memcpy(mcep_var+lh->hmm->mcepvsize*j,lh->mcepvariance[1]+1,sizeof(float)*lh->hmm->mcepvsize);
                memcpy(conca_lf0_mean+lh->hmm->conca_lf0stream*j,lh->conca_lf0mean[1]+1,sizeof(float)*lh->hmm->conca_lf0stream);
                memcpy(conca_lf0_var+lh->hmm->conca_lf0stream*j,lh->conca_lf0variance[1]+1,sizeof(float)*lh->hmm->conca_lf0stream);
                memcpy(conca_lf0_weight+lh->hmm->conca_lf0stream*j,lh->conca_lf0weight[1]+1,sizeof(float)*lh->hmm->conca_lf0stream);
                memcpy(conca_mcep_mean+lh->hmm->conca_mcepvsize*j,lh->conca_mcepmean[1]+1,sizeof(float)*lh->hmm->conca_mcepvsize);
                memcpy(conca_mcep_var+lh->hmm->conca_mcepvsize*j,lh->conca_mcepvariance[1]+1,sizeof(float)*lh->hmm->conca_mcepvsize);
                j++;
            }
        }
        //kld selected
        if(cs->lexicon->input[i].nunit > cs->cfg->n_kld_presele)
        {
            kld_unit_pair_t *pair = (kld_unit_pair_t*)wtk_malloc(sizeof(kld_unit_pair_t)*cs->lexicon->input[i].nunit);
            for(k=0;k<cs->lexicon->input[i].nunit;++k)
            {   
            	nphn=nphone;
                candi_uid = cs->lexicon->input[i].unit_id[k];
                wrd_idx = get_word_index(cs->lexicon,cs->lexicon->input[i].word_id);
                candi_unit = cs->lexicon->wrds[wrd_idx].unit[candi_uid];
                //wtk_debug("input[%d] wrd_idx=%d wrd_id=%d unit_id[%d]=candi_uid=%d candi_unit=%d\n", i, wrd_idx, cs->lexicon->input[i].word_id, k,candi_uid, candi_unit)
                wtk_cosynthesis_mdl_data_new2(&candi_dur_mean,&candi_dur_var,&candi_lf0_mean,&candi_lf0_var,&candi_lf0_weight,
                &candi_mcep_mean,&candi_mcep_var,candi_unit.nphone,&cs->lexicon->cfg->feat_cfg);
                // copy_data2candi()
                wtk_cosynthesis_backend_get_candi_unit_dur(cs->back_end,candi_dur_mean,candi_dur_var,candi_unit.dur_idx,candi_unit.nphone);
                //print_float(candi_dur_mean, cs->back_end->cfg->hmm->nstate *candi_unit.nphone );
                //print_float(candi_dur_var, cs->back_end->cfg->hmm->nstate *candi_unit.nphone );
                //exit(0);
                wtk_cosynthesis_backend_get_candi_unit_lf0(cs->back_end,candi_lf0_mean,candi_lf0_var,candi_lf0_weight,2,candi_unit.lf0_idx,candi_unit.nphone);
                //print_float(candi_lf0_mean, cs->back_end->cfg->hmm->lf0stream *candi_unit.nphone );
                //print_float(candi_lf0_var, cs->back_end->cfg->hmm->lf0stream *candi_unit.nphone );
                wtk_cosynthesis_backend_get_candi_unit_mcep(cs->back_end,candi_mcep_mean,candi_mcep_var,2,candi_unit.spec_idx,candi_unit.nphone);
                //print_float(candi_mcep_mean, cs->back_end->cfg->hmm->mcepvsize *candi_unit.nphone );
                //print_float(candi_mcep_var, cs->back_end->cfg->hmm->mcepvsize *candi_unit.nphone );
                //exit(0);
                if (nphn > candi_unit.nphone)
                {
#ifdef CONCA_DEBUG
            		wtk_debug("warnning: diff nphone=%d candi_unit.nphone=%d in input[%d/%d] unit[%d/%d]\n", nphone, candi_unit.nphone, i, cs->lexicon->ninput,k,cs->lexicon->input[i].nunit);
#endif
                	nphn = candi_unit.nphone;
                }
                kld = kld_preselect(candi_dur_mean,candi_dur_var,candi_lf0_mean,candi_lf0_var,
                candi_lf0_weight,candi_mcep_mean,candi_mcep_var,dur_mean,dur_var,lf0_mean,lf0_var,
                lf0_weight,mcep_mean,mcep_var,nphn,candi_unit.kld_lf0,cs->lexicon->cfg->feat_cfg.hmm_dur_len,
                cs->lexicon->cfg->feat_cfg.hmm_lf0_len,cs->lexicon->cfg->feat_cfg.hmm_mcep_len);
                //wtk_debug("kld=%f unit_id=%d\n", kld, candi_uid);
                pair[k].kld = kld;
                pair[k].unit_id = candi_uid;
                wtk_cosynthesis_mdl_data_delete(candi_dur_mean,candi_dur_var,candi_lf0_mean,candi_lf0_var,
            candi_lf0_weight,candi_mcep_mean,candi_mcep_var,NULL,NULL,NULL,NULL,NULL);
            }

            //qsort(pair,cs->lexicon->input[i].nunit,sizeof(kld_unit_pair_t),kld_unit_pair_compare);
            wtk_qsort2(pair,cs->lexicon->input[i].nunit,sizeof(kld_unit_pair_t),kld_unit_pair_compare2, NULL);
            tmp_units = (uint16_t*)wtk_heap_malloc(cs->dec_heap, sizeof(uint16_t)*cs->cfg->n_kld_presele);
            for(k=0;k<cs->cfg->n_kld_presele;++k)
            {
                tmp_units[k] = pair[k].unit_id;
            }
            cs->lexicon->input[i].unit_id=tmp_units;
            cs->lexicon->input[i].nunit = cs->cfg->n_kld_presele;
            //wtk_debug("=========tmp_units\n");
            //print_short(tmp_units, cs->cfg->n_kld_presele);
            //exit(0);
            wtk_free(pair);
        }
        idx = get_word_index(cs->lexicon,cs->lexicon->input[i].word_id);
        if(i==0)
        {
            pre_nunit = 1;
            pre_unit_ids = (short *)sil_unit;
            pre_unit = NULL;
            cur_nunit = cs->lexicon->input[i].nunit;
            cur_unit_ids = (short*)cs->lexicon->input[i].unit_id;
            cur_unit =cs->lexicon->wrds[idx].unit;
        }else
        {
            if(sil_flg)
            {
                pre_nunit = 1;
                pre_unit_ids = (short *)sil_unit;
                pre_unit = NULL;
                sil_flg=0;
            }else
            {
                pre_nunit = cur_nunit;
                pre_unit_ids = cur_unit_ids;
                pre_unit = cur_unit;
            }
            cur_nunit = cs->lexicon->input[i].nunit;
            cur_unit_ids = (short*)cs->lexicon->input[i].unit_id;
            cur_unit =cs->lexicon->wrds[idx].unit;
        }
        //wtk_debug("cur_unit_ids\n");
        //wtk_debug("idx=%d i=%d %d cur_unit=%p %d pre_unit=%p\n",idx, i,cur_nunit,cur_unit, pre_nunit,pre_unit);
        ret=wtk_cosynthesis_get_likehood(cs, mcep_mean, mcep_var,cs->lexicon->cfg->feat_cfg.hmm_mcep_len,
                            lf0_mean,lf0_var,lf0_weight,cs->lexicon->cfg->feat_cfg.hmm_lf0_len,
                            dur_mean, dur_var,5,
                            conca_mcep_mean,conca_mcep_var,1,cs->lexicon->cfg->feat_cfg.hmm_mcep_len,
                            conca_lf0_mean,conca_lf0_var,conca_lf0_weight,1,1,
                            cur_unit_ids, cur_nunit,
                            cur_unit,pre_unit_ids, pre_nunit, pre_unit,nphone,
                            &cs->lexicon->input[i].select_cost,&cs->lexicon->input[i].conca_cost,
                            spec_w,dur_w,pitch_w,conca_spec_w,conca_pitch_w);
        if (ret !=0)goto end;
        if(cs->lexicon->input[i].finfo->sil_flabel!=NULL)
        {
            pre_nunit = cur_nunit;
            pre_unit_ids = cur_unit_ids;
            pre_unit = cur_unit;
            sil_flg=1;
            //wtk_debug("i=%d(sil) %d %d\n",i,cur_nunit,pre_nunit);
            ret=wtk_cosynthesis_get_likehood(cs, sil_mcep_mean, sil_mcep_var,cs->lexicon->cfg->feat_cfg.hmm_mcep_len,
                            sil_lf0_mean,sil_lf0_var,sil_lf0_weight,cs->lexicon->cfg->feat_cfg.hmm_lf0_len,
                            sil_dur_mean, sil_dur_var,5,
                            conca_mcep_mean,conca_mcep_var,1,cs->lexicon->cfg->feat_cfg.hmm_mcep_len,
                            conca_lf0_mean,conca_lf0_var,conca_lf0_weight,1,1,
                            (short *)sil_unit, 1,NULL,pre_unit_ids, pre_nunit, pre_unit,nphone,
                            NULL,&cs->lexicon->input[i].sil_conca_cost,spec_w,dur_w,pitch_w,conca_spec_w,conca_pitch_w);
            if (ret!=0)goto end;
        }

        wtk_cosynthesis_mdl_data_delete(dur_mean,dur_var,lf0_mean,lf0_var,lf0_weight,
        mcep_mean,mcep_var,conca_mcep_mean,conca_mcep_var,conca_lf0_mean,conca_lf0_var,conca_lf0_weight);
    }
end:
    return ret;
}


float wtk_cosynthesis_get_conca_cost(float **conca_cost,int cur_idx,int pre_idx)
{
    //wtk_debug("%f,%d %d\n",conca_cost[cur_idx][pre_idx],cur_idx,pre_idx);
    return conca_cost[cur_idx][pre_idx];
}

wtk_cosynthesis_token_t *wtk_cosynthesis_sil_token_new(wtk_cosynthesis_t *cs,int wrd_idx)
{
    wtk_cosynthesis_token_t *tok;
    tok = (wtk_cosynthesis_token_t*)wtk_vpool2_pop(cs->tok_pool);
    tok->unit_id = 0;
    tok->unit = NULL;
    tok->cost = 0.0;
    tok->unit_path = NULL;
    return tok;
}

wtk_cosynthesis_token_t *wtk_cosynthesis_token_new(wtk_cosynthesis_t *cs,int i,int wrd_idx, uint16_t word_id)
{
    wtk_cosynthesis_token_t *tok;
    int idx;
    idx = get_word_index(cs->lexicon,word_id);
    float *select_cost = cs->lexicon->input[wrd_idx].select_cost;
    if(idx<0){
        printf("error\n");
        exit(0);
    }
    tok = (wtk_cosynthesis_token_t*)wtk_vpool2_pop(cs->tok_pool);
    tok->unit_id = cs->lexicon->input[wrd_idx].unit_id[i];
    tok->unit = &(cs->lexicon->wrds[idx].unit[tok->unit_id]);
    if (select_cost)
    	tok->cost = select_cost[i];
    else
    	tok->cost = 0;
    //wtk_debug("wrd_idx=%d tok->cost=%f\n", wrd_idx, tok->cost);
    tok->unit_path = NULL;

    return tok;
}

void wtk_cosynthesis_token_update(wtk_cosynthesis_token_t *pre_token,wtk_cosynthesis_token_t *cur_token,float cost,int idx,int is_sil, wtk_heap_t* h)
{
    int path_len = idx + 1;
    cur_token->cost = cost;

    if(cur_token->unit_path == NULL)
    {
        //cur_token->unit_path = (uint16_t*)wtk_calloc(path_len,sizeof(uint16_t));
    	cur_token->unit_path = (uint16_t*)wtk_heap_zalloc(h, path_len * sizeof(uint16_t));
    }
    //wtk_debug("idx=%d pre_token->unit_path=%p pre_token->unit_id=%d cur_token->unit_path= %p\n", idx,pre_token->unit_path,pre_token->unit_id,cur_token->unit_path);
    if(is_sil)
    {
        if(pre_token->unit_path)
        {
            memcpy(cur_token->unit_path,pre_token->unit_path,sizeof(uint16_t)*(path_len));
        }
    }else
    {
        if(pre_token->unit_path)
        {
            memcpy(cur_token->unit_path,pre_token->unit_path,sizeof(uint16_t)*(path_len-1));
            cur_token->unit_path[path_len-1] = cur_token->unit_id;
        }
    }
    //print_short(cur_token->unit_path,path_len);
    //printf("cur_token->unit_path[1]=%d\n", cur_token->unit_path[1]);
}

void wtk_cosynthesis_token_push(wtk_cosynthesis_t *cs,wtk_cosynthesis_token_t *token)
{
//    if(token->unit_path)
//    {
//        // wtk_debug("delete %p\n",token->unit_path);
//        wtk_free(token->unit_path);
//    }
    wtk_vpool2_push(cs->tok_pool,token);
}

void wtk_cosynthesis_token_clean(wtk_cosynthesis_t *cs)
{
    wtk_queue_node_t *qn;
    wtk_cosynthesis_token_t *token;
    while(1)
    {
        if(cs->cur_token_q)
        {
            qn=wtk_queue_pop(cs->cur_token_q);
            if(!qn){break;}
            token = data_offset(qn,wtk_cosynthesis_token_t,q_n);
            wtk_cosynthesis_token_push(cs,token);
        }else{

            break;
        }
    }
}

void wtk_cosynthesis_pre_tokenq_clean(wtk_cosynthesis_t *cs)
{
    wtk_queue_node_t *qn;
    wtk_cosynthesis_token_t *token;
    while(1)
    {
        qn=wtk_queue_pop(cs->pre_token_q);
        if(!qn){break;}
        token = data_offset(qn,wtk_cosynthesis_token_t,q_n);
        wtk_cosynthesis_token_push(cs,token);
    }
}

void wtk_cosynthesis_unit_decode_start(wtk_cosynthesis_t *cs)
{
    wtk_cosynthesis_token_t *start_tok;
    uint16_t first_id = cs->lexicon->input[0].word_id;
    int n =  cs->lexicon->input[0].nunit;
    int i;

    //wtk_debug("n=%d\n", n);
    for(i=0;i<n;++i)
    {
        start_tok = wtk_cosynthesis_token_new(cs,i,0,first_id);
        //wtk_debug("wrd[%d]-unit[%d] start_tok=%p unit_id=%d unit_path=%p\n", first_id,i, start_tok, start_tok->unit_id, start_tok->unit_path);
        //wtk_debug("start_tok=%p\n", start_tok);
        wtk_queue_push(cs->cur_token_q,&(start_tok->q_n));
        if(start_tok->unit_path == NULL)
        {
        	//start_tok->unit_path = (uint16_t*)wtk_calloc(1,sizeof(uint16_t));
        	start_tok->unit_path = (uint16_t*)wtk_heap_zalloc(cs->dec_heap, 1*sizeof(uint16_t));
        }
        start_tok->unit_path[0] = start_tok->unit_id;
    }
}

int wtk_cosynthesis_unit_decode_run(wtk_cosynthesis_t *cs ,int wrd_index, int word_id)
{
    wtk_queue_t *tmp_q;
    wtk_queue_node_t *qn;
    wtk_cosynthesis_token_t *pre_tok;
    wtk_cosynthesis_token_t *cur_tok;
    int i,j, flag;
    int nunit = cs->lexicon->input[wrd_index].nunit;
    float **conca_cost = cs->lexicon->input[wrd_index].conca_cost;
    float **sil_conca_cost = cs->lexicon->input[wrd_index].sil_conca_cost;
    float min_cost=__FLT_MAX__,tmp_cost, cost;

    //wtk_debug("wtk_cosynthesis_unit_decode_run word_id=%d wrd_idx=%d nunit=%d cur_token_q_len=%d pre_token_q_len=%d\n", word_id, wrd_index, nunit,cs->cur_token_q->length, cs->pre_token_q->length);
	tmp_q = cs->pre_token_q;
	cs->pre_token_q = cs->cur_token_q;
	cs->cur_token_q = tmp_q;
    if(word_id<0) //process sil
    {
        cur_tok = wtk_cosynthesis_sil_token_new(cs,wrd_index);
        j=0;
        cost=cur_tok->cost;
        while(1)
        {
            qn=wtk_queue_pop(cs->pre_token_q);
            //wtk_debug("pre_token_q qn=%p\n", qn);
            if(!qn){break;}
            pre_tok = data_offset(qn,wtk_cosynthesis_token_t,q_n);
            tmp_cost = cost + pre_tok->cost +
            wtk_cosynthesis_get_conca_cost(sil_conca_cost,0,j);
            // cur_cost = t(a) + t(b) + l(a,b);
            //wtk_debug("min_cost=%f tmp_cost=%f\n", min_cost, tmp_cost);
            if(min_cost > tmp_cost)
            {
                min_cost = tmp_cost;
                wtk_cosynthesis_token_update(pre_tok,cur_tok,min_cost,wrd_index,1, cs->dec_heap);
            }
            wtk_cosynthesis_token_push(cs,pre_tok);
            j++;
        }
        wtk_queue_push(cs->cur_token_q,&(cur_tok->q_n));
    }else
    {
        for(i=0;i<nunit;++i)
        {
            j=0;
            cur_tok = wtk_cosynthesis_token_new(cs,i,wrd_index,word_id);
            //wtk_debug("word[%d] unit_id[%d]=%d\n", wrd_index, i,cur_tok->unit_id);
            //wtk_debug("nqueue=%d\n", cs->pre_token_q->length);
            flag=0;
            cost=cur_tok->cost;
            for (qn = cs->pre_token_q->pop; qn;qn=qn->next)
            {
                pre_tok = data_offset(qn,wtk_cosynthesis_token_t,q_n);
                //wtk_debug("i=%d pre_token_q qn=%p cur_tok->cost=%f pre_tok->cost=%f\n",i, qn, cur_tok->cost, pre_tok->cost);
                tmp_cost = cost + pre_tok->cost +
                wtk_cosynthesis_get_conca_cost(conca_cost,i,j);
                // cur_cost = t(a) + t(b) + l(a,b);
                //wtk_debug("min_cost=%f tmp_cost=%f cur_tok->cost=%f pre_tok->cost=%f\n", min_cost, tmp_cost, cur_tok->cost,pre_tok->cost);
                if (!(wrd_index > 1 && !pre_tok->unit_path))
                {
                    if(min_cost > tmp_cost)
                    {
                        min_cost = tmp_cost;
                       	wtk_cosynthesis_token_update(pre_tok,cur_tok,min_cost,wrd_index,0, cs->dec_heap);
                       	flag=1;
                    }
                }
                j++;
            }
            if (1==flag)
            	wtk_queue_push(cs->cur_token_q,&(cur_tok->q_n));
        }
        wtk_cosynthesis_pre_tokenq_clean(cs);
    }

    return 0;
}

void wtk_cosynthesis_get_best_path(wtk_cosynthesis_t *cs)
{
    short *best_path=NULL;
    float best_cost = __FLT_MAX__;
    wtk_queue_node_t *qn;
    wtk_cosynthesis_token_t *token;

    //wtk_debug("====wtk_cosynthesis_get_best_path======\n");
    for(qn = cs->cur_token_q->pop; qn;qn=qn->next)
    {
        token = data_offset(qn,wtk_cosynthesis_token_t,q_n);
        //wtk_debug("token=%p\n", token);
        if(token->unit_path)
        {
            if(best_cost > token->cost)
            {
                best_cost = token->cost;
                //wtk_debug("best_cost=%f\n", best_cost);
                best_path = (short*)token->unit_path;
                //wtk_debug("unit_path=%p\n", best_path);
            }
        }
    }
    //wtk_debug("best_path=%p\n", best_path);
    if (cs->lexicon->ninput > 0)
    	cs->best_cost = best_cost/cs->lexicon->ninput;
    else
    	cs->best_cost = best_cost;
    //wtk_debug("best_cost=%f\n", best_cost);
    cs->best_path = (uint16_t*)best_path;
}

void wtk_cosynthesis_unit_decode_feed(wtk_cosynthesis_t *cs)
{
    int i;
    int n = cs->lexicon->ninput;
    int word_id;

    //wtk_debug("nwrd=%d\n", n);
    //wtk_debug("sil_flabel=%p\n", cs->lexicon->input[0].finfo->sil_flabel);
    if(cs->lexicon->input[0].finfo->sil_flabel!=NULL)
    {
        wtk_cosynthesis_unit_decode_run(cs,0,-1);
    }
    for (i=1;i<n;++i)
    {
        word_id = cs->lexicon->input[i].word_id;
        wtk_cosynthesis_unit_decode_run(cs,i,word_id);
        if(cs->lexicon->input[i].finfo->sil_flabel!=NULL)
        {
            wtk_cosynthesis_unit_decode_run(cs,i,-1);
        }
    }
}

float wtk_cosynthesis_get_conca_cost_inset(wtk_unit_t *prev_unit, wtk_unit_t *cur_unit, int len)
{
	int i;
	float v, *spec_l, *spec_r, cost;

	cost=0;
	spec_l = cur_unit->spec_l;
	spec_r = prev_unit->spec_r;
	for (i=0; i < len; i++)
	{
		v = spec_l[i]-spec_r[i];
		cost += v * v;
	}
	//wtk_debug("cost=%f\n", cost);
	return cost;
}

int wtk_cosynthesis_unit_decode_run_inset(wtk_cosynthesis_t *cs ,int wrd_index, int word_id)
{
    wtk_queue_t *tmp_q;
    wtk_queue_node_t *qn;
    wtk_cosynthesis_token_t *pre_tok, *best_pre_tok;
    wtk_cosynthesis_token_t *cur_tok;
    int i,j;
    int nunit = cs->lexicon->input[wrd_index].nunit;
    float min_cost=__FLT_MAX__,tmp_cost, cost;

	tmp_q = cs->pre_token_q;
	cs->pre_token_q = cs->cur_token_q;
	cs->cur_token_q = tmp_q;
    for(i=0;i<nunit;++i)
    {
        cur_tok = wtk_cosynthesis_token_new(cs,i,wrd_index,word_id);
        cost = cur_tok->cost;
        min_cost=__FLT_MAX__;
        for (j=0, qn = cs->pre_token_q->pop; qn;j++,qn=qn->next)
        {
            pre_tok = data_offset(qn,wtk_cosynthesis_token_t,q_n);
            //wtk_debug("cur_tok->cost=%f pre_tok->cost=%f\n", cost, pre_tok->cost);
            tmp_cost = cost + pre_tok->cost + wtk_cosynthesis_get_conca_cost_inset(pre_tok->unit, cur_tok->unit, cs->lexicon->cfg->feat_cfg.spec_llen);
            //wtk_debug("pre=%d cur=%d tmp_cost=%f\n", pre_tok->unit_id, cur_tok->unit_id, tmp_cost);
            if(min_cost > tmp_cost)
            {
                min_cost = tmp_cost;
                best_pre_tok = pre_tok;
            }
        }
        //wtk_debug("best_pre_tok id=%d cur_tok id=%d min_cost=%f\n", best_pre_tok->unit_id, cur_tok->unit_id, min_cost);
        wtk_cosynthesis_token_update(best_pre_tok,cur_tok,min_cost,wrd_index,0, cs->dec_heap);
        wtk_queue_push(cs->cur_token_q,&(cur_tok->q_n));
    }
    wtk_cosynthesis_pre_tokenq_clean(cs);

    return 0;
}

void wtk_cosynthesis_unit_decode_feed_inset(wtk_cosynthesis_t *cs)
{
    int i;
    int n = cs->lexicon->ninput;
    int word_id;

    for (i=1;i<n;++i)
    {
        word_id = cs->lexicon->input[i].word_id;
        wtk_cosynthesis_unit_decode_run_inset(cs,i,word_id);
    }
}

void wtk_cosynthesis_unit_decode(wtk_cosynthesis_t *cs, int inset)
{
    wtk_cosynthesis_unit_decode_start(cs);
    if (inset)
    	wtk_cosynthesis_unit_decode_feed_inset(cs);
    else
    	wtk_cosynthesis_unit_decode_feed(cs);
    wtk_cosynthesis_get_best_path(cs);
}

char* wtk_cosynthesis_pre_process2(char *in)
{
	wtk_strbuf_t *buf;
    int i;
    char* in2;
    int strl=strlen(in);

    buf=wtk_strbuf_new(128,1);
    for(i=0;i<strl;++i)
    {
        //trip ' space/space '
//    	if ((0==i || i==strl-1)&& '\''==in[i])
//    		continue;
//        if (' '==in[i] && buf->pos >0 && '\''==buf->data[buf->pos-1])
//        {
//        	buf->pos--;
//        }
//        if ('\''==in[i] && buf->pos >0 && ' '==buf->data[buf->pos-1])
//        {
//        	continue;
//        }

        if(in[i]>64 && in[i]<91)
        {
            wtk_strbuf_push_c(buf,in[i]+32);
        }
        else
        	wtk_strbuf_push_c(buf,in[i]);

    	//tmp design
    	//for US -> U S
    	if (i-1<strl && 'U' == in[i])
    	{
    		if (((i>0 && ' '==in[i-1]) || 0==i) && ('S' == in[i+1] || 'N' == in[i+1]))
    		{
    			wtk_strbuf_push_c(buf,' ');
    		}
    	}
    }
    wtk_strbuf_push_c(buf, 0);
    in2 = (char*)wtk_calloc(buf->pos,sizeof(char));
    memcpy(in2,buf->data,sizeof(char)*(buf->pos));
    for(i=0;i<buf->pos;++i)
    {
        if(ispunct(in2[i]))
        {
            if(in2[i]!=39 && in2[i]!=45) //"\'-"
            {
                in2[i]=32; //" "
            }
        }
    }
    wtk_strbuf_delete(buf);
    return in2;
}

//see valid characters.
int wtk_cosynthesis_is_check(wtk_cosynthesis_t *cs, char *data, int bytes)
{
    int ret = 0;
    char *p = data;
    char *end = data + bytes;
    
    //wtk_debug("data:%.*s\n", bytes,data);
    while(p < end){
        if(*p >= 48 && *p <= 57){   //0 -- 9
            goto loop;
        }else if( *p >= 65 && *p <= 90){ //A --- Z
            goto loop;
        }else if( *p >= 97 && *p <= 122){ //a --- z
            goto loop;
        }else if( *p == '.' || *p == '\'' || *p == '-' || *p == '?' || *p == '!'  || *p == ',' || *p == ' '){ // . ' - ? ! , 
            goto loop;
        }
        ret = -1;
        goto end;
loop:
        p += 1;
    }
end:
    return ret;
}

//生成新的word text
char* wtk_cosynthesis_new_text(wtk_cosynthesis_t *cs)
{
    char *ret = NULL;
    int n = cs->lexicon->ninput;
    int i = 0;
    wtk_strbuf_t *buf = wtk_strbuf_new(256,1.0f);
    for(i = 0; i < n; ++i){
        if(buf->pos > 0){
            wtk_strbuf_push_c(buf,' ');
        }
        wtk_strbuf_push(buf,cs->lexicon->word_seq[i],strlen(cs->lexicon->word_seq[i]));
    }
    ret = wtk_heap_dup_str2(cs->heap,buf->data,buf->pos);
    wtk_strbuf_delete(buf);
    return ret;
}

int wtk_consynthesis_txtcheck(wtk_cosynthesis_t *cs, char* data, int bytes)
{
//    if (bytes <=0 ){
//    	wtk_debug("null data\n");
//    	wtk_cosynthesis_output_set(cs->output, QTK_COSYN_STATUS_ERR_INPUT, 0,"null data",strlen("null data"));
//    	return -1;
//    }
    if(wtk_cosynthesis_is_check(cs,data,bytes)){    //刘雍要求
        wtk_debug("have err Symbol\n");
        wtk_cosynthesis_output_set(cs->output, QTK_COSYN_STATUS_ERR_INPUT, 0,"have err Symbol",strlen("have err Symbol"));
        return -2;
    }

    return 0;
}

char* wtk_consynthesis_txtnol(wtk_cosynthesis_t *cs, char* data, int bytes)
{
    return wtk_cosynthesis_pre_process2(data);
}

void wtk_cosynthesis_loadinfo(wtk_cosynthesis_t *cs)
{
	int i, idx;

	wtk_cosynthesis_lexicon_loadinput(cs->lexicon);

	for (i=0; i < cs->lexicon->ninput; i++)
	{
		idx=cs->lexicon->input[i].word_id;
		if (NULL==cs->mtrie->root[idx])
			wtk_trietree_readitem(cs->heap, cs->mtrie->trie_fp, cs->mtrie->root, idx, &cs->cfg->trie_cfg.root_pos[idx], 1);
		if (NULL==cs->mtrie->root_inset[idx])
			wtk_trietree_readitem(cs->heap, cs->mtrie->trie_inset_fp, cs->mtrie->root_inset, idx, &cs->cfg->trie_cfg.root_inset_pos[idx], 0);
	}
}

/**
 * reset all needed words unit info and feat info.
 */
void wtk_cosynthesis_cleaninfo(wtk_cosynthesis_t *cs)
{
	int i, idx;

	wtk_cosynthesis_lexicon_cleaninput(cs->lexicon);
	for (i=0; i < cs->lexicon->ninput; i++)
	{
		idx=cs->lexicon->input[i].word_id;
	    if(cs->mtrie->root[idx])
	    {
	    	wtk_trietree_delete(cs->mtrie->root[idx]);
	    	cs->mtrie->root[idx]=NULL;
	    }
	    if(cs->mtrie->root_inset[idx])
	    {
	    	wtk_trietree_delete(cs->mtrie->root_inset[idx]);
	    	cs->mtrie->root_inset[idx]=NULL;
	    }
	}
}
int wtk_cosynthesis_feed(wtk_cosynthesis_t *cs, char* data, int bytes)
{
    int ret=0, nwrd;
    char *new_data = NULL;

    if(!cs->cfg->use_auth){
    	wtk_cosynthesis_output_set(cs->output, QTK_COSYN_STATUS_ERR_USEAUTH, 0,0,0);
        return -1;
    }

    ret=wtk_consynthesis_txtcheck(cs ,data, bytes);
    if(ret!=0){
    	return ret;
    }
    cs->output->snt_num++;

    data=wtk_consynthesis_txtnol(cs ,data, bytes);

    //words parser
    nwrd=wtk_cosynthesis_lexicon_txtparser(cs->lexicon, data, bytes);
    if (nwrd <= 0) goto end;

    //load input unit info and feat info
    if (cs->lexicon->cfg->use_fp)
    	wtk_cosynthesis_loadinfo(cs);

    //text with all words
    new_data = wtk_cosynthesis_new_text(cs);
    wtk_strbuf_push(cs->proc_txt, new_data, strlen(new_data));

    ret=wtk_cosynthesis_front_end(cs,new_data);
    if(ret<0){goto end;}
    //check in set?
    if (cs->cfg->trie_cfg.load_all)
    	ret=wtk_cosynthesis_process_trietree(cs, cs->cfg->trie_cfg.root_inset, 1);
    else
    	ret=wtk_cosynthesis_process_trietree(cs, cs->mtrie->root_inset, 1);
    if(0==ret)
        wtk_cosynthesis_unit_decode(cs, 1);

    if (cs->best_cost > cs->cfg->inset_thd)
    {
    	wtk_cosynthesis_unit_decode_reset(cs);
    	if (cs->cfg->trie_cfg.load_all)
    		ret=wtk_cosynthesis_process_trietree(cs, cs->cfg->trie_cfg.root, 0);
    	else
    		ret=wtk_cosynthesis_process_trietree(cs, cs->mtrie->root, 0);
        if(ret<0){goto end;}
        //outset
        ret=wtk_cosynthesis_back_end(cs);
        if(ret<0)
        {
        	wtk_cosynthesis_output_set(cs->output, QTK_COSYN_STATUS_ERR_PROC, 0,0,0);
            goto end;
        }
        wtk_cosynthesis_unit_decode(cs, 0);
        ret=wtk_cosynthesis_process_wsola(cs, 0);
        if (ret < 0)
        {
        	wtk_cosynthesis_output_set(cs->output, QTK_COSYN_STATUS_ERR_PROC, 0,0,0);
        	goto end;
        }
    }else{
    	//inset
        ret=wtk_cosynthesis_process_wsola(cs, 1);
        if (ret < 0)
        {
        	wtk_cosynthesis_output_set(cs->output, QTK_COSYN_STATUS_ERR_PROC, 0,0,0);
        	goto end;
        }
        wtk_cosynthesis_output_set(cs->output, QTK_COSYN_STATUS_SUCCESS, cs->best_cost,0,0);
    }

end:
	if(data)
		wtk_free(data);

    return ret;
}

int wtk_cosynthesis_feedm(wtk_cosynthesis_t *cs,char* data, int bytes, const char* sep)
{
	wtk_strbuf_t *buf=NULL;
	char *p,*pe, *lp, c;
	int curi, reali, ret=0;

	if (sep)
	{
		//strip dup sep
		for(curi=0, reali=0; curi<bytes; curi++)
		{
			if(data[curi]==*sep && (curi+1<bytes && data[curi+1]==*sep))
			{
				continue;
			}else
			{
				data[reali]=data[curi];
				reali++;
			}
		}
		data[reali]=0;
		bytes=reali;
	}
    if (bytes <=0 ){
    	wtk_cosynthesis_output_set(cs->output, QTK_COSYN_STATUS_ERR_INPUT, 0,"null data",strlen("null data"));
    	wtk_debug("NULL data\n");
    	ret=-1; goto end;
    }
    buf=wtk_strbuf_new(128, 1);
    wtk_strbuf_push(cs->raw_txt, data, bytes);

    lp=p=data;
    pe=data+bytes;
    if (sep!=NULL)
    {
    	c=*sep;
        while((p=wtk_str_chr(lp, pe-lp, c))!=NULL)
        {
        	wtk_strbuf_reset(buf);
        	wtk_strbuf_push(buf, lp, p-lp);
        	wtk_strbuf_push_c(buf, 0);
        	if (buf->pos > 1)
        	{
            	ret=wtk_cosynthesis_feed(cs, buf->data, strlen(buf->data));
            	if (ret!=0 || !wtk_cosynthesis_output_issucc(cs->output))
            	{
            		ret=-1; goto end;
            	}
            	wtk_cosynthesis_reset(cs);
        	}
        	p++;
        	lp=p;
        	if (lp >=pe)break;
        }
    }

    if (lp < pe)
    {
    	wtk_strbuf_reset(buf);
    	wtk_strbuf_push(buf, lp, pe-lp);
    	wtk_strbuf_push_c(buf, 0);
    	if (buf->pos > 1)
    	{
        	ret=wtk_cosynthesis_feed(cs, buf->data, strlen(buf->data));
        	if (ret!=0 || !wtk_cosynthesis_output_issucc(cs->output))
        	{
        		ret=-1; goto end;
        	}
    	}
    }

end:
	cs->output->isend=1;
	wtk_cosynthesis_audio_notify(cs, 0 ,0);
	if (buf)
		wtk_strbuf_delete(buf);

    return ret;
}

void wtk_cosynthesis_audio_notify(wtk_cosynthesis_t *cs,short *data, int len)
{
	if (data)
	{
	    wtk_cosynthesis_output_append(cs->output, data, len, (short*)cs->best_path,cs->lexicon->ninput, cs->best_cost);
	}
	if (cs->output->isend)
	{
		cs->notify(cs->ths, (short*)cs->output->audiodata->data, cs->output->audiodata->pos/2, (short*)cs->output->unitid->data, cs->output->unitid->pos/2);
	}
}


void wtk_cosynthesis_set_notify(wtk_cosynthesis_t *cs,void *ths,wtk_cosynthesis_notity_f notify)
{
    cs->notify = notify;
    cs->ths = ths;
}

char* wtk_cosynthesis_get_rawtxt(wtk_cosynthesis_t *cs)
{
	if (cs->raw_txt->pos > 0)
	{
		wtk_strbuf_push_c(cs->raw_txt, 0);
		return cs->raw_txt->data;
	}
	return NULL;
}
char* wtk_cosynthesis_get_proctxt(wtk_cosynthesis_t *cs)
{
	if (cs->proc_txt->pos > 0){
		wtk_strbuf_push_c(cs->proc_txt, 0);
		return cs->proc_txt->data;
	}
	return NULL;
}

wtk_cosynthesis_info_t* wtk_cosynthesis_getOutput(wtk_cosynthesis_t *cs)
{
	wtk_cosynthesis_info_t *info;
    float loss;

    info = (wtk_cosynthesis_info_t*)wtk_heap_zalloc(cs->heap,sizeof(*info));

    info->state = cs->output->state;
//    info->in = cs->output->in;
    info->loss = cs->output->loss;
    if (cs->output->log->pos > 0)
    	info->log = cs->output->log->data;
    else
    	info->log = NULL;
    //continue remedy
    //wtk_debug("ninput=%d\n", cs->lexicon->ninput);
    loss = cs->output->loss;
    //wtk_debug("raw loss=%f\n", loss);

    if(loss > 0.0f){
    	loss = 1+loss/10;
    }else{
    	loss = cs->output->wrds_tot;
        wtk_sigmoid(&loss,1);
    }

    if (loss >= cs->cfg->max_loss)
    {
    	loss = cs->cfg->max_loss;
    }

    info->loss = loss;

    return info;
}
