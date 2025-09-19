#include "wtk_ebnfdec2.h" 
#include "wtk/asr/wfst/wtk_wfstdec.h"

wtk_ebnfdec2_t* wtk_ebnfdec2_new(wtk_ebnfdec2_cfg_t *cfg)
{
	wtk_ebnfdec2_t *dec;

	dec=(wtk_ebnfdec2_t*)wtk_malloc(sizeof(wtk_ebnfdec2_t));
	dec->cfg=cfg;
	if(cfg->use_bin)
	{
		dec->dec=wtk_wfstdec_new(cfg->dec_bin_cfg);
	}else
	{
		dec->dec=wtk_wfstdec_new((wtk_wfstdec_cfg_t*)(cfg->dec_cfg->cfg));
	}
	return dec;
}

void wtk_ebnfdec2_delete(wtk_ebnfdec2_t *dec)
{
	wtk_wfstdec_delete(dec->dec);
	wtk_free(dec);
}

int wtk_ebnfdec2_start(wtk_ebnfdec2_t *dec)
{
	return  wtk_wfstdec_start(dec->dec);
}

void wtk_ebnfdec2_reset(wtk_ebnfdec2_t *dec)
{
	wtk_wfstdec_reset(dec->dec);
}

int wtk_ebnfdec2_feed(wtk_ebnfdec2_t *dec,char *data,int bytes,int is_end)
{
	return wtk_wfstdec_feed(dec->dec,data,bytes,is_end);
}

wtk_string_t wtk_ebnfdec2_get_result(wtk_ebnfdec2_t *dec)
{
	wtk_string_t v;

	wtk_string_set(&(v),0,0);
	wtk_wfstdec_get_result(dec->dec,&(v));
	return v;
}
