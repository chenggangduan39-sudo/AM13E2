#include "qtk_tts_parse.h"
#include "tts-mer/wtk-extend/wtk_heap2.h"

int qtk_tts_parse_id_print(qtk_tts_parse_t *parse);

qtk_tts_parse_t *qtk_tts_parse_new(qtk_tts_parse_cfg_t *cfg)
{
    qtk_tts_parse_t *parse = NULL;
    int ret=0;
    
    parse = wtk_malloc(sizeof(*parse));
    memset(parse,0,sizeof(qtk_tts_parse_t));
    parse->cfg = cfg;
    
    parse->norm = wtk_tts_norm_new(&cfg->norm,cfg->rbin);
    parse->segsnt = wtk_malloc(sizeof(*parse->segsnt));
    wtk_tts_segsnt_init(parse->segsnt,&cfg->segsnt);
    if(cfg->is_en || cfg->use_segmax){
        parse->segwrd = wtk_tts_segwrd_new(&cfg->segwrd,NULL);
    }else{
        parse->segwrd2 = qtk_tts_segwrd_new(&cfg->segwrd2, cfg->rbin);
        if (NULL==parse->segwrd2)
        {
        	wtk_debug("segwrd2 is null\n");
        	ret=-1;
        	goto end;
        }
    }
    parse->symbols = qtk_tts_symbols_id_new(&cfg->symbols);
    parse->prosody = qtk_tts_prosody_new(&cfg->prosody, cfg->rbin);
    parse->ply = qtk_tts_polyphone_new(&cfg->ply, cfg->rbin);

    parse->heap = wtk_heap_new(1024);
    parse->buf = wtk_strbuf_new(1024,1.2f);
    parse->nid = 0;

    parse->lab = NULL;
end:
	if (ret!=0)
	{
		qtk_tts_parse_delete(parse);
		parse=NULL;
	}
    return parse;
}

int qtk_tts_parse_delete(qtk_tts_parse_t *parse)
{
    int ret = 0;
    if(parse == NULL) goto end;
    if(parse->norm)
        wtk_tts_norm_delete(parse->norm);
    if(parse->segsnt){
        wtk_tts_segsnt_clean(parse->segsnt);
        wtk_free(parse->segsnt);
    }
    if(parse->segwrd)
        wtk_tts_segwrd_delete(parse->segwrd);
    if(parse->symbols)
        qtk_tts_symbols_id_delete(parse->symbols);
    if(parse->prosody)
        qtk_tts_prosody_delete(parse->prosody);
    if(parse->ply)
        qtk_tts_polyphone_delete(parse->ply);
    if(parse->segwrd2)
        qtk_tts_segwrd_delete(parse->segwrd2);

    if(parse->heap)
        wtk_heap_delete(parse->heap);
    if(parse->buf)
        wtk_strbuf_delete(parse->buf);

    wtk_free(parse);
end:
    return ret;
}

int qtk_tts_parse_en_lab2id(qtk_tts_parse_t *parse)
{
    int ret = -1,len = 0;
    wtk_veci_t *vec,**vecs = NULL;
    wtk_tts_lab_t  *lab = NULL;
    wtk_tts_snt_t **snts = NULL;
    wtk_tts_wrd_t **wrds = NULL,*wrd=NULL;
    wtk_tts_syl_t *s = NULL;
    int i = 0, j = 0,k = 0,n = 0,u = 0;
    wtk_heap_t *heap = parse->heap;
    int *tmp = NULL;

    lab = parse->lab;
    snts = lab->snts->slot;
    j = lab->snts->nslot;
    vecs = wtk_heap_malloc(heap,sizeof(wtk_veci_t*)*j);
    parse->nid = j;
    tmp = wtk_malloc(sizeof(int)*1024);

    for(i = 0; i < j; ++i){
        wrds = snts[i]->wrds->slot;
        n = snts[i]->wrds->nslot;
        len = 0;
        for(k = 0; k < n; ++k){
            wrd = wrds[k];
            for(u=0,s=wrd->pron->pron->syls;u < wrd->pron->pron->nsyl;++u,++s)
            {
                // printf("v[%d]=%.*s:%d\n",i,s->v->len,s->v->data,s->tone);
                tmp[len] =  qtk_tts_symbols_id_get_id(parse->symbols,s->v->data,s->v->len);
                if(tmp[len] < 0){
                    printf("err sumbols %.*s:%d\n",s->v->len,s->v->data,s->tone);
                    goto end;
                }
                len++;
            }
        }
        vec = wtk_veci_heap_new(heap,len);
        memcpy(vec->p,tmp,len*sizeof(int));
        vecs[i] = vec;
    }
    parse->id_vec = vecs;
    ret = 0;
end:
    wtk_free(tmp);
    return ret;
}

