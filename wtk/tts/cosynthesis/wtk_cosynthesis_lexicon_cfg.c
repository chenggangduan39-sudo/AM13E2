#include "wtk_cosynthesis_lexicon_cfg.h"

int wtk_cosynthesis_feat_cfg_init(wtk_cosynthesis_feat_cfg_t *cfg)
{
    cfg->dur_len = 1;
    cfg->lf0_len = 1;
    cfg->spec_len = 25;
    cfg->hmm_dur_len =5;
    cfg->hmm_lf0_len = 1;
    cfg->hmm_mcep_len = 25;
    cfg->spec_llen = 42;

    return 0;
}

int wtk_cosynthesis_feat_cfg_update_local(wtk_cosynthesis_feat_cfg_t *cfg,wtk_local_cfg_t *lc)
{
    wtk_string_t *v;
    wtk_local_cfg_update_cfg_i(lc, cfg, dur_len, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, lf0_len, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, spec_len, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, hmm_dur_len, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, hmm_lf0_len, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, hmm_mcep_len, v);
    return 0;
}

int wtk_cosynthesis_lexicon_cfg_init(wtk_cosynthesis_lexicon_cfg_t *cfg)
{
    wtk_cosynthesis_feat_cfg_init(&cfg->feat_cfg);
    cfg->snt_fn = NULL;
    cfg->word_fn = NULL;
    cfg->unit_fn = NULL;
    cfg->wrds = NULL;
    cfg->snts = NULL;
    cfg->max_edit_distance=3;
    cfg->nsnt_wrdlen = 0;
    cfg->snt_wrdlen_split_idx = NULL;
    cfg->use_snt_wrdlen_split_idx = 0;
    cfg->sil_id = 7;
    cfg->samples = 16000;
    cfg->g_audio_res=NULL;
    cfg->g_feat_res=NULL;
    cfg->g_snt_res=NULL;
    cfg->g_wrd_res=NULL;

    cfg->g_audio_res_fp=NULL;
    cfg->g_feat_res_fp=NULL;
    cfg->g_snt_res_fp=NULL;
    cfg->g_wrd_res_fp=NULL;

    cfg->use_fp = 0;

    cfg->maxn_wrds=30;
    cfg->rbin=NULL;

    return 0;
}

int wtk_cosynthesis_lexicon_cfg_update_local(wtk_cosynthesis_lexicon_cfg_t *cfg,wtk_local_cfg_t *lc)
{
    wtk_string_t *v;

    wtk_local_cfg_update_cfg_i(lc, cfg, maxn_wrds, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, sil_id, v);
    wtk_local_cfg_update_cfg_i(lc, cfg, samples, v);

    wtk_local_cfg_update_cfg_str(lc, cfg, word_fn, v);
    wtk_local_cfg_update_cfg_str(lc, cfg, snt_fn, v);
    wtk_local_cfg_update_cfg_str(lc, cfg, unit_fn, v);
    wtk_local_cfg_update_cfg_str(lc, cfg, feat_fn, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_snt_wrdlen_split_idx, v);
    wtk_local_cfg_update_cfg_b(lc, cfg, use_fp, v);
    lc=wtk_local_cfg_find_lc_s(lc,"feat");
    if(lc)
    {
        wtk_cosynthesis_feat_cfg_update_local(&cfg->feat_cfg,lc);
    }
    return 0;
}

int wtk_cosynthesis_lexicon_wrds_unit_clean(wtk_unit_t *unit)
{
    return 0;
}

