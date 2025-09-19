#include "wtk_cosynthesis_lexicon.h"
#include "wtk/core/wtk_str.h"
//#include "third/lame/include/lame.h"
#include <stdio.h>
#include "wtk_cosynthesis_output.h"

#ifdef WTK_ED_ALGORITHM
int wtk_cosynthesis_lexicon_ed_path_clean(wtk_cosynthesis_lexicon_t *lex);
#endif
void wtk_cosynthesis_lexicon_init(wtk_cosynthesis_lexicon_t *lex);

wtk_cosynthesis_lexicon_t *wtk_cosynthesis_lexicon_new(wtk_cosynthesis_lexicon_cfg_t *cfg)
{
	FILE *f;
	int ret=0;
    wtk_cosynthesis_lexicon_t *lex = wtk_malloc(sizeof(wtk_cosynthesis_lexicon_t));
    lex->cfg = cfg;
    lex->ed = (wtk_ed_t*)wtk_malloc(sizeof(wtk_ed_t));
    lex->ed_pathq = wtk_queue_new();
    lex->heap = wtk_heap_new(1024);
    lex->ed_path_pool = wtk_vpool2_new(sizeof(wtk_ed_path_t),100);
    lex->wrds = (wtk_word_t*)wtk_calloc(lex->cfg->nwrds , sizeof(wtk_word_t));

    wtk_cosynthesis_lexicon_init(lex);

    lex->g_audio_res_fp=NULL;
    lex->g_feat_res_fp=NULL;

    if(cfg->use_fp && cfg->unit_fn)
    {
        if (cfg->rbin)
        {
        	f = fopen(cfg->rbin->fn,"rb");
        }else
        {
            f = fopen(cfg->unit_fn,"rb");
            if(!f)
            {
                printf("Read %s FAILED\n",cfg->unit_fn);
                ret=-1;goto end;
            }
        }
        lex->g_audio_res_fp=f;
    }
    if(cfg->use_fp && cfg->feat_fn)
    {
    	if (cfg->rbin)
    	{
        	f = fopen(cfg->rbin->fn,"rb");
    	}else
    	{
            f = fopen(cfg->feat_fn,"rb");
            if(!f)
            {
                printf("Read %s FAILED\n",cfg->feat_fn);
                ret=-1;goto end;
            }
    	}
        lex->g_feat_res_fp=f;
    }
end:
	if (ret < 0)
	{
		wtk_cosynthesis_lexicon_delete(lex);
		wtk_free(lex);
		lex=NULL;
	}

    return lex;
}

void wtk_cosynthesis_lexicon_init(wtk_cosynthesis_lexicon_t *lex)
{
    wtk_heap_reset(lex->heap);
    wtk_vpool2_reset(lex->ed_path_pool);
    wtk_queue_init(lex->ed_pathq);
    memcpy(lex->wrds, lex->cfg->wrds, lex->cfg->nwrds * sizeof(wtk_word_t));

    lex->min_edit_distance = lex->cfg->max_edit_distance;
//    lex->output = NULL;  //outer var. don't change.
    lex->ninput = 0;
    lex->input = NULL;
    lex->min_path = NULL;
    lex->min_path_map = NULL;
    lex->match_snt_idx = -1;
    lex->min_path_len = 0;
    lex->input = NULL;
}

void wtk_cosynthesis_input_delete(wtk_cosynthesis_lexicon_input_t *input, int ninput)
{
	int i,j;

    for(i=0;i<ninput;++i)
    {
        if(input[i].finfo)
        {
            if(input[i].finfo->left_phone) wtk_string_delete(input[i].finfo->left_phone);
            if(input[i].finfo->left_type) wtk_string_delete(input[i].finfo->left_type);
            if(input[i].finfo->right_phone) wtk_string_delete(input[i].finfo->right_phone);
            if(input[i].finfo->right_type) wtk_string_delete(input[i].finfo->right_type);
            if(input[i].finfo->left_phone_nosil) wtk_string_delete(input[i].finfo->left_phone_nosil);
            if(input[i].finfo->right_phone_nosil) wtk_string_delete(input[i].finfo->right_phone_nosil);
            if(input[i].finfo->sil_flabel) wtk_string_delete(input[i].finfo->sil_flabel);
            if(input[i].finfo->nphone>0)
            {
                for(j=0;j<input[i].finfo->nphone;++j)
                {
                    wtk_string_delete(input[i].finfo->flabel[j]);
                }
                wtk_free(input[i].finfo->flabel);
            }
            wtk_free(input[i].finfo);
        }
    }
    wtk_free(input);
}