//remove er hua
void qtk_tts_parse_chn_erhua(char *data,int *len)
{
    static char *voice_erhua[]={"uer","uir","iur","unr"};
    int n = sizeof(voice_erhua)/sizeof(char*);
    int i = 0,j = 0;
    int ulen = 0;
    for(i = 0; i < n; ++i){
        for(j = 0; j < *len;++j){
            ulen = min(strlen(voice_erhua[i]),*len-j);
            if(ulen == strlen(voice_erhua[i]) && 0 == strncmp(data+j,voice_erhua[i],ulen)){
                if(data[*len-1] != 'r'){
                    data[*len-2] = data[*len-1];
                }
                *len = *len-1;
                goto end;
            }
        }
    }
end:
    return;
}

//拼音格式化
static char eos='_';
static char pad='~';
static char prosody_boundary[]={'@','#','$'};

int qtk_tts_parse_char_in_prosody_boundary(char d)
{
    int i = 0;
    int n = sizeof(prosody_boundary)/sizeof(char);
    for(i = 0; i < n; ++i){
        if(d == prosody_boundary[i]){
            return 1;
        }
    }
    return 0;
}

wtk_string_t qtk_tts_parse_chn_pinyin_format(qtk_tts_parse_t *parse,char *data,char len)
{
    static char *translate_dict[] = {
        "a", "o", "e", "er", "ei", "ai", "ou", "ao", "en", "an", "eng", "ang", "ci", "chi", "cun", "cui", "chui", "chun", "dui", "dun", "diu", "gui", 
        "gun", "hui", "hun", "ju", "juan", "jun", "jiu", "jue", "kun", "kui", "lun", "liu", "lue", "niu", "qu", "quan", "qun", "que", "qiu", "ri", 
        "rui", "run", "si", "shi", "sun", "sui", "shui", "shun", "tui", "tun", "xu", "xuan", "xun", "xiu", "xue", "zi", "zhi", "zui", "zhun", "zhui", 
        "ya", "ye", "yao", "you", "yan", "yi", "yin", "yo", "yir", "yanr", "yu", "yang", "ying", "yong", "yingr", "yangr", "niur", "yuan", "yue",
        "yun", "yuanr", "duir", "dunr", "zhunr", "wa", "wo", "wu", "wai", "shir", "wei", "wan", "wen", "wanr", "weir", "huir", "gunr",
        "miu", "zun", "weng", "wang",
    };
    static char *translate[] = {
        "A","O","E","Er","Ei","Ai","Ou","Ao","En","An","Eng","Ang","cii","chiii","cuen",
        "cuei","chuei","chuen","duei","duen","diou","guei","guen","huei","huen",
        "jv","jvan","jvn","jiou","jve","kuen","kuei","luen","liou","lve","niou","qv","qvan",
        "qvn","qve","qiou","riii","ruei","ruen","sii","shiii","suen","suei","shuei","shuen",
        "tuei","tuen","xv","xvan","xvn","xiou","xve","zii","zhiii","zuei","zhuen","zhuei",
        "Ia","Ie","Iao","Iou","Ian","I","In","Io","Ir","Ianr","V","Iang","Ing","Iong","Ingr",
        "Iangr","niour","Van","Ve","Vn","Vanr","dueir","duenr","zhuenr","Ua","Uo","U","Uai","shiiir","Uei",
        "Uan","Uen","Uanr","Ueir","hueir","guenr","miou","zuen","Ueng","Uang",
    };
    int num = sizeof(translate_dict)/sizeof(char*);
    int i = 0;
    wtk_string_t new = {NULL,};
    wtk_strbuf_t *buf = NULL;
    int ulen = 0;
    // char *data = buf->data;
    // int len = buf->pos;

    //if sige
    if(len == 1){
        if(data[0] == eos || data[0] == pad || qtk_tts_parse_char_in_prosody_boundary(data[0])){
            new.data = data;
            new.len = len;
            goto end;
        }
    }
    buf = wtk_strbuf_new(256,1);
    ulen = (data[len-1] > '0' && data[len-1] <= '5') ? len-1 : len;

    for(i = 0; i < num; ++i){
        if(ulen == strlen(translate_dict[i])&&0 == strncmp(data,translate_dict[i],ulen)){
            wtk_strbuf_push(buf,translate[i],strlen(translate[i]));
            if(ulen != len)
                wtk_strbuf_push_c(buf,data[len-1]);
            new.data = wtk_heap_dup_str2(parse->heap,buf->data,buf->pos);
            new.len = buf->pos;
            goto end;
        }
    }
    if(buf->pos != 0){
        new.data = wtk_heap_dup_str2(parse->heap,buf->data,buf->pos);  //都没找到就是原始的
        new.len = buf->pos;
    }else{
        new.data = data;
        new.len = len;
    }
end:
    if(buf) wtk_strbuf_delete(buf);
    return new;
}

