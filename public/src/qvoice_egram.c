#include "wtk/asr/wfst/egram/wtk_egram.h"
#include "wtk/core/cfg/wtk_mbin_cfg.h"
#include "wtk/core/cfg/wtk_opt.h"
#include "public/inc/qvoice_egram.h"
#include "wtk/core/json/wtk_json_parse.h"
//chn info
//in_id to dnn_id(orignal id) 0-5680
static short chn_map[]=
{
#include "res/asr/huami-res/chn.map"
};

//in_id to dnn_id+1 in_id(final_cut.mdl id) 0-5680
static short chn_map2[]=	//0-5680 max_in 1190
{
#include "res/asr/huami-res/chn.map2"
};

static short chn_map3[]=   //1190
{
#include "res/asr/huami-res/chn.did"
};

static short chn_lmin[]=
{
#include "res/asr/huami-res/chn.min"
};

static unsigned char chn_lshift[]=
{
#include "res/asr/huami-res/chn.shift"
};

static unsigned char chn_l[]=//3045*40
{
#include "res/asr/huami-res/chn.linear"
};

static unsigned char chn_b[]=
{
#include "res/asr/huami-res/chn.bias"
};

char chn_mdl_info[]={12,1,0,0,0,0,12,0,4};


//eng info
//in_id to dnn_id(orignal id)
static short eng_map[]=
{
#include "res/asr/huami-res/eng.map1"
};

//in_id to dnn_id+1 in_id(final_cut.mdl id)
static short eng_map2[]=
{
#include "res/asr/huami-res/eng.map2"
};

static short eng_map3[]=
{
#include "res/asr/huami-res/eng.map3"
};

static short eng_lmin[]=
{
#include "res/asr/huami-res/eng.min"
};

static unsigned char eng_lshift[]=
{
#include "res/asr/huami-res/eng.shift"
};

static unsigned char eng_l[]=//3045*40
{
#include "res/asr/huami-res/eng.linear"
};

static unsigned char eng_b[]=
{
#include "res/asr/huami-res/eng.bias"
};

char eng_mdl_info[]={11,1,0,0,0,0,24,0,4};



//span info
//in_id to dnn_id(orignal id)
static short span_map[]=
{
#include "res/asr/huami-res/span.map1"
};

//in_id to dnn_id+1 in_id(final_cut.mdl id)
static short span_map2[]=
{
#include "res/asr/huami-res/span.map2"
};

static short span_map3[]=
{
#include "res/asr/huami-res/span.map3"
};

static short span_lmin[]=
{
#include "res/asr/huami-res/span.min"
};

static unsigned char span_lshift[]=
{
#include "res/asr/huami-res/span.shift"
};

static unsigned char span_l[]=//3045*40
{
#include "res/asr/huami-res/span.linear"
};

static unsigned char span_b[]=
{
#include "res/asr/huami-res/span.bias"
};

char span_mdl_info[]={12,1,0,0,0,0,12,0,4};


//ger info
//in_id to dnn_id(orignal id)
static short ger_map[]=
{
#include "res/asr/huami-res/ger.map1"
};

//in_id to dnn_id+1 in_id(final_cut.mdl id)
static short ger_map2[]=
{
#include "res/asr/huami-res/ger.map2"
};

static short ger_map3[]=
{
#include "res/asr/huami-res/ger.map3"
};

static short ger_lmin[]=
{
#include "res/asr/huami-res/ger.min"
};

static unsigned char ger_lshift[]=
{
#include "res/asr/huami-res/ger.shift"
};

static unsigned char ger_l[]=
{
#include "res/asr/huami-res/ger.linear"
};

static unsigned char ger_b[]=
{
#include "res/asr/huami-res/ger.bias"
};

char ger_mdl_info[]={11,1,0,0,0,0,12,0,4};

typedef struct
{
	wtk_mbin_cfg_t *bin_cfg_zh;
	wtk_mbin_cfg_t *bin_cfg_en;
	wtk_mbin_cfg_t *bin_cfg_de;
	wtk_mbin_cfg_t *bin_cfg_es;
	wtk_egram_t *e_zh;//chinese
	wtk_egram_t *e_en;//english
	wtk_egram_t *e_de;//deutsch
	wtk_egram_t *e_es;//spannish
    wtk_json_parser_t *json;

}qvoice_egram_t;

wtk_ecut_cfg_t ecut_chn;
wtk_ecut_cfg_t ecut_ger;
wtk_ecut_cfg_t ecut_eng;
wtk_ecut_cfg_t ecut_sp;