void wtk_cosynthesis_lexicon_delete(wtk_cosynthesis_lexicon_t *lex)
{
    wtk_free(lex->ed);
    if (lex->input)
    	wtk_cosynthesis_input_delete(lex->input, lex->ninput);
    if(lex->min_path)
    {
        wtk_free(lex->min_path);
    }
    if (lex->min_path_map)
    {
    	wtk_free(lex->min_path_map);
    }
#ifdef WTK_ED_ALGORITHM
    wtk_cosynthesis_lexicon_ed_path_clean(lex);
#endif
    wtk_heap_delete(lex->heap);
    wtk_queue_delete(lex->ed_pathq);
    wtk_vpool2_delete(lex->ed_path_pool);
    wtk_free(lex->wrds);
    if(lex->g_audio_res_fp)
    	fclose(lex->g_audio_res_fp);
    if(lex->g_feat_res_fp)
    	fclose(lex->g_feat_res_fp);

    wtk_free(lex);
}

void wtk_cosynthesis_input_reset(wtk_cosynthesis_lexicon_input_t *input, int ninput)
{
	int i, j;

    for(i=0;i<ninput;++i)
    {
        // wtk_debug("%d\n",i);
        if(input[i].finfo)
        {
            if(input[i].finfo->left_phone)
            	wtk_string_delete(input[i].finfo->left_phone);
            if(input[i].finfo->left_type)
            	wtk_string_delete(input[i].finfo->left_type);
            if(input[i].finfo->right_phone)
                wtk_string_delete(input[i].finfo->right_phone);
            if(input[i].finfo->right_type)
            	wtk_string_delete(input[i].finfo->right_type);
            if(input[i].finfo->left_phone_nosil)
            	wtk_string_delete(input[i].finfo->left_phone_nosil);
            if(input[i].finfo->right_phone_nosil)
            	wtk_string_delete(input[i].finfo->right_phone_nosil);

            if(input[i].finfo->sil_flabel)
            {
                wtk_string_delete(input[i].finfo->sil_flabel);
                if(input[i].sil_conca_cost)
                {
                    wtk_free(input[i].sil_conca_cost[0]);
                    wtk_free(input[i].sil_conca_cost);
                }
            }
            if(input[i].finfo->nphone>0)
            {
                for(j=0;j<input[i].finfo->nphone;++j)
                {
                    wtk_string_delete(input[i].finfo->flabel[j]);
                }
                wtk_free(input[i].finfo->flabel);
            }
            wtk_free(input[i].finfo);
            input[i].finfo = NULL;
        }
        if(input[i].select_cost)
        {
            wtk_free(input[i].select_cost);
        }
        if(input[i].conca_cost)
        {
            for(j=0;j<input[i].nunit;++j)
            {
                wtk_free(input[i].conca_cost[j]);
            }
            wtk_free(input[i].conca_cost);
        }
        input[i].sil_tot_dur=0;
        if(input[i].unit_id)
        {
            //wtk_free(input[i].unit_id);
        	input[i].unit_id=NULL;
        	input[i].nunit=0;
        }
    }
    wtk_free(input);
}

void wtk_cosynthesis_lexicon_reset(wtk_cosynthesis_lexicon_t *lex)
{
	if (lex->cfg->use_fp)
		wtk_cosynthesis_lexicon_cleaninput(lex);
	if(lex->input)
		wtk_cosynthesis_input_reset(lex->input, lex->ninput);
    if(lex->min_path)
    {
        wtk_free(lex->min_path);
    }
    if(lex->min_path_map)
    {
        wtk_free(lex->min_path_map);
    }

    wtk_cosynthesis_lexicon_init(lex);
}

void wtk_cosynthesis_lexicon_input_new(wtk_cosynthesis_lexicon_t *lex,uint16_t *input,int nwrd)
{
    int i;

    lex->input = (wtk_cosynthesis_lexicon_input_t*)wtk_malloc(sizeof(wtk_cosynthesis_lexicon_input_t)*nwrd);
    for(i=0;i<nwrd;++i)
    {
        lex->input[i].unit_id = NULL;
        lex->input[i].word_id = input[i];
        lex->input[i].nunit = 0;
        lex->input[i].need_process = NOTHING;
        //wtk_debug("%d\n",lex->input[i].word_id);
        lex->input[i].finfo = (wtk_flabel_feats_t*)wtk_malloc(sizeof(wtk_flabel_feats_t));
        // wtk_debug("new %p\n",lex->input[i].finfo);
        lex->input[i].finfo->left_phone=NULL;
        lex->input[i].finfo->left_type=NULL;
        lex->input[i].finfo->right_phone=NULL;
        lex->input[i].finfo->right_type=NULL;
        lex->input[i].finfo->left_phone_nosil=NULL;
        lex->input[i].finfo->right_phone_nosil=NULL;
        lex->input[i].finfo->flabel = NULL;
        lex->input[i].finfo->nphone = 0;
        lex->input[i].finfo->sil_flabel = NULL;
        wtk_queue_init(&lex->input[i].wrd_q);
        lex->input[i].select_cost = NULL;
        lex->input[i].conca_cost = NULL;
        lex->input[i].sil_conca_cost = NULL;
        lex->input[i].sil_tot_dur=0;
    }
}