wtk_string_t qtk_tts_parse_chn_pinyin_format2(qtk_tts_parse_t *parse,char *data,char len)
{
    static char *translate_dict[] = {
        "a", "o", "e", "er", "ei", "ai", "ou", "ao", "en", "an", "eng", "ang", "ci", "chi", "cun", "cui", "chui", "chun", "dui", "dun", "diu", "gui",
        "gun", "hui", "hun", "ju", "juan", "jun", "jiu", "jue", "kun", "kui", "lun", "liu", "lue", "niu", "qu", "quan", "qun", "que", "qiu", "ri",
        "rui", "run", "si", "shi", "sun", "sui", "shui", "shun", "tui", "tun", "xu", "xuan", "xun", "xiu", "xue", "zi", "zhi", "zui", "zhun", "zhui",
        "ya", "ye", "yao", "you", "yan", "yi", "yin", "yo", "yir", "yanr", "yu", "yang", "ying", "yong", "yingr", "yangr", "niur", "yuan", "yue",
        "yun", "yuanr", "duir", "dunr", "zhunr", "wa", "wo", "wu", "wai", "shir", "wei", "wan", "wen", "wanr", "weir", "huir", "gunr",
        "miu", "zun", "weng", "wang",
    };
    static char *translate[] = {
        "A","O","E","Er","Ei","Ai","Ou","Ao","En","An","Eng","Ang","cii","chiii","cuen",
        "cuei","chuei","chuen","duei","duen","diou","guei","guen","huei","huen",
        "jv","jvan","jvn","jiou","jve","kuen","kuei","luen","liou","lve","niou","qv","qvan",
        "qvn","qve","qiou","riii","ruei","ruen","sii","shiii","suen","suei","shuei","shuen",
        "tuei","tuen","xv","xvan","xvn","xiou","xve","zii","zhiii","zuei","zhuen","zhuei",
        "Ia","Ie","Iao","Iou","Ian","I","In","Io","Ir","Ianr","VV","Iang","Ing","Iong","Ingr",
        "Iangr","niour","Van","Ve","Vn","Vanr","dueir","duenr","zhuenr","Ua","Uo","U","Uai","shiiir","Uei",
        "Uan","Uen","Uanr","Ueir","hueir","guenr","miou","zuen","Ueng","Uang",
    };
    int num = sizeof(translate_dict)/sizeof(char*);
    int i = 0;
    wtk_string_t new = {NULL,};
    wtk_strbuf_t *buf = NULL;
    int ulen = 0;
    // char *data = buf->data;
    // int len = buf->pos;

    //if sige 
    if(len == 1){
        if(data[0] == eos || data[0] == pad || qtk_tts_parse_char_in_prosody_boundary(data[0])){
            new.data = data;
            new.len = len;
            goto end;
        }
    }
    buf = wtk_strbuf_new(256,1);
    ulen = (data[len-1] > '0' && data[len-1] <= '5') ? len-1 : len;

    for(i = 0; i < num; ++i){
        if(ulen == strlen(translate_dict[i])&&0 == strncmp(data,translate_dict[i],ulen)){
            wtk_strbuf_push(buf,translate[i],strlen(translate[i]));
            if(ulen != len)
                wtk_strbuf_push_c(buf,data[len-1]);
            new.data = wtk_heap_dup_str2(parse->heap,buf->data,buf->pos);
            new.len = buf->pos;
            goto end;
        }
    }
    if(buf->pos != 0){
        new.data = wtk_heap_dup_str2(parse->heap,buf->data,buf->pos);  //都没找到就是原始的
        new.len = buf->pos;
    }else{
        new.data = data;
        new.len = len;
    }
end:
    if(buf) wtk_strbuf_delete(buf);
    return new;
}
//切分成声韵母和声调
wtk_string_t qtk_tts_parse_chn_seqrate_syllable(qtk_tts_parse_t*parse, char *data, int len)
{
    static char *consonant_list[] = {
        "b", "p", "m", "f", "d", "t", "n", "l", "g", "k",
        "h", "j", "q", "x", "zh", "ch", "sh", "r", "z",
        "c", "s", "y", "w",
    } ;
    wtk_string_t new = {NULL};
    wtk_strbuf_t *buf = NULL;
    // char *data = buf->data;
    // int len = buf->pos;
    int i = 0,num = sizeof(consonant_list)/sizeof(char*);
    int ntonel = 0;

    if(len == 1){
        if(data[0] == eos || data[0] == pad || qtk_tts_parse_char_in_prosody_boundary(data[0])){
            new.data = data;
            new.len = len;
            goto end;
        }
    }
    buf = wtk_strbuf_new(256,1);
    ntonel = (data[len-1] > '0' && data[len-1] <= '5') ? len-1 : len;

    // if(ntonel == 1){
    //     wtk_strbuf_push_c(buf,data[0]);
    //     wtk_strbuf_push_c(buf,' ');
    //     if(ntonel != len)
    //         wtk_strbuf_push_c(buf,data[1]);
    //     goto end;
    // }
    for(i = 0; i < num;++i){
        if(0 == strncmp(consonant_list[i],data,strlen(consonant_list[i]))){
            wtk_strbuf_push(buf,consonant_list[i],strlen(consonant_list[i]));
            wtk_strbuf_push_c(buf,' ');
            wtk_strbuf_push(buf,data+strlen(consonant_list[i]),ntonel-strlen(consonant_list[i]));
            if(ntonel != len){
                wtk_strbuf_push_c(buf,' ');
                wtk_strbuf_push_c(buf,data[len-1]);
            }
            new.data = wtk_heap_dup_str2(parse->heap,buf->data,buf->pos);
            new.len = buf->pos;
            goto end;
        }
    }
    //只有韵母的情况
    wtk_strbuf_push(buf,data,ntonel);
    if(ntonel != len){
        wtk_strbuf_push_c(buf,' ');
        wtk_strbuf_push_c(buf,data[len-1]);
    }
    new.data = wtk_heap_dup_str2(parse->heap,buf->data,buf->pos);
    new.len = buf->pos;
end:
    if(buf) wtk_strbuf_delete(buf);
    return new;
}

