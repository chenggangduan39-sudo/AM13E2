#include "wtk_ebnfdec.h" 

wtk_ebnfdec_t* wtk_ebnfdec_new(wtk_ebnfdec_cfg_t *cfg)
{
	wtk_ebnfdec_t *dec;

	dec=(wtk_ebnfdec_t*)wtk_malloc(sizeof(wtk_ebnfdec_t));
	dec->cfg=cfg;
	if(cfg->use_bin)
	{
		dec->egram=wtk_egram_new2(cfg->egram_bin_cfg);
		dec->net=wtk_fst_net_new(&(dec->egram->e2fst->cfg->net));
		dec->dec=wtk_wfstdec_new(cfg->dec_bin_cfg);
	}else
	{
		dec->egram=wtk_egram_new((wtk_egram_cfg_t*)(cfg->egram_cfg->cfg),NULL);
		dec->net=wtk_fst_net_new(&(dec->egram->e2fst->cfg->net));
		dec->dec=wtk_wfstdec_new((wtk_wfstdec_cfg_t*)cfg->dec_cfg->cfg);
	}
	dec->dec->parm->dnn->cfg->use_linear_output=0;
	//cfg->dec_cfg->use_mt=0;
	if(dec->cfg->vad_cfg)
	{
		wtk_queue_init(&(dec->vad_q));
		dec->vad=wtk_vad_new(cfg->vad_cfg,&(dec->vad_q));
	}else
	{
		dec->vad=NULL;
	}
	return dec;
}

void wtk_ebnfdec_delete(wtk_ebnfdec_t *dec)
{
	if(dec->vad)
	{
		wtk_vad_delete(dec->vad);
	}
	wtk_egram_delete(dec->egram);
	wtk_fst_net_delete(dec->net);
	wtk_wfstdec_delete(dec->dec);
	wtk_free(dec);
}

int wtk_ebnfdec_set_word(wtk_ebnfdec_t *dec,char *s,int bytes)
{
	int ret;
	wtk_strbuf_t *buf;

	wtk_egram_reset(dec->egram);
	buf=wtk_strbuf_new(256,1);
	wtk_strbuf_push_s(buf,"(\\<s\\> ( ");
	wtk_strbuf_push(buf,s,bytes);
	wtk_strbuf_push_s(buf," ) \\<\\/s\\> )");
	//wtk_debug("[%.*s]\n",buf->pos,buf->data);
	ret=wtk_egram_ebnf2fst(dec->egram,buf->data,buf->pos);
	if(ret!=0){goto end;}
	ret=wtk_egram_dump(dec->egram,dec->net);
	if(ret!=0){goto end;}
	dec->dec->custom_net=dec->net;
end:
	wtk_strbuf_delete(buf);
	//exit(0);
	return ret;
}

void wtk_ebnfdec_reset(wtk_ebnfdec_t *dec)
{
	wtk_fst_net_reset(dec->net);
	wtk_wfstdec_reset(dec->dec);
	if(dec->vad)
	{
		wtk_vad_reset(dec->vad);
	}
}


int wtk_ebnfdec_start(wtk_ebnfdec_t *dec)
{
	int ret;

	if(dec->vad)
	{
		ret=wtk_vad_start(dec->vad);
		if(ret!=0){goto end;}
	}
	ret=wtk_wfstdec_start(dec->dec);
	if(ret!=0){goto end;}
	wtk_string_set(&(dec->dec->env.sep),0,0);
end:
	return ret;
}

int wtk_ebnfdec_feed(wtk_ebnfdec_t *dec,char *data,int bytes,int is_end)
{
	wtk_queue_node_t *qn;
	wtk_vframe_t *vf;

	if(dec->vad)
	{
		wtk_vad_feed(dec->vad,data,bytes,is_end);
		while(1)
		{
			qn=wtk_queue_pop(&(dec->vad_q));
			if(!qn){break;}
			vf=data_offset2(qn,wtk_vframe_t,q_n);
			if(vf->state!=wtk_vframe_sil)
			{
				wtk_wfstdec_feed(dec->dec,(char*)vf->wav_data,vf->frame_step*2,0);
			}
		}
		if(is_end)
		{
			wtk_wfstdec_feed(dec->dec,0,0,1);
		}
	}else
	{
		wtk_wfstdec_feed(dec->dec,data,bytes,is_end);
		//wtk_fst_decoder_print_path(dec->dec);
	}
	return 0;
}

wtk_string_t wtk_ebnfdec_get_result(wtk_ebnfdec_t *dec)
{
	wtk_string_t v;

	wtk_string_set(&(v),0,0);
	wtk_wfstdec_get_result(dec->dec,&(v));
	return v;
}