int wtk_cosynthesis_lexicon_get_wrd_id(wtk_cosynthesis_lexicon_t *lex, char* data,int len)
{
    int nwrd = lex->cfg->nwrds;
    char *word;
    int i;
    int ret = -1;
    // wtk_debug("======== >%d %.*s\n",len,len,data);
    for (i=0;i<nwrd;++i)
    {
        word = lex->cfg->wrds[i].word;
        if(len == lex->cfg->wrds[i].word_len)
        {
            if(strncmp(word,data,len)==0)
            {
                ret = i;
                break;
            }
        }
    }
    return ret;

}

int get_word_index(wtk_cosynthesis_lexicon_t *lex,uint16_t word_id)
{
    int idx=-1;
    int i;
    for(i=0;i<lex->cfg->nwrds;++i)
    {
        if(lex->cfg->wrds[i].word_id == word_id)
        {
            idx = i;
            break;
        }
    }
    return idx;
}

static const char* num_map[]=
{
    "zero",
    "one",
    "two",
    "three",
    "four",
    "five",
    "six",
    "seven",
    "eight",
    "nine",
    "ten",
    "eleven",
    "twelve",
    "thirteen",
    "fourteen",
    "fifteen",
    "sixteen",
    "seventeen",
    "eighteen",
    "ninteen",
};

void process_num_wrd(wtk_strbuf_t *buf)
{
    int num, len;
    char *s;

    len=buf->pos;
    while(len--)
    {
    	if(0==isdigit(buf->data[len]))
    		return;
    }
    len = buf->pos+1;
    s = (char*)wtk_malloc(len);
    memcpy(s,buf->data,buf->pos);
    s[len-1] = '\0';
    num = atoi(s);
    if(num>0 && num<20)
    {
        wtk_strbuf_reset(buf);
        wtk_strbuf_push(buf,num_map[num],strlen(num_map[num]));
    }
    wtk_free(s);
}


void wtk_cosynthesis_lexicon_mapping(wtk_strbuf_t *buf)
{
    static char *e[] = {
        "kilometres",
        "neighbour",
        "favourite",
        "colour",
//        "programme",
//        "guilin",
    };
    static char *e_mapping[] = {
        "kilometers",
        "neighbor",
        "favorite",
        "color",
//        "programmer",
//        "guilt"
    };
    int len = sizeof(e)/sizeof(char*);
    int i = 0;
    for(i = 0; i < len; ++i){
        if(strlen(e[i]) == buf->pos && !strncmp(buf->data,e[i],buf->pos)){
            wtk_strbuf_reset(buf);
            wtk_strbuf_push(buf,e_mapping[i],strlen(e_mapping[i]));
            break;
        }
    }
    return;
}

int wtk_cosynthesis_lexicon_txtparser(wtk_cosynthesis_lexicon_t *lex, char* data,int len)
{
	wtk_strbuf_t *buf;
	int id, nwrd=0;
	uint16_t* input;

	input = (uint16_t*) wtk_calloc(lex->cfg->maxn_wrds, sizeof(*input));
	buf = wtk_strbuf_new(64,1);
    //wtk_debug("reftext=[%.*s]\n", len,data);
	len++;   //include end symbol '\0'
    while(len--)
    {
        if(isspace(*data) || *data=='\0')
        {
            if(buf->pos > 0)
            {
            	//digit convert
                process_num_wrd(buf);
                //words map
                wtk_cosynthesis_lexicon_mapping(buf);
                //wtk_debug("wrd=[%.*s]\n", buf->pos,buf->data);
                id = wtk_cosynthesis_lexicon_get_wrd_id(lex,buf->data,buf->pos);
                //wtk_debug("id=%d\n", id);
                if(id>=0)
                {
                    if(nwrd >= lex->cfg->maxn_wrds)
                    {
                    	wtk_cosynthesis_output_set(lex->output, QTK_COSYN_STATUS_ERR_MAXNWRD, 0,0,0);
                        //wtk_debug("word cnt[%d] exceeded max[%d]\n",nwrd, lex->cfg->maxn_wrds);
                    	nwrd=0;
                        goto end;
                    }
                    input[nwrd] = id;
                    lex->word_seq[nwrd] = wtk_heap_dup_str2(lex->heap,buf->data,buf->pos);
                    wtk_strbuf_reset(buf);
                    nwrd++;
                }else
                {
                	wtk_cosynthesis_output_set(lex->output, QTK_COSYN_STATUS_ERR_OUTDICT, 0, buf->data, buf->pos);
                    //wtk_debug("word[%.*s] is out of lexicon\n",buf->pos,buf->data);
                	nwrd=0;
                    goto end;
                }
            }
        }else
        {
            wtk_strbuf_push_c(buf,*data);
        }
        data++;
    }
    if(nwrd>0)
    {
        wtk_cosynthesis_lexicon_input_new(lex,input,nwrd);
        lex->ninput = nwrd;
    }

end:
	if(input)
		wtk_free(input);
	if(buf)
		wtk_strbuf_delete(buf);

    return nwrd;
}