//再次格式化，引入各信息
/**
 * reformat token sequence to phoneme sequence,toner sequence and prosody sequence
 *     input:
        n in 2 h ao 3 $
    return :
        phoneme:['k', 'a', 'Er', 'p', 'u', 'p', 'ei', 'Uai', 's', 'uen', 'Uan', 'h', 'ua', 't', 'i']
        toner:['2', '2', '2', '3', '3', '2', '2', '4', '1', '1', '2', '2', '2', '1', '1']
        prosody:['0', '_', '_', '0', '#', '0', '_', '_', '0', '@', '_', '0', '_', '0', '$']
 */
wtk_string_t qtk_tts_parse_chn_reformat(qtk_tts_parse_t*parse, char *data, int len)
{
	char *s, *e;
	wtk_strbuf_t *buf;
	wtk_string_t rst;

	buf=wtk_strbuf_new(128, 1);
	s=data;
	e=s+len;

	wtk_strbuf_push_s(buf, "bos");
	while(s < e)
	{
		if (*s >'0' && *s <= '5')
		{
			wtk_strbuf_push_c(buf, *s);
			wtk_strbuf_push_s(buf, " #S");
		}
		else if (qtk_tts_parse_char_in_prosody_boundary(*s))
		{
			//buf->pos--;
			//wtk_strbuf_push_s(buf, " #S");
			wtk_strbuf_push_c(buf, *s);
		}
		else if(*s != ' ')
		{
			wtk_strbuf_push_c(buf, *s);
		}else{
			wtk_strbuf_push_c(buf, ' ');
		}
		s++;
	}
	wtk_strbuf_push_s(buf, " eos");
	rst.data = wtk_heap_dup_str2(parse->heap,buf->data,buf->pos);
	rst.len = buf->pos;
	wtk_strbuf_delete(buf);

	return rst;
}

wtk_string_t qtk_tts_parse_chn_reformat2(qtk_tts_parse_t*parse, char *data, int len)
{
	char *s, *e;
	wtk_strbuf_t *buf;
	wtk_string_t rst;

	buf=wtk_strbuf_new(128, 1);
	s=data;
	e=s+len;

	wtk_strbuf_push_s(buf, "sil");
	while(s < e)
	{
		if (*s == '$')
		{
			wtk_strbuf_push_c(buf, *s);
			wtk_strbuf_push_s(buf, " sil");
		}
		else
		{
			wtk_strbuf_push_c(buf, *s);
		}
		s++;
	}
	rst.data = wtk_heap_dup_str2(parse->heap,buf->data,buf->pos);
	rst.len = buf->pos;
	wtk_strbuf_delete(buf);

	return rst;
}