static void prepare_egram(qvoice_egram_t *e, char *cfg_fn)
{
	wtk_egram_cfg_t *xx;

	ecut_chn.min = -143;
	ecut_chn.shift = 7;
	ecut_chn.sil_id1 = 1;
	ecut_chn.sil_id2 = 45;
	ecut_chn.map1 = chn_map;
	ecut_chn.map2 = chn_map2;
	ecut_chn.map3 = chn_map3;
	ecut_chn.lmin = chn_lmin;
	ecut_chn.lshift = chn_lshift;
	ecut_chn.l = chn_l;
	ecut_chn.b = chn_b;
	ecut_chn.mdl_info = chn_mdl_info;
	ecut_chn.num_in = 5680;
	ecut_chn.num_dnn = 3045;
	ecut_chn.num_cutdnn = 1190;
	ecut_chn.num_col = 68;
	e->bin_cfg_zh=wtk_mbin_cfg_new_type(wtk_egram_cfg,cfg_fn,
		"./egram.cfg.c");
	e->e_zh=wtk_egram_new2(e->bin_cfg_zh);
	e->e_zh->ecut = &ecut_chn;

	ecut_sp.min = -51;
	ecut_sp.shift = 6;
	ecut_sp.sil_id1 = 1;
	ecut_sp.sil_id2 = 50;
	ecut_sp.map1 = span_map;
	ecut_sp.map2 = span_map2;
	ecut_sp.map3 = span_map3;
	ecut_sp.lmin = span_lmin;
	ecut_sp.lshift = span_lshift;
	ecut_sp.l = span_l;
	ecut_sp.b = span_b;
	ecut_sp.mdl_info = span_mdl_info;
	ecut_sp.num_in = 6854;
	ecut_sp.num_dnn = 1592;
	ecut_sp.num_cutdnn = 1091;
	ecut_sp.num_col = 60;
	e->bin_cfg_es=wtk_mbin_cfg_new_type(wtk_egram_cfg,cfg_fn,
		"./egram.cfg.s");
	e->e_es=wtk_egram_new2(e->bin_cfg_es);
	e->e_es->ecut = &ecut_sp;


	e->bin_cfg_de=wtk_mbin_cfg_new_type(wtk_egram_cfg,cfg_fn,
		"./egram.cfg.g");
	xx = e->bin_cfg_de->cfg;
	xx->type = 1;
	xx->e2fst.type = 1;
	xx->xbnfnet.type = 1;
	e->e_de=wtk_egram_new2(e->bin_cfg_de);


	ecut_ger.min = -96;
	ecut_ger.shift = 7;
	ecut_ger.sil_id1 = 1;
	ecut_ger.sil_id2 = 83;
	ecut_ger.map1 = ger_map;
	ecut_ger.map2 = ger_map2;
	ecut_ger.map3 = ger_map3;
	ecut_ger.lmin = ger_lmin;
	ecut_ger.lshift = ger_lshift;
	ecut_ger.l = ger_l;
	ecut_ger.b = ger_b;
	ecut_ger.mdl_info = ger_mdl_info;
	ecut_ger.num_in = 10262;
	ecut_ger.num_dnn = 1680;
	ecut_ger.num_cutdnn = 1256;
	ecut_ger.num_col = 60;
	e->e_de->ecut = &ecut_ger;

	ecut_eng.min = -122;
	ecut_eng.shift = 7;
	ecut_eng.sil_id1 = 1;
	ecut_eng.sil_id2 = 38;
	ecut_eng.map1 = eng_map;
	ecut_eng.map2 = eng_map2;
	ecut_eng.map3 = eng_map3;
	ecut_eng.lmin = eng_lmin;
	ecut_eng.lshift = eng_lshift;
	ecut_eng.l = eng_l;
	ecut_eng.b = eng_b;
	ecut_eng.mdl_info = eng_mdl_info;
	ecut_eng.num_in = 27842;
	ecut_eng.num_dnn = 2933;
	ecut_eng.num_cutdnn = 1431;
	ecut_eng.num_col = 54;
	e->bin_cfg_en=wtk_mbin_cfg_new_type(wtk_egram_cfg,cfg_fn,
		"./egram.cfg.e");
	e->e_en=wtk_egram_new2(e->bin_cfg_en);
	e->e_en->ecut = &ecut_eng;
}