#ifdef WTK_ED_ALGORITHM
int word_cmp(uint16_t *words,wtk_cosynthesis_lexicon_input_t *input,int len)
{
    int i;
    int ret = 0;
    for(i=0;i<len;++i)
    {
        if(words[i]==input[i].word_id)
        {
            continue;
        }else if(words[i] > input[i].word_id)
        {
            ret = 1;
            break;
        }else if(words[i] < input[i].word_id)
        {
            ret =-1;
            break;
        }
    }
    return ret;
}

int wtk_cosynthesis_lexicon_seek_snt(wtk_cosynthesis_lexicon_t *lex,int nwrds)
{
    wtk_snt_t *snts = lex->cfg->snts;
    // int i;
    int mid,start=0;
    int end =lex->cfg->nsnts;
    int snt_index=-1;
    //wtk_debug("nwrds=%d\n", nwrds);
    while(start<end)
    {
        // wtk_debug("start=%d end=%d\n",start,end);
        mid =(start + end) / 2;
        if(snts[mid].nosil_word_cnt == nwrds)
        {
            // wtk_debug("nwrds=%d\n",nwrds);
            // print_short(input,nwrds);
            // print_short(snts[mid].nosil_word_ids,nwrds);
            // wtk_debug("mid=%d\n",mid);
            if(word_cmp(snts[mid].nosil_word_ids,lex->input,nwrds)==0)
            {
                snt_index = mid;
                break;
            }else if(word_cmp(snts[mid].nosil_word_ids,lex->input,nwrds) < 0)
            {
                start = mid + 1;
            }else if(word_cmp(snts[mid].nosil_word_ids,lex->input,nwrds) > 0)
            {
                end = mid;
            }
        }else if(snts[mid].nosil_word_cnt < nwrds)
        {
            start = mid + 1;
        }else if(snts[mid].nosil_word_cnt> nwrds)
        {
            end = mid;
        }
    }

    return snt_index;
}

int wtk_cosynthesis_lexicon_get_right_board(wtk_cosynthesis_lexicon_cfg_t *cfg,int n)
{
    int mid;
    int left = 0;
    int right = cfg->nsnts;

    while(left < right)
    {
        mid=(left+right)/2;
        if(cfg->snts[mid].nosil_word_cnt == n)
        {
            left=mid+1;
        }else if(cfg->snts[mid].nosil_word_cnt<n)
        {
            left=mid+1;
        }else if(cfg->snts[mid].nosil_word_cnt>n)
        {
            right=mid;
        }
    }
    return left-1;
}

int wtk_cosynthesis_lexicon_get_left_board(wtk_cosynthesis_lexicon_cfg_t *cfg,int n)
{
    int mid;
    int left = 0;
    int right = cfg->nsnts;

    while(left<right)
    {
        mid = (left+right)/2;
        if(cfg->snts[mid].nosil_word_cnt == n)
        {
            right=mid;
        }else if(cfg->snts[mid].nosil_word_cnt<n)
        {
            left=mid+1;
        }else if(cfg->snts[mid].nosil_word_cnt>n)
        {
            right=mid;
        }
    }
    return left;
}

wtk_ed_node_t get_min_ed_node(wtk_ed_node_t add_node, wtk_ed_node_t del_node,wtk_ed_node_t sub_node)
{
    wtk_ed_node_t min_node;
    if(add_node.val > del_node.val)
    {
        min_node.val = del_node.val;
        min_node.cmd = WTK_ED_DEL;
    }else
    {
        min_node.val = add_node.val;
        min_node.cmd = WTK_ED_ADD;
    }
    if(min_node.val > sub_node.val)
    {
        min_node.val = sub_node.val;
        min_node.cmd = WTK_ED_SUB;
    }
    return min_node;
}

int wtk_cosynthesis_lexicon_ed_path_clean(wtk_cosynthesis_lexicon_t *lex)
{
    wtk_ed_path_t *ed_path;
    wtk_queue_node_t *qn;
	while (1)
	{
		qn=wtk_queue_pop(lex->ed_pathq);
		if(!qn){break;}
        ed_path = data_offset(qn,wtk_ed_path_t,q_n);
        wtk_vpool2_push(lex->ed_path_pool,ed_path);
    }
    return 0;
}

wtk_ed_path_t *wtk_cosynthesis_lexicon_ed_path_new(wtk_cosynthesis_lexicon_t *lex,int ed,int id)
{
    wtk_ed_path_t *ed_path;
    ed_path = (wtk_ed_path_t*)wtk_vpool2_pop(lex->ed_path_pool);
    ed_path->path = NULL;
    ed_path->ed = ed;
    ed_path->snt_id = id;
    ed_path->path_len = 0;
    wtk_queue_push(lex->ed_pathq,&(ed_path->q_n));
    return ed_path;
}