char *qtk_tts_parse_arpabet(char *data,int len)
{
    static char *list[] = {
        "b","p","m","f","d","t","n","l","g","k","h","j","q","x","zh","ch","sh","r","z","s","y","w","c",
        "a","ai","an","ao","ang","e","er","ei","eng","en","i","iu","ia","ian","ie","iao","in","iang",
        "iong","ing","u","uai","ua","un","uo","ui","uang","uan","ue","o","ou","ong","v","ve","vn","van"
    };
    static char *convlist[] = {
        "B","P","M","F","D","T","N","L","G","K","HH","J","Q","X","JH","CH","SH","R","Z","S","Y","W","TS",
        "AA","AY","AE N","AW","AE NG","ER","AA R","EY","AH NG","AH N","IY","IY UH","IY AA","IY AE N","IY EH","IY AW","IY N",
        "IY AE NG","IY UH NG","IY NG","UW","UW AY","UW AA","UW AH N","UW AO","UW IY","UW AE NG","UW AE N","IY UW EH",
        "AO","OW","UH NG","IY UW","IY UW EH","UW AH N","UW AE N",
    };
    int ret = -1;
    int n = sizeof(list)/sizeof(char*),i = 0;
    int slen = 0;

    for(i = 0; i < n; ++i){
        slen = strlen(list[i]);
        if(slen == len && !strncmp(data,list[i],slen)){
            ret = i;
            goto end;
        }
    }
end:
    if(ret != -1){
        return convlist[ret];
    }
    return NULL;
}

wtk_string_t qtk_tts_parse_convert2arpabet(qtk_tts_parse_t *parse,char *data,int len)
{
    wtk_strbuf_t *buf = NULL,*tmp = NULL;
    wtk_string_t seq_syllable = {NULL,0};
    int ulen = 0, erhua = 0;
    char *s,*e,*arpabet;
    
    //is sign
    if(len == 1){
        if(data[0] == eos || data[0] == pad || qtk_tts_parse_char_in_prosody_boundary(data[0])){
            seq_syllable.data = data;
            seq_syllable.len = len;
            goto end;
        }
    }
    //SIL
    if(len == 3 && !strncmp(data,"SIL",len)){
        seq_syllable.data = data;
        seq_syllable.len = len;
        goto end;
    }
    ulen = (data[len-1] > '0' && data[len-1] <= '5') ? len-1 : len;
    if(data[ulen-1] == 'r' && ulen >= 2 && (ulen != 2 || strncmp("er",data,ulen))){
        ulen -= 1;
        erhua = 1;
    }else{
        erhua = 0;
    }
    seq_syllable = qtk_tts_parse_chn_seqrate_syllable(parse,data,ulen);
    buf = wtk_strbuf_new(256,1.0f);
    tmp = wtk_strbuf_new(256,1.0f);
    s = seq_syllable.data;
    e = s+seq_syllable.len;
    while(s <= e){
        if(s != e && *s != ' '){
            wtk_strbuf_push_c(buf,*s);
            s++;
            continue;
        }
        arpabet =  qtk_tts_parse_arpabet(buf->data,buf->pos);
        if(arpabet != NULL){
            wtk_strbuf_push(tmp,arpabet,strlen(arpabet));
            wtk_strbuf_push_c(tmp,' ');
        }
        wtk_strbuf_reset(buf);
        s++;
    }
    if(erhua)
        wtk_strbuf_push(tmp,"R ",strlen("R "));
    if(ulen != len){
        wtk_strbuf_push_c(tmp,data[len-1]);
    }else{
        wtk_strbuf_push_c(tmp,'5');
    }
    seq_syllable.data = wtk_heap_dup_str2(parse->heap,tmp->data,tmp->pos);
    seq_syllable.len = tmp->pos;
end:
    if(buf) wtk_strbuf_delete(buf);
    if(tmp) wtk_strbuf_delete(tmp);
    return seq_syllable;
}