int wtk_cosynthesis_lexicon_cfg_clean(wtk_cosynthesis_lexicon_cfg_t *cfg)
{
    int i;
    if(cfg->snt_wrdlen_split_idx)
    {
        wtk_free(cfg->snt_wrdlen_split_idx);
    }
    if(cfg->snts)
    {
    	if (cfg->use_fp)
    	{
            for(i=0;i<cfg->nsnts;++i)
            {
            	wtk_free(cfg->snts[i].raw_word_ids);
            	wtk_free(cfg->snts[i].nosil_word_ids);
            	wtk_free(cfg->snts[i].unit_ids);
            }
    	}
        wtk_free(cfg->snts);
    }
    if(cfg->wrds)
    {
        for(i=0;i<cfg->nwrds;++i)
        {
        	if (cfg->use_fp)
        	{
        		wtk_free(cfg->wrds[i].word);
        	}
        	else
        	{
                if(cfg->wrds[i].unit)
                {
                    wtk_free(cfg->wrds[i].unit);
                }
        	}
        }
        wtk_free(cfg->wrds);
    }
    if (cfg->g_audio_res)
    	wtk_free(cfg->g_audio_res);
    if(cfg->g_feat_res)
    	wtk_free(cfg->g_feat_res);
    if(cfg->g_snt_res)
    	wtk_free(cfg->g_snt_res);
    if(cfg->g_wrd_res)
    	wtk_free(cfg->g_wrd_res);

    if(cfg->g_audio_res_fp)
    	fclose(cfg->g_audio_res_fp);
    if(cfg->g_feat_res_fp)
    	fclose(cfg->g_feat_res_fp);
    if(cfg->g_snt_res_fp)
    	fclose(cfg->g_snt_res_fp);
    if(cfg->g_wrd_res_fp)
    	fclose(cfg->g_wrd_res_fp);

    return 0;
}