void wtk_cosynthesis_lexicon_process_min_ed_path(wtk_cosynthesis_lexicon_t* lex,wtk_ed_node_t **node,int m,int n,int min_ed,int snt_id,int need_reset)
{
    wtk_ed_cmt_t cmd;
    int len = max(m,n);
    wtk_larray_t *a = wtk_larray_new(len,sizeof(uint8_t));
    wtk_ed_path_t *ed_path;
    uint8_t t;
    // int i;

    if(need_reset)
    {
        wtk_cosynthesis_lexicon_ed_path_clean(lex);
        ed_path = wtk_cosynthesis_lexicon_ed_path_new(lex,min_ed,snt_id);
    }else
    {
        ed_path = wtk_cosynthesis_lexicon_ed_path_new(lex,min_ed,snt_id);
    }
    while(m!=0 && n!=0)
    {
        cmd = node[m][n].cmd;
        // wtk_debug("m=%d n=%d cmd=%d,val=%d\n",m,n,cmd,node[m][n].val);
        switch(cmd)
        {
            case WTK_ED_SKIP:
                m--;
                n--;
                t = cmd;
                wtk_larray_push2(a,&t);
                break;
            case WTK_ED_ADD:
                n--;
                t = cmd;
                wtk_larray_push2(a,&t);
                break;
            case WTK_ED_DEL:
                m--;
                t = cmd;
                wtk_larray_push2(a,&t);
                break;
            case WTK_ED_SUB:
                m--;
                n--;
                t = cmd;
                wtk_larray_push2(a,&t);
                break;
            default:
                break;
        }
    }
    while(m>0)
    {
        m--;
        t = WTK_ED_DEL;
        wtk_larray_push2(a,&t);
    }
    while(n>0)
    {
        n--;
        t = WTK_ED_ADD;
        wtk_larray_push2(a,&t);
    }
    ed_path->path = wtk_heap_malloc(lex->heap,sizeof(uint8_t)*a->nslot);
    ed_path->path_len = a->nslot;
    memcpy(ed_path->path,a->slot,sizeof(uint8_t)*a->nslot);//倒序存储
    wtk_larray_delete(a);
}

int wtk_cosynthesis_lexicon_cal_edit_distance(wtk_cosynthesis_lexicon_t* lex,int i,int nin)
{
    int j,k;
    wtk_ed_node_t **node = lex->ed->node;
    wtk_ed_node_t min_node;
    wtk_snt_t snt = lex->cfg->snts[i];
    int n = nin;
    int m = snt.nosil_word_cnt;
    // wtk_debug("snt = %d m=%d n=%d\n",snt.snt_id,m,n);
    uint8_t min_ed;
    node = (wtk_ed_node_t**)wtk_malloc(sizeof(wtk_ed_node_t*)*(m+1));
    // print_short(input,nin);
    // print_short(snt.word_ids,n);
    for(j=0;j<m+1;++j)
    {
        node[j] = (wtk_ed_node_t*)wtk_malloc(sizeof(wtk_ed_node_t)*(n+1));
    }

    for(j=0;j<m+1;++j)
    {
        node[j][0].val = j;
        node[j][0].cmd = WTK_ED_DEL;
    }
    for(j=1;j<n+1;++j)
    {
        node[0][j].val = j;
        node[0][j].cmd = WTK_ED_ADD;
    }
    for(j=1;j<m+1;++j)
    {
        for(k=1;k<n+1;++k)
        {
            // wtk_debug("in=%d word=%d\n",input[j-1],snt.word_ids[k-1]);
            if(lex->input[k-1].word_id==snt.nosil_word_ids[j-1])
            {
                node[j][k].val = node[j-1][k-1].val;
                node[j][k].cmd = WTK_ED_SKIP;
                // wtk_debug("cmd=%d\n",node[j][k].cmd);
            }else
            {
                min_node = get_min_ed_node(node[j][k-1],node[j-1][k],node[j-1][k-1]);
                node[j][k].val = min_node.val + 1;
                node[j][k].cmd = min_node.cmd;
                // wtk_debug("cmd=%d\n",node[j][k].cmd);
            }
        }
    }
    min_ed = node[m][n].val;
    
    if(min_ed > lex->cfg->max_edit_distance)
    {
        goto end;
    }else if(min_ed < lex->min_edit_distance)
    {
        lex->min_edit_distance = min_ed;
        // wtk_debug("min_ed=%d\n",min_ed);
        wtk_cosynthesis_lexicon_process_min_ed_path(lex,node,m,n,min_ed,i,1);
    }else if(min_ed == lex->min_edit_distance)
    {
        wtk_cosynthesis_lexicon_process_min_ed_path(lex,node,m,n,min_ed,i,0);
    }
end:
    for(j=0;j<m+1;++j)
    {
        wtk_free(node[j]);
    }
    wtk_free(node);
    return 0;
}