wtk_veci_t* qtk_tts_parse_chn_list2id(qtk_tts_parse_t *parse,char *data,int len)
{
    char *s = data,*e = data+len;
    wtk_strbuf_t *buf = NULL;
    wtk_strbuf_t *seq = NULL;
    wtk_strbuf_t *tmp = NULL;
    wtk_veci_t *vec = NULL;
    wtk_string_t tok = {NULL,0};
    int id = 0;
    char *st = 0,*et = 0;

    buf = wtk_strbuf_new(256,1);
    seq = wtk_strbuf_new(64,1);
    tmp = wtk_strbuf_new(128,1.0f);

    wtk_strbuf_reset(tmp);
    while(s <= e){
        if(s != e && *s != ' '){
            wtk_strbuf_push_c(buf,*s);
            s++;
            continue;
        }
        s++;
        if(buf->pos == 0){
            continue;
        }
        tok.data = buf->data;
        tok.len = buf->pos;
        if(0){  //不兼容了  以后去掉
            // qtk_tts_parse_chn_erhua(tok.data,&tok.len);
            // tok = qtk_tts_parse_chn_pinyin_format(parse,tok.data,tok.len);
            // tok = qtk_tts_parse_chn_seqrate_syllable(parse,tok.data,tok.len);
        }else{  //use arpabet syllable
            tok = qtk_tts_parse_convert2arpabet(parse,tok.data,tok.len);
        }
        //to id 
        st = tok.data;
        et = tok.data+tok.len;
        while(st <= et){
            if(st != et && *st != ' '){
                wtk_strbuf_push_c(seq,*st);
                st++;
                continue;
            }
//            printf("1. %.*s\n", seq->pos, seq->data);
            id = qtk_tts_symbols_id_get_id(parse->symbols,seq->data,seq->pos);
//            wtk_debug("2. %d %.*s\n",id,seq->pos,seq->data);
            wtk_strbuf_push(tmp,(char*)&id,sizeof(id));
            st++;
            wtk_strbuf_reset(seq);
        }
        wtk_strbuf_reset(seq);
        wtk_strbuf_reset(buf);
    }
    // id = 1; //push句子结尾
    // wtk_strbuf_push(tmp,(char*)&id,sizeof(id));
    vec = wtk_veci_heap_new(parse->heap,tmp->pos/sizeof(int));
    memcpy(vec->p,tmp->data,tmp->pos);
    wtk_strbuf_delete(tmp);
    wtk_strbuf_delete(seq);
    wtk_strbuf_delete(buf);
    // wtk_veci_print(vec);
    return vec;
}

// support qtk_tts_parse_chn_list2id
wtk_veci_t* qtk_tts_parse_chn_list2id2(qtk_tts_parse_t *parse,char *data,int len)
{
//	data = "you3 xin1 de5 @ ting2 che1 @ jian1 kong4 shi4 pin2 $ qing3 @ lian2 jie1 shou3 ji1 @ cha2 kan4 $ ~";
//	len = strlen(data);
    char *s = data,*e = data+len;
    wtk_strbuf_t *buf = NULL;
    wtk_strbuf_t *tokbuf = NULL;
    wtk_strbuf_t *seq = NULL;
    wtk_strbuf_t *tmp = NULL;
    wtk_veci_t *vec = NULL;
    wtk_string_t tok = {NULL,0};
    int id = 0;
    char *st = 0,*et = 0;

    buf = wtk_strbuf_new(256,1);
    tokbuf = wtk_strbuf_new(256,1);
    seq = wtk_strbuf_new(64,1);
    tmp = wtk_strbuf_new(128,1.0f);

    // wtk_debug("===%.*s\n", len, data);
    //
    wtk_strbuf_reset(tmp);
    while(s <= e){
    	//get string which don't contain space.
        if(s != e && *s != ' '){
            wtk_strbuf_push_c(buf,*s);
            s++;
            continue;
        }
        s++;
        //skip space of begin position
        if(buf->pos == 0){
            continue;
        }

        tok.data = buf->data;
        tok.len = buf->pos;
        // printf("forvite: %d tok: %.*s\n", parse->cfg->forvits, tok.len, tok.data);
        if(parse->cfg->forvits || parse->cfg->fordevicetts){
        	//skip symbol '|_~'
        	if (buf->data[0]=='|' || buf->data[0]=='_' || buf->data[0]=='~')
        	{
            	wtk_strbuf_reset(buf);
            	continue;
        	}
        	//prosody doing ,pl: '@', '#', '$'
            if (qtk_tts_parse_char_in_prosody_boundary(buf->data[0]))
            {
            	//only keep one prosody symbol which is the first.
            	if (tokbuf->pos > 0 && qtk_tts_parse_char_in_prosody_boundary(tokbuf->data[tokbuf->pos-1]))
            	{
                	//wtk_strbuf_push_c(tokbuf, ' ');
            		tokbuf->pos--;
            	}else
            	{
            		wtk_strbuf_push_c(tokbuf, ' ');
            	}
            	wtk_strbuf_push_c(tokbuf, buf->data[0]);
            	wtk_strbuf_reset(buf);
            	continue;
            }
             qtk_tts_parse_chn_erhua(tok.data,&tok.len);
             tok = qtk_tts_parse_chn_pinyin_format2(parse,tok.data,tok.len);
//             printf("tok: %.*s\n", tok.len, tok.data);
             tok = qtk_tts_parse_chn_seqrate_syllable(parse,tok.data,tok.len);
//             printf("tok2: %.*s\n", tok.len, tok.data);
             wtk_strbuf_push_c(tokbuf, ' ')
             wtk_strbuf_push(tokbuf, tok.data, tok.len);
        }else{  //use arpabet syllable
            tok = qtk_tts_parse_convert2arpabet(parse,tok.data,tok.len);
        }
        wtk_strbuf_reset(buf);
    }
//    printf("tokbuf: %.*s\n", tokbuf->pos, tokbuf->data);
    if (parse->cfg->forvits)
    	tok = qtk_tts_parse_chn_reformat(parse, tokbuf->data, tokbuf->pos);
    else if (parse->cfg->fordevicetts)
    	tok = qtk_tts_parse_chn_reformat2(parse, tokbuf->data, tokbuf->pos);
    else{
    	tok.data = tokbuf->data;
    	tok.len = tokbuf->pos;
    }

    //to id
    st = tok.data;
    et = tok.data+tok.len;
//    printf("tok: %.*s\n", tok.len, tok.data);
    wtk_strbuf_reset(tmp);
    while(st <= et){
        if(st != et && *st != ' '){
            wtk_strbuf_push_c(seq,*st);
            st++;
            continue;
        }
        id = qtk_tts_symbols_id_get_id(parse->symbols,seq->data,seq->pos);
        if (0==parse->cfg->forvits && 0==parse->cfg->fordevicetts)
        	id = id+1;
        wtk_strbuf_push(tmp,(char*)&id,sizeof(id));
        st++;
        wtk_strbuf_reset(seq);
    }
    wtk_strbuf_reset(seq);

    vec = wtk_veci_heap_new(parse->heap,tmp->pos/sizeof(int));
    memcpy(vec->p,tmp->data,tmp->pos);
    wtk_strbuf_delete(tmp);
    wtk_strbuf_delete(seq);
    wtk_strbuf_delete(buf);
    wtk_strbuf_delete(tokbuf);
    // wtk_veci_print(vec);
    return vec;
}