int wtk_cosynthesis_lexicon_cfg_update_info(wtk_cosynthesis_lexicon_cfg_t *cfg,
                                        unsigned char *g_snt_res,
                                        unsigned char *g_wrd_res,
                                        unsigned char *g_audio_res,
                                        unsigned char *g_feat_res)
{
    int i,j,l;
    cfg->nsnts = *(int*)g_snt_res;
    //wtk_debug("snt len=%d\n",cfg->nsnts);
    g_snt_res+=sizeof(int);
    cfg->snts = (wtk_snt_t*)wtk_malloc(sizeof(wtk_snt_t)*cfg->nsnts );
    for(i=0;i<cfg->nsnts;++i)
    {
        cfg->snts[i].raw_word_cnt = (uint8_t)(*g_snt_res);
        g_snt_res+=sizeof(uint8_t);
        cfg->snts[i].nosil_word_cnt = (uint8_t)(*g_snt_res);
        g_snt_res+=sizeof(uint8_t);
        cfg->snts[i].raw_word_ids = (uint16_t*)(g_snt_res);
        l = cfg->snts[i].raw_word_cnt * sizeof(uint16_t);
        g_snt_res+=l;
        cfg->snts[i].unit_ids = (uint16_t*)(g_snt_res);
        g_snt_res+=l;
        cfg->snts[i].nosil_word_ids = (uint16_t*)(g_snt_res);
        l = cfg->snts[i].nosil_word_cnt * sizeof(uint16_t);
        g_snt_res+=l;

    }
    cfg->nwrds = *(int*)g_wrd_res;
    //wtk_debug("nwrds=%d\n",cfg->nwrds);
    g_wrd_res+=sizeof(int);
    cfg->wrds = (wtk_word_t*)wtk_malloc(sizeof(wtk_word_t)*cfg->nwrds);
    for(i=0;i<cfg->nwrds;++i)
    {
        cfg->wrds[i].word_len = (uint8_t)(*g_wrd_res);
        g_wrd_res+=sizeof(uint8_t);
        // wtk_debug("words len=%d\n",cfg->wrds[i].word_len);
        cfg->wrds[i].word = (char*)(g_wrd_res);
        // wtk_debug("%.*s\n",cfg->wrds[i].word_len,cfg->wrds[i].word);
        g_wrd_res+=cfg->wrds[i].word_len;
        cfg->wrds[i].word_id = i;
        // cfg->wrds[i].word_id = (uint16_t)(*g_wrd_res);
        // g_wrd_res+=2;
        cfg->wrds[i].nunit = *(uint16_t*)(g_wrd_res);
        g_wrd_res+=sizeof(uint16_t);
        //wtk_debug("nunit=%d\n",cfg->wrds[i].nunit);
        cfg->wrds[i].unit = (wtk_unit_t*)wtk_malloc(sizeof(wtk_unit_t)*cfg->wrds[i].nunit);
        memset(cfg->wrds[i].unit,0,sizeof(wtk_unit_t)*cfg->wrds[i].nunit);
        //wtk_debug("wrd[%d]:%p\n", i, cfg->wrds[i].unit);
        for(j=0;j<cfg->wrds[i].nunit;++j)
        {
            cfg->wrds[i].unit[j].unit_id = j;
            cfg->wrds[i].unit[j].data_len = *(uint16_t*)(g_audio_res);
            g_audio_res+=2;
            cfg->wrds[i].unit[j].raw_audio_len = *(uint16_t*)(g_audio_res);
            g_audio_res+=2;
            cfg->wrds[i].unit[j].is_compress = *(uint8_t*)(g_audio_res);
            g_audio_res += 1;
            cfg->wrds[i].unit[j].shifit = *(uint16_t*)(g_audio_res);;
            g_audio_res += 2;
            cfg->wrds[i].unit[j].data = (char*)(g_audio_res);
            g_audio_res += cfg->wrds[i].unit[j].data_len;

            if(strncmp(cfg->wrds[i].word,"pau",3)==0)
            {
                continue;
            }
            cfg->wrds[i].unit[j].nphone = *(int*)(g_feat_res);
            // wtk_debug("nphone=%d\n",cfg->wrds[i].unit[j].nphone);
            g_feat_res+=sizeof(int);
            cfg->wrds[i].unit[j].spec = (float*)(g_feat_res);
            g_feat_res+=cfg->wrds[i].unit[j].nphone * cfg->feat_cfg.spec_len * sizeof(float);

            cfg->wrds[i].unit[j].lf0 = (float*)(g_feat_res);
            g_feat_res+=cfg->wrds[i].unit[j].nphone * cfg->feat_cfg.lf0_len * sizeof(float);
            // print_float(cfg->wrds[i].unit[j].lf0,cfg->wrds[i].unit[j].nphone);

            cfg->wrds[i].unit[j].dur = (float*)(g_feat_res);
            g_feat_res+=cfg->wrds[i].unit[j].nphone * cfg->feat_cfg.dur_len*  sizeof(float);
            // print_float(cfg->wrds[i].unit[j].dur,cfg->wrds[i].unit[j].nphone);

            cfg->wrds[i].unit[j].kld_lf0 = (float*)(g_feat_res);
            g_feat_res+=cfg->wrds[i].unit[j].nphone * sizeof(float);

            cfg->wrds[i].unit[j].spec_idx = (int*)(g_feat_res);
            g_feat_res+=cfg->wrds[i].unit[j].nphone * sizeof(int);
            // print_int( cfg->wrds[i].unit[j].spec_idx,cfg->wrds[i].unit[j].nphone);

            cfg->wrds[i].unit[j].lf0_idx = (int*)(g_feat_res);
            g_feat_res+=cfg->wrds[i].unit[j].nphone * sizeof(int);

            cfg->wrds[i].unit[j].dur_idx = (int*)(g_feat_res);
            g_feat_res+=cfg->wrds[i].unit[j].nphone * sizeof(int);
            // print_int( cfg->wrds[i].unit[j].dur_idx,cfg->wrds[i].unit[j].nphone);
        }
    }
    return 0;
}