int wtk_cosynthesis_lexicon_set_min_ed_path(wtk_cosynthesis_lexicon_t *lex)
{
    int ret = 0;
    wtk_ed_path_t *ed_path;
    wtk_queue_node_t *qn;
    int i, ni, mi;
    uint8_t it;
    if(lex->ed_pathq->length>0)
    {
        for(qn = lex->ed_pathq->pop; qn;)
        {
            ed_path = data_offset(qn,wtk_ed_path_t,q_n);
            if(ed_path->ed == lex->min_edit_distance)
            {
                break;
            }
        }
        lex->min_path_len = ed_path->path_len;
        lex->match_snt_idx = ed_path->snt_id;
        lex->min_path = (uint8_t*)wtk_malloc(sizeof(uint8_t)*lex->min_path_len);
        lex->min_path_map = (int*)wtk_malloc(sizeof(int)*lex->min_path_len);
        for(i=0; i<lex->min_path_len; i++)
        	lex->min_path_map[i]=-1;
        //wtk_debug("min_path_len=%d\n", lex->min_path_len);
        //wtk_debug("match_snt_idx=%d\n", lex->match_snt_idx);
        ni=0;  //word index of input snt
        mi=0;  //word index of match snt
        for(i=0;i<lex->min_path_len;++i)
        {
        	it = ed_path->path[ed_path->path_len-1-i];
            lex->min_path[i] = it;
            //set min_path_map
        	if (mi < lex->cfg->snts[lex->match_snt_idx].raw_word_cnt && lex->cfg->snts[lex->match_snt_idx].raw_word_ids[mi]==lex->cfg->sil_id)
        		mi++;
            switch(it){
            case  WTK_ED_SKIP:
            	lex->min_path_map[ni]=mi;
            	ni++;
            	mi++;
            	break;
            case WTK_ED_ADD:
            	ni++;
            	break;
            case WTK_ED_DEL:
            	mi++;
            	break;
            case WTK_ED_SUB:
            	ni++;
            	mi++;
            	break;
            default:
            	break;
            }
             //printf("%d ",lex->min_path[i]);
        }
        // printf("\n");
        wtk_cosynthesis_lexicon_ed_path_clean(lex);
    }else
    {
        ret = -1;
        goto end;
    }

end:
    return ret;
}

int wtk_cosynthesis_lexicon_check_edit_distance(wtk_cosynthesis_lexicon_t *lex,int nin)
{
    // int min_len,max_len;
    int left,right;
    // uint16_t *raw_snt;
    int ret = -1;
    int i;
    if(lex->cfg->snt_wrdlen_split_idx && lex->cfg->use_snt_wrdlen_split_idx)
    {
        left = lex->cfg->snt_wrdlen_split_idx[max(1,nin-lex->cfg->max_edit_distance)-1];
        right = lex->cfg->snt_wrdlen_split_idx[min(lex->cfg->nsnt_wrdlen,nin+lex->cfg->max_edit_distance)]-1;
    }else
    {
        left = wtk_cosynthesis_lexicon_get_left_board(lex->cfg,max(1,nin-lex->cfg->max_edit_distance));
        right = wtk_cosynthesis_lexicon_get_right_board(lex->cfg,min(lex->cfg->snts[lex->cfg->nsnts-1].nosil_word_cnt,nin+lex->cfg->max_edit_distance));
    }
    for(i=left;i<right;++i)
    {
        wtk_cosynthesis_lexicon_cal_edit_distance(lex,i,nin);
        if(lex->min_edit_distance == 1)
        {
            break;
        }
    }
    ret = wtk_cosynthesis_lexicon_set_min_ed_path(lex);
    return ret;
}