int qtk_tts_parse_chn_lab2id(qtk_tts_parse_t *parse)
{
    // wtk_strbuf_t **lbuf = NULL;
    int i = 0,n = 0;
    wtk_veci_t **vecs = NULL;
    wtk_string_t **prosody_list = NULL;

    n = parse->nid;
    prosody_list = parse->prosody->prosody_list;
    vecs = wtk_heap_malloc(parse->heap,sizeof(wtk_veci_t*)*n);
     for(i = 0; i < n; ++i){
        if(prosody_list[i] == NULL){
            vecs[i] = wtk_veci_heap_new(parse->heap,0);
        }else{
        	if (parse->cfg->forvits || parse->cfg->fordevicetts)
        		vecs[i] = qtk_tts_parse_chn_list2id2(parse,prosody_list[i]->data,prosody_list[i]->len);
        	else
        		vecs[i] = qtk_tts_parse_chn_list2id(parse,prosody_list[i]->data,prosody_list[i]->len);
        }
    }
    parse->id_vec = vecs;
    return 0;
}

int qtk_tts_parse_lab2id(qtk_tts_parse_t *parse)
{
    int ret = 0;

    if(parse->cfg->symbols.is_en){
        ret = qtk_tts_parse_en_lab2id(parse);
    }else{
        parse->nid = parse->prosody->nids;
        ret = qtk_tts_parse_chn_lab2id(parse);    //终于转成id了
    }
    return ret;
}

wtk_string_t qtk_tts_prase_strip(qtk_tts_parse_t *parse,wtk_tts_info_t *info,char *txt,int len)
{
    wtk_string_t str = {NULL,};
    wtk_strbuf_t *buf = NULL;
    char *s = NULL,*e = NULL;
    int n = 0;

    s = txt;e = txt+len;
    buf = wtk_strbuf_new(256,1.0f);
    while(s < e){
        n = wtk_utf8_bytes(*s);
        if(!isspace((unsigned char)*s)){
            wtk_strbuf_push(buf,s,n);
        }
        s += n;
    }
    str.len = buf->pos;
    str.data = wtk_heap_dup_str2(info->heap,buf->data,buf->pos);
    wtk_strbuf_delete(buf);
    return str;
}