int wtk_cosynthesis_lexicon_cfg_update_file(wtk_cosynthesis_lexicon_cfg_t *cfg)
{
    int i,j,l, nphn;
    uint16_t datalen;

    if (cfg->g_snt_res_fp)
    {
        //snt info
        fread(&(cfg->nsnts), sizeof(int), 1, cfg->g_snt_res_fp);
        //wtk_debug("snt len=%d\n",cfg->nsnts);
        cfg->snts = (wtk_snt_t*)wtk_malloc(sizeof(wtk_snt_t)*cfg->nsnts);
        for(i=0;i<cfg->nsnts;++i)
        {
        	//raw_word_cnt
            fread(&(cfg->snts[i].raw_word_cnt), sizeof(uint8_t), 1, cfg->g_snt_res_fp);
            //nosil word cnt
            fread(&(cfg->snts[i].nosil_word_cnt), sizeof(uint8_t), 1, cfg->g_snt_res_fp);
            //raw word ids list
            l=cfg->snts[i].raw_word_cnt;
            cfg->snts[i].raw_word_ids = (uint16_t*)wtk_malloc(l * sizeof(uint16_t));
            fread(cfg->snts[i].raw_word_ids, sizeof(uint16_t), l, cfg->g_snt_res_fp);
            //raw unit ids list of raw words list
            cfg->snts[i].unit_ids = (uint16_t*)wtk_malloc(l * sizeof(uint16_t));
            fread(cfg->snts[i].unit_ids, sizeof(uint16_t), l, cfg->g_snt_res_fp);
            //nosil word ids list
            l = cfg->snts[i].nosil_word_cnt;
            cfg->snts[i].nosil_word_ids = (uint16_t*)wtk_malloc(l * sizeof(uint16_t));
            fread(cfg->snts[i].nosil_word_ids, l,sizeof(uint16_t), cfg->g_snt_res_fp);
        }
    }

    //words info
    fread(&(cfg->nwrds), sizeof(int), 1, cfg->g_wrd_res_fp);
    //wtk_debug("nwrds=%d\n",cfg->nwrds);
    cfg->wrds = (wtk_word_t*)wtk_calloc(cfg->nwrds, sizeof(wtk_word_t));
    for(i=0;i<cfg->nwrds;++i)
    {
        fread(&(cfg->wrds[i].word_len), sizeof(uint8_t), 1, cfg->g_wrd_res_fp);
        // wtk_debug("words len=%d\n",cfg->wrds[i].word_len);
        l=cfg->wrds[i].word_len;
        cfg->wrds[i].word = (char*)wtk_calloc(l+1, sizeof(char));
        fread(cfg->wrds[i].word, sizeof(char), l, cfg->g_wrd_res_fp);
        cfg->wrds[i].word_id = i;
        fread(&(cfg->wrds[i].nunit), sizeof(uint16_t), 1, cfg->g_wrd_res_fp);
        //wtk_debug("wrd=%.*s %p nunit=%d\n",cfg->wrds[i].word_len, cfg->wrds[i].word, &(cfg->wrds[i]), cfg->wrds[i].nunit);
        cfg->wrds[i].unit = NULL;
        cfg->wrds[i].unit_pos = ftell(cfg->g_audio_res_fp);
        cfg->wrds[i].feat_pos = ftell(cfg->g_feat_res_fp);
        //skip current word info.
        for(j=0;j<cfg->wrds[i].nunit;++j)
        {
        	// unit data len (current saved in corpus)
			fread(&datalen, sizeof(uint16_t), 1, cfg->g_audio_res_fp);
            l = sizeof(uint16_t) +         // raw unit data len
    				sizeof(uint8_t) +      // is compressed ?
    				sizeof(uint16_t)+      // shift for compressed
    				datalen;  // unit data
            fseek(cfg->g_audio_res_fp, l, SEEK_CUR);
            if(strncmp(cfg->wrds[i].word,"pau",3)==0)
            {
                continue;
            }

            //the following for no sil
            fread(&nphn, 1, sizeof(int), cfg->g_feat_res_fp);
            l = sizeof(float) +                                   //sil_prev_l
            		nphn *(
            		cfg->feat_cfg.spec_len * sizeof(float) +    //spec_data
					cfg->feat_cfg.lf0_len * sizeof(float) +     //lf0 data
					cfg->feat_cfg.dur_len*  sizeof(float) +     //dur data
					sizeof(float) +                             //kld_lf0 data
					sizeof(int) +                               //spec idx data
					sizeof(int) +                               //lf0 idx data
					sizeof(int)) +                               //dur idx data
			        cfg->feat_cfg.spec_llen*  sizeof(float) +     //spec_l data
			        cfg->feat_cfg.spec_llen*  sizeof(float);     //spec_r data
            fseek(cfg->g_feat_res_fp, l, SEEK_CUR);
        }
    }

    return 0;
}