int wtk_cosynthesis_lexicon_process(wtk_cosynthesis_lexicon_t *lex, char* data,int len)
{
    int ret=WTK_COSYN_NOTHING;
    wtk_strbuf_t *buf;
    buf = wtk_strbuf_new(64,1);
    uint16_t input[50];
    int id,i,nwrd=0;
    int snt_idx;
    i = 0;

    //wtk_debug("reftext=[%.*s]\n", len,data);
    while(len+1)
    {
        if(isspace(data[i])||data[i]=='\0')
        {
            if(buf->pos>0)
            {
                process_num_wrd(buf);
                wtk_cosynthesis_lexicon_mapping(buf);
                //wtk_debug("wrd=[%.*s]\n", buf->pos,buf->data);
                id = wtk_cosynthesis_lexicon_get_wrd_id(lex,buf->data,buf->pos);
                //wtk_debug("id=%d\n", id);
                if(id>=0)
                {
                    input[nwrd] = id;
                    lex->word_seq[nwrd] = wtk_heap_dup_str2(lex->heap,buf->data,buf->pos);
                    // wtk_debug("id=%d\n",id);
                    wtk_strbuf_reset(buf);
                    nwrd++;
                    if(nwrd > lex->cfg->maxn_wrds)
                    {
                    	wtk_cosynthesis_output_set(lex->output, QTK_COSYN_STATUS_ERR_MAXNWRD, 0,0,0);
                        wtk_debug("word cnt[%d] exceeded max[%d]\n",nwrd, lex->cfg->maxn_wrds);
                        ret=WTK_COSYN_ERR;
                        goto end;
                    }
                }else
                {
                	wtk_cosynthesis_output_set(lex->output, QTK_COSYN_STATUS_ERR_OUTDICT, 0, buf->data, buf->pos);
                    wtk_debug("word[%.*s] is out of lexicon\n",buf->pos,buf->data);
                    ret = WTK_COSYN_OUT_DICT;
                    goto end;
                }
            }
        }else
        {   
            wtk_strbuf_push(buf,&data[i],1);
        }
        len--;
        i++;
    }
    if(nwrd>0)
    {
        wtk_cosynthesis_lexicon_input_new(lex,input,nwrd);
        lex->ninput = nwrd;

        //load input unit info and feat info
        if (lex->cfg->use_fp)
        	wtk_cosynthesis_lexicon_loadinput(lex);
    }

    if(nwrd>0)
    {
        if((snt_idx=wtk_cosynthesis_lexicon_seek_snt(lex,nwrd))>0)
        {
            lex->match_snt_idx = snt_idx;
            ret = WTK_COSYN_MATCH_SNT;
        }else
        {
            ret = wtk_cosynthesis_lexicon_check_edit_distance(lex,nwrd);
            if(ret < 0)
            {
                ret = WTK_COSYN_PROCESS_ED_OUT;
            }else
            {
                ret = WTK_COSYN_PROCESS_ED_IN;
            }
        }
    }

end:
    wtk_strbuf_delete(buf);
    return ret;
}
#endif
int wtk_cosynthesis_lexicon_loadwrdinfo(wtk_cosynthesis_lexicon_t *lex, int wrd_id)
{
	wtk_cosynthesis_lexicon_cfg_t *cfg=lex->cfg;
	wtk_heap_t *h=lex->heap;
    int j,l;
    wtk_word_t *wrd;
    FILE *audio_fp, *feat_fp;
    uint16_t u16;
	uint8_t u8;

    audio_fp=lex->g_audio_res_fp;
    feat_fp=lex->g_feat_res_fp;
    wrd=&(lex->wrds[wrd_id]);
    if (wrd->unit)
    {
    	return 0;
    }

    //load word unit info and feat info.
    wrd->unit = (wtk_unit_t*)wtk_heap_zalloc(h, wrd->nunit * sizeof(wtk_unit_t));
    fseek(audio_fp,wrd->unit_pos,SEEK_SET);
    fseek(feat_fp,wrd->feat_pos,SEEK_SET);
    //wtk_debug("wrd_id=%d nunit=%d\n", wrd_id,wrd->nunit);
    for(j=0;j < wrd->nunit;++j)
    {
    	wrd->unit[j].unit_id = j;
        fread(&u16, 1, sizeof(uint16_t), audio_fp);
        wrd->unit[j].data_len=u16;
        fread(&u16, 1, sizeof(uint16_t), audio_fp);
        wrd->unit[j].raw_audio_len=u16;
        fread(&u8, 1, sizeof(uint8_t), audio_fp);
        wrd->unit[j].is_compress=u8;
        fread(&u16, 1, sizeof(uint16_t), audio_fp);
        wrd->unit[j].shifit=u16;
        wrd->unit[j].audio_pos = ftell(audio_fp);
        fseek(audio_fp,wrd->unit[j].data_len,SEEK_CUR);
        if(strncmp(wrd->word,"pau",3)==0)
        {
            continue;
        }
        fread(&(wrd->unit[j].nphone), 1, sizeof(int), feat_fp);
        fread(&(wrd->unit[j].sil_prev_l), 1, sizeof(float), feat_fp);
        //wtk_debug("nphone=%d sil_prev_l=%f\n", wrd->unit[j].nphone, wrd->unit[j].sil_prev_l);
        l=wrd->unit[j].nphone * cfg->feat_cfg.spec_len;
        wrd->unit[j].spec = (float*)wtk_heap_zalloc(h,l * sizeof(float));
        fread(wrd->unit[j].spec, l, sizeof(float), feat_fp);
        //print_float(wrd->unit[j].spec, l);

        l=wrd->unit[j].nphone * cfg->feat_cfg.lf0_len;
        wrd->unit[j].lf0 = (float*)wtk_heap_zalloc(h, l * sizeof(float));
        fread(wrd->unit[j].lf0, l, sizeof(float), feat_fp);

        l=wrd->unit[j].nphone * cfg->feat_cfg.dur_len;
        wrd->unit[j].dur = (float*)wtk_heap_zalloc(h, l * sizeof(float));
        fread(wrd->unit[j].dur, l, sizeof(float), feat_fp);
        //print_float(wrd->unit[j].dur, l);

        l=wrd->unit[j].nphone;
        wrd->unit[j].kld_lf0 = (float*)wtk_heap_zalloc(h, l * sizeof(float));
        fread(wrd->unit[j].kld_lf0, l, sizeof(float), feat_fp);
        //print_float(wrd->unit[j].kld_lf0, l);

        l=cfg->feat_cfg.spec_llen;
        wrd->unit[j].spec_r = (float*)wtk_heap_zalloc(h, l * sizeof(float));
        fread(wrd->unit[j].spec_r, l, sizeof(float), feat_fp);
        //print_float(wrd->unit[j].spec_r, l);
        wrd->unit[j].spec_l = (float*)wtk_heap_zalloc(h, l * sizeof(float));
        fread(wrd->unit[j].spec_l, l, sizeof(float), feat_fp);
        //print_float(wrd->unit[j].spec_l, l);

        l=wrd->unit[j].nphone;
        wrd->unit[j].spec_idx = (int*)wtk_heap_zalloc(h, l * sizeof(int));
        fread(wrd->unit[j].spec_idx, l, sizeof(int), feat_fp);
        //print_int(wrd->unit[j].spec_idx, l);

        l=wrd->unit[j].nphone;
        wrd->unit[j].lf0_idx = (int*)wtk_heap_zalloc(h, l * sizeof(int));
        fread(wrd->unit[j].lf0_idx, l, sizeof(int), feat_fp);
        //print_int(wrd->unit[j].lf0_idx, l);

        l=wrd->unit[j].nphone;
        wrd->unit[j].dur_idx = (int*)wtk_heap_zalloc(h, l * sizeof(int));
        fread(wrd->unit[j].dur_idx, l, sizeof(int), feat_fp);
        //print_int(wrd->unit[j].dur_idx, l);
    }

    return 0;
}