void *qvoice_egram_new(char *fn)
{
	qvoice_egram_t *e = (qvoice_egram_t*)wtk_malloc(sizeof(qvoice_egram_t));

	e->json=wtk_json_parser_new();
	prepare_egram(e,fn);
	return e;
}

void qvoice_egram_delete(void *inst)
{
	qvoice_egram_t *e = (qvoice_egram_t*)inst;

	wtk_json_parser_delete(e->json);
    wtk_egram_delete(e->e_de);
    wtk_egram_delete(e->e_en);
    wtk_egram_delete(e->e_es);
    wtk_egram_delete(e->e_zh);
    wtk_mbin_cfg_delete(e->bin_cfg_de);
    wtk_mbin_cfg_delete(e->bin_cfg_en);
    wtk_mbin_cfg_delete(e->bin_cfg_es);
    wtk_mbin_cfg_delete(e->bin_cfg_zh);
    wtk_free(e);
}

int qvoice_egram_feed(void *inst, char *data, int len)
{
	qvoice_egram_t *e = (qvoice_egram_t*)inst;
	wtk_json_item_t *en,*zh,*es,*de;
	wtk_strbuf_t *buf;
	int i;
	char c;

	buf = wtk_strbuf_new(1024,1);
    wtk_json_parser_t *json=e->json;
    wtk_json_parser_reset(json);
    wtk_json_parser_parse(json, data, len);

    en = wtk_json_obj_get_s(json->json->main, "en");
    zh = wtk_json_obj_get_s(json->json->main, "zh");
    es = wtk_json_obj_get_s(json->json->main, "es");
    de = wtk_json_obj_get_s(json->json->main, "de");

    if(zh)
    {
    	wtk_strbuf_reset(buf);
    	wtk_strbuf_push(buf,"$main = ",strlen("$main = "));
    	wtk_strbuf_push(buf,zh->v.str->data,zh->v.str->len);
    	wtk_strbuf_push(buf,";\n(\\<s\\>($main)\\<\\/s\\>)",strlen(";\n(\\<s\\>($main)\\<\\/s\\>)"));
    	wtk_egram_ebnf2fst(e->e_zh,buf->data,buf->pos);
    	wtk_egram_qlite_bin(e->e_zh,NULL);
    	printf("zh done!\n");
    }

    if(en)
    {
    	wtk_strbuf_reset(buf);

    	for(i=0;i<en->v.str->len;i++)
    	{
    		c = en->v.str->data[i];

    		if(c>='a' && c<='z')
    		{
    			en->v.str->data[i] = c - 32;
    		}
    	}
    	wtk_strbuf_push(buf,"$main = ",strlen("$main = "));
    	wtk_strbuf_push(buf,en->v.str->data,en->v.str->len);
    	wtk_strbuf_push(buf,";\n(\\<s\\>($main)\\<\\/s\\>)",strlen(";\n(\\<s\\>($main)\\<\\/s\\>)"));
    	//wtk_debug("%.*s\n",buf->pos,buf->data);
    	wtk_egram_ebnf2fst(e->e_en,buf->data,buf->pos);
    	wtk_egram_qlite_bin(e->e_en,NULL);
    	printf("en done!\n");
    }

    if(es)
    {
    	wtk_strbuf_reset(buf);
    	wtk_strbuf_push(buf,"$main = ",strlen("$main = "));
    	wtk_strbuf_push(buf,es->v.str->data,es->v.str->len);
    	wtk_strbuf_push(buf,";\n(\\<s\\>($main)\\<\\/s\\>)",strlen(";\n(\\<s\\>($main)\\<\\/s\\>)"));
    	wtk_egram_ebnf2fst(e->e_es,buf->data,buf->pos);
    	wtk_egram_qlite_bin(e->e_es,NULL);
    	printf("es done!\n");
    }

    if(de)
    {
    	wtk_strbuf_reset(buf);
    	wtk_strbuf_push(buf,"$main = ",strlen("$main = "));
    	wtk_strbuf_push(buf,de->v.str->data,de->v.str->len);
    	wtk_strbuf_push(buf,";\n(\\<s\\>($main)\\<\\/s\\>)",strlen(";\n(\\<s\\>($main)\\<\\/s\\>)"));
    	wtk_egram_ebnf2fst(e->e_de,buf->data,buf->pos);
    	wtk_egram_qlite_bin(e->e_de,NULL);
    	printf("de done!\n");
    }

    wtk_strbuf_delete(buf);
    return 0;
}