int wtk_cosynthesis_lexicon_cfg_update(wtk_cosynthesis_lexicon_cfg_t *cfg)
{
    int ret = 0;
    FILE *f;
    int datasize;

    if(cfg->word_fn)
    {
        f = fopen(cfg->word_fn,"rb");
        if(!f)
        {
            printf("Open %s failed\n",cfg->word_fn);
            ret=-1;goto end;
        }
        if (cfg->use_fp)
        	cfg->g_wrd_res_fp=f;
        else{
            fseek(f, 0, SEEK_END);
            datasize = ftell(f);
        	fseek(f, 0, SEEK_SET);
        	cfg->g_wrd_res = wtk_calloc(datasize, sizeof(unsigned char));
        	fread(cfg->g_wrd_res, 1, datasize, f);
            fclose(f);
        }
    }
    if(cfg->snt_fn)
    {
        f = fopen(cfg->snt_fn,"rb");
        if(!f)
        {
            printf("Open %s failed\n",cfg->snt_fn);
            ret=-1; goto end;
        }
        if (cfg->use_fp)
        	cfg->g_snt_res_fp=f;
        else
        {
            fseek(f, 0, SEEK_END);
            datasize = ftell(f);
        	fseek(f, 0, SEEK_SET);
        	cfg->g_snt_res = wtk_calloc(datasize, sizeof(unsigned char));
        	fread(cfg->g_snt_res, 1, datasize, f);
            fclose(f);
        }
    }
    if(cfg->unit_fn)
    {
        f = fopen(cfg->unit_fn,"rb");
        if(!f)
        {
            printf("Open %s failed\n",cfg->unit_fn);
        }
        if (cfg->use_fp)
        	cfg->g_audio_res_fp=f;
        else{
            fseek(f, 0, SEEK_END);
            datasize = ftell(f);
        	fseek(f, 0, SEEK_SET);
        	cfg->g_audio_res = wtk_calloc(datasize, sizeof(unsigned char));
        	fread(cfg->g_audio_res, 1, datasize, f);
        	fclose(f);
        }
    }
    if(cfg->feat_fn)
    {
        f = fopen(cfg->feat_fn,"rb");
        if(!f)
        {
            printf("Open %s failed\n",cfg->feat_fn);
            ret=-1;goto end;
        }
        if (cfg->use_fp)
        	cfg->g_feat_res_fp=f;
        else{
            fseek(f, 0, SEEK_END);
            datasize = ftell(f);
        	fseek(f, 0, SEEK_SET);
        	cfg->g_feat_res = wtk_calloc(datasize, sizeof(unsigned char));
        	fread(cfg->g_feat_res, 1, datasize, f);
            fclose(f);
        }
    }
    if (cfg->use_fp)
    	ret=wtk_cosynthesis_lexicon_cfg_update_file(cfg);
    else
    	ret=wtk_cosynthesis_lexicon_cfg_update_info(cfg,cfg->g_snt_res,cfg->g_wrd_res,cfg->g_audio_res,cfg->g_feat_res);

end:
    return ret;
}

int wtk_cosynthesis_lexicon_cfg_loadwrd(wtk_cosynthesis_lexicon_cfg_t *cfg, wtk_rbin2_t *rbin, char *fn)
{
	FILE *f;
	int i,l, ret=0;
	uint8_t word_len;

	if (rbin)
		f = wtk_rbin2_get_file(rbin, fn);
	else
	{
		f=fopen(fn,"rb");
		if(!f){
			wtk_debug("open failed %s", fn);
			ret=-1;
			goto end;
		}
	}

    if (f && cfg->use_fp)
    {
        //words info
        fread(&(cfg->nwrds), sizeof(int), 1, f);
        //wtk_debug("nwrds=%d\n",cfg->nwrds);
        cfg->wrds = (wtk_word_t*)wtk_calloc(cfg->nwrds, sizeof(wtk_word_t));
        for(i=0;i<cfg->nwrds;++i)
        {
            fread(&word_len, sizeof(uint8_t), 1, f);
            cfg->wrds[i].word_len = word_len;
            // wtk_debug("words len=%d\n",cfg->wrds[i].word_len);
            l=cfg->wrds[i].word_len;
            cfg->wrds[i].word = (char*)wtk_calloc(l+1, sizeof(char));
            fread(cfg->wrds[i].word, sizeof(char), l, f);
            cfg->wrds[i].word_id = i;
            fread(&(cfg->wrds[i].nunit), sizeof(uint16_t), 1, f);
        }
    }
end:
	if (f)
		fclose(f);
    return ret;
}