int wtk_compress_decoder(char *data, int data_len, short *wav_data,int *len)
{
#if 0
    hip_t gfp = NULL;
    int ret = 0;

    gfp = hip_decode_init();
    ret = hip_decode(gfp,(unsigned char*)data,data_len,(short*)wav_data,NULL);
    if(ret < 0){
        printf("decode error create mp3 bin error\n");
        ret=-1; goto end;
    }else if(ret == 0){
        ret = hip_decode(gfp,(unsigned char *)(data),0,(short*)wav_data,NULL);
    }
    *len = ret;

end:
	hip_decode_exit(gfp);
    return ret;
#else
    return -1;
#endif
}

short* wtk_cosynthesis_lexicon_getaudio(wtk_cosynthesis_lexicon_t* lex, wtk_unit_t *unit, int *len)
{
	char *data;
	short *audio=NULL;
	short *audio_p;
	int audio_len=0;
	int ret=0;
	FILE *f;

	f=lex->g_audio_res_fp;
	fseek(f, unit->audio_pos, SEEK_SET);
	data = (char*)wtk_heap_zalloc(lex->heap, unit->data_len);
	ret=fread(data, 1, unit->data_len, f);
	if (ret<=0)
	{
		wtk_debug("read failed\n");
		goto end;
	}

    if(unit->is_compress){  //compress
        audio_p = (short*)wtk_heap_zalloc(lex->heap, unit->raw_audio_len * 10);   //enough big
        ret=wtk_compress_decoder(data,unit->data_len,audio_p,&audio_len);
        if (ret<=0) {
        	wtk_cosynthesis_output_set(lex->output, QTK_COSYN_STATUS_ERR_MAXNWRD, 0,0,0);
        	goto end;
        }
        if((audio_len-unit->shifit) >= unit->raw_audio_len){
            audio_len = unit->raw_audio_len;
        }else{
            audio_len = audio_len-unit->shifit;
        }
        audio = audio_p + unit->shifit;
    }else{
        audio = (short*)data;
    }
    *len = audio_len;
end:
	return audio;
}

/**
 * load all needed words unit info and feat info(not include unit audio info).
 */
void wtk_cosynthesis_lexicon_loadinput(wtk_cosynthesis_lexicon_t *lex)
{
	int i;

	for (i=0; i < lex->ninput; i++)
	{
		wtk_cosynthesis_lexicon_loadwrdinfo(lex, lex->input[i].word_id);
	}
}

/**
 * reset all needed words unit info and feat info.
 */
void wtk_cosynthesis_lexicon_cleaninput(wtk_cosynthesis_lexicon_t *lex)
{
	wtk_cosynthesis_lexicon_cfg_t *cfg=lex->cfg;
    int i;

	for (i=0; i < lex->ninput; i++)
	{
		cfg->wrds[lex->input[i].word_id].unit=NULL;
	}

	cfg->wrds[lex->cfg->sil_id].unit=NULL;
}