wtk_string_t qtk_tts_prase_strip_alpha(qtk_tts_parse_t *parse,wtk_tts_info_t *info,char *txt,int len)
{
    wtk_string_t str = {NULL,};
    wtk_strbuf_t *buf = NULL;
    char *s = NULL,*e = NULL;
    int n = 0;

    s = txt;e = txt+len;
    buf = wtk_strbuf_new(256,1.0f);
    while(s < e){
        n = wtk_utf8_bytes(*s);
        if(n == 1 && isalpha(*s)){
            s += n;
            continue;
        }
        wtk_strbuf_push(buf,s,n);
        s += n;
    }
    str.len = buf->pos;
    str.data = wtk_heap_dup_str2(info->heap,buf->data,buf->pos);
    wtk_strbuf_delete(buf);
    return str;
}

int qtk_tts_parse_process(qtk_tts_parse_t *parse, char *txt,int len)
{
    int ret = 0;
    wtk_tts_lab_t* lab = NULL;
	wtk_tts_info_t info;

    qtk_tts_parse_reset(parse);
    info.heap = parse->heap;
    info.buf = parse->buf;
    //norm
    parse->txt = wtk_tts_norm_process(parse->norm,txt,len);
    if(parse->txt.data == NULL) {printf("norm_process error\n");ret = -1;goto end;}
    if(parse->cfg->is_en == 0){
        //去除空格
        parse->txt = qtk_tts_prase_strip(parse,&info,parse->txt.data,parse->txt.len);
        //因为现在不支持中英文合成  所以去除英文字符
        parse->txt = qtk_tts_prase_strip_alpha(parse,&info,parse->txt.data,parse->txt.len);
    }
    if(parse->cfg->debug_norm){
        printf("[txt]: %.*s\n",parse->txt.len,parse->txt.data);
    }
    lab = wtk_tts_segsnt_process(parse->segsnt,&info,parse->txt.data,parse->txt.len);
    if(parse->cfg->debug_segsnt){
        wtk_tts_segsnt_print(lab);
    }
    if(parse->cfg->is_en || parse->cfg->use_segmax){
        ret = wtk_tts_segwrd_process(parse->segwrd,&info,lab);
    }else{
    	ret = qtk_tts_segwrd_process(parse->segwrd2,&info,lab);
    }
    if(parse->cfg->debug_segwrd){
        wtk_tts_lab_segwrd_print(lab);
    }
    qtk_tts_polyphone_process(parse->ply,&info,lab);    //消除多音字
    if(parse->cfg->debug_polyphone){
        wtk_tts_lab_segwrd_print(lab);
    }
    ret = qtk_tts_prosody_process(parse->prosody,&info,lab);    //韵律
    if(ret != 0){
    	wtk_debug("get prosody error\n");
    	goto end;
    }
    if(parse->cfg->debug_prosody){
        qtk_tts_prosody_print(parse->prosody);
    }
    parse->lab = lab;
    ret = qtk_tts_parse_lab2id(parse);
    if(parse->cfg->debug_id){
        qtk_tts_parse_id_print(parse);
    }
end:
    return ret;
}

int qtk_tts_parse_reset(qtk_tts_parse_t *parse)
{
    wtk_tts_norm_reset(parse->norm);
    wtk_tts_segsnt_reset(parse->segsnt);
    if(parse->cfg->is_en || parse->cfg->use_segmax){
        wtk_tts_segwrd_reset(parse->segwrd);
    }
    wtk_heap_reset(parse->heap);
    wtk_strbuf_reset(parse->buf);
    parse->id_vec = NULL;

    return 0;
}

int qtk_tts_parse_symbols_id_chn_print(qtk_tts_parse_t *parse)
{
    int i = 0,j = 0;
    wtk_veci_t *vec = NULL;

    for(i = 0; i < parse->nid; i++){
        vec = parse->id_vec[i];
        for(j = 0; j < vec->len;++j){
        	if (j==vec->len-1)
        		printf("%d",vec->p[j]);
        	else
        		printf("%d ",vec->p[j]);
        }
        puts("");
    }

    return 0;
}

int qtk_tts_parse_id_en_print(qtk_tts_parse_t *parse)
{
    wtk_veci_t *vec = NULL,**vecs = NULL;
    int ret = 0,n = 0;
    int i = 0,j = 0;

    n = parse->nid;
    vecs = parse->id_vec;
    for(i = 0; i < n; ++i){
        vec = vecs[i];
        for(j = 0; j < vec->len; ++j)
            printf("%d ",vec->p[j]);
        printf("\n");
    }

    return ret;
}

int qtk_tts_parse_id_print(qtk_tts_parse_t *parse)
{
    if(parse->cfg->symbols.is_en){
        qtk_tts_parse_id_en_print(parse);
    }else{
        qtk_tts_parse_symbols_id_chn_print(parse);
    }
    return 0;
}