int wtk_cosynthesis_lexicon_cfg_loadunit(wtk_cosynthesis_lexicon_cfg_t *cfg, wtk_rbin2_t *rbin, char *fn)
{
	FILE *f;
	int i,j,l, ret=0;
	uint16_t datalen;

	if (rbin)
		f = wtk_rbin2_get_file(rbin, fn);
	else
	{
		f=fopen(fn,"rb");
		if(!f){
			wtk_debug("open failed %s", fn);
			ret=-1;
			goto end;
		}
	}
    if (f && cfg->use_fp)
    {
        //words info
        for(i=0;i<cfg->nwrds;++i)
        {
            cfg->wrds[i].unit = NULL;
            cfg->wrds[i].unit_pos = ftell(f);
            //skip current word info.
            for(j=0;j<cfg->wrds[i].nunit;++j)
            {
            	// unit data len (current saved in corpus)
    			fread(&datalen, sizeof(uint16_t), 1, f);
                l = sizeof(uint16_t) +         // raw unit data len
        				sizeof(uint8_t) +      // is compressed ?
        				sizeof(uint16_t)+      // shift for compressed
        				datalen;  // unit data
                fseek(f, l, SEEK_CUR);
            }
        }
    }
end:
	if(f)
		fclose(f);
    return ret;
}
int wtk_cosynthesis_lexicon_cfg_loadfeat(wtk_cosynthesis_lexicon_cfg_t *cfg, wtk_rbin2_t* rbin, char *fn)
{
	FILE *f;
	int i,j,l, nphn, ret=0;

	if (rbin)
		f = wtk_rbin2_get_file(rbin, fn);
	else
	{
		f=fopen(fn,"rb");
		if(!f){
			wtk_debug("open failed %s", fn);
			ret=-1;
			goto end;
		}
	}
    if (f && cfg->use_fp)
    {
        //words info
        for(i=0;i<cfg->nwrds;++i)
        {
        	cfg->wrds[i].feat_pos = ftell(f);
            //skip current word info.
            for(j=0;j<cfg->wrds[i].nunit;++j)
            {
                if(strncmp(cfg->wrds[i].word,"pau",3)==0)
                {
                    continue;
                }
                //the following for no sil
                fread(&nphn, 1, sizeof(int), f);
                l = sizeof(float) +                                   //sil_prev_l
                		nphn *(
                		cfg->feat_cfg.spec_len * sizeof(float) +    //spec_data
    					cfg->feat_cfg.lf0_len * sizeof(float) +     //lf0 data
    					cfg->feat_cfg.dur_len*  sizeof(float) +     //dur data
    					sizeof(float) +                             //kld_lf0 data
    					sizeof(int) +                               //spec idx data
    					sizeof(int) +                               //lf0 idx data
    					sizeof(int)) +                               //dur idx data
    			        cfg->feat_cfg.spec_llen*  sizeof(float) +     //spec_l data
    			        cfg->feat_cfg.spec_llen*  sizeof(float);     //spec_r data
                fseek(f, l, SEEK_CUR);
            }
        }
    }
end:
	if(f)
		fclose(f);
    return ret;
}

int wtk_cosynthesis_lexicon_cfg_update2(wtk_cosynthesis_lexicon_cfg_t *cfg,wtk_source_loader_t *sl)
{
    int ret = 0;

    cfg->rbin = sl->hook;
    if (cfg->snt_fn)
    {
    	//
    }
    if(cfg->word_fn)
    {
    	ret=wtk_cosynthesis_lexicon_cfg_loadwrd(cfg, sl->hook, cfg->word_fn);
    	if(ret!=0) goto end;
    }
    if(cfg->unit_fn)
    {
    	ret=wtk_cosynthesis_lexicon_cfg_loadunit(cfg, sl->hook, cfg->unit_fn);
    	if(ret!=0) goto end;
    }
    if(cfg->feat_fn)
    {
    	ret=wtk_cosynthesis_lexicon_cfg_loadfeat(cfg, sl->hook, cfg->feat_fn);
    	if(ret!=0) goto end;
    }

end:
    return ret;
}
