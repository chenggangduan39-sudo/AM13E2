#include "wtk_mer_engtts_api.h"
#include "tts-mer/syn/wtk_mer_sound.h"
/* 使用教程
new
start
delete
 */
wtk_mer_engtts_t* wtk_mer_engtts_new(char *cfg_fn, int is_rbin, int seek_pos, void* callback, void *param)
{
    wtk_mer_engtts_t *eng = wtk_malloc(sizeof(wtk_mer_engtts_t));
    eng->tts = wtk_mer_tts_new(cfg_fn, is_rbin, seek_pos);
    eng->callback = callback;
    eng->param = param;
    eng->is_stop = 0;
    return eng;
}

wtk_mer_wav_stream_t* wtk_mer_engtts_start(wtk_mer_engtts_t *eng, char *txt)
{
    wtk_heap_t *heap = wtk_heap_new(4096);
    wtk_mer_tts_t *tts = eng->tts;
    wtk_tts_parser_t *parser = tts->parser;
    wtk_tts_lab_t *lab;
    
    eng->is_stop=0;
    wtk_world_sound_set_allow(1);
    wtk_mer_tts_reset(tts);
    wtk_tts_parser_process(parser, txt, strlen(txt));
    lab=parser->lab;
    {
        // int wav_len=0;
        float rho=0.86, diff=0;
		int i,j,j2,k
          , lab_len=500
          , lab_i
          , phn_len
          , idx[2]
          , count =0
          , dur, durs, dure;
        wtk_tts_snt_t **snt,*s;
		wtk_tts_xphn_t **phns,*phn;
        wtk_strbuf_t
            ***lab_arrs = wtk_heap_malloc( heap, sizeof(wtk_strbuf_t**)*lab_len),
            **str_arr;
        wtk_syn_t *syn = tts->syn_tts;
        wtk_syn_hmm_lc_t *lc;

        for (i=0; i<lab_len; ++i)
        {
            lab_arrs[i] = wtk_heap_malloc( heap, sizeof(wtk_strbuf_t*)*4);
            for (j=0; j<4; j++)
            {
                lab_arrs[i][j] = wtk_strbuf_heap_new(heap, 256, 1);
            }
        }

		snt=(wtk_tts_snt_t**)lab->snts->slot;
        for(i=0;i<lab->snts->nslot;++i)
		{
			s=snt[i];
			if(!s->phns){continue;}
            phns=(wtk_tts_xphn_t**)s->phns->slot;
            lab_i=0;
            phn_len=s->phns->nslot;
            dur=durs=dure=0;
            j2=0;
			for(j=0;j<phn_len;++j)
			{
                if (i==0 && j ==0 ) {/* 移除首句静音 */continue;}
                // if ( j == 0 ) {/* 移除头部静音 */continue;}
				phn=phns[j];
                // printf("%.*s\n", phn->lab->len, phn->lab->data);
                k = 0;
                lc=wtk_syn_hmm_lc_new(syn->cfg->hmm, syn->heap,phn->lab->data,phn->lab->len);
                lc->phn=phn;
                wtk_syn_dtree_search(syn->cfg->tree,WTK_SYN_DTREE_TREE_DUR,2,phn->lab->data,phn->lab->len, idx);
		        wtk_syn_hmm_lc_find_durpdf(lc, rho, idx, &diff);

                while (k<=4)
                {
                    str_arr = lab_arrs[lab_i];
                    durs = dure;
                    dure = durs + lc->dur[k];
                    str_arr[1]->pos = durs;
                    str_arr[2]->pos = dure;
                    sprintf(str_arr[3]->data, "%.*s[%d]", phn->lab->len, phn->lab->data, k+2);
                    str_arr[3]->pos = phn->lab->len+3;
                    // printf("%d %d %.*s\n", durs, dure, str_arr[3]->pos, str_arr[3]->data);
                    lab_i++;
                    k++;
                }
                dur+=lc->totaldur;
                j2++;
                // if (j>0 && ((j2 >= 12 && phn->bound == WTK_TTS_BOUND_PHASE) || j==(phn_len-1)))
                if (j>0 && (j2 == 12 || j==(phn_len-1)))
                {
                    wtk_mer_process_thread(NULL, lab_arrs, lab_i, tts);
                    // wtk_mer_process(NULL, lab_arrs, lab_i, tts);
                    lab_i=0;
                    wtk_mer_wav_stream_savefile2(tts->wav, "output/test.wav", count);
                    if (eng->is_stop)  { eng->is_stop=0; goto end; }
                    if (eng->callback) {eng->callback(tts->wav, eng->param);}
                    // char buf[512];
                    // sprintf(buf, "output/test.a.%d.wav", count);
                    // if (count==0)
                    // {wtk_mer_wav_stream_savefile(tts->wav, buf, 0);wtk_exit(1);}
                    // tts->wav->data = wav_old_data;
                    count++;
                    j2=0;
                    // if (count>3) { wtk_exit(1); }
                }
			}
            wtk_world_sound_reset();
            // wtk_mer_process_thread(NULL, lab_arrs, lab_i, tts);
            // wtk_mer_process(NULL, lab_arrs, lab_i, tts);
            // if (eng->callback) {eng->callback(tts->wav, eng->param);}
            // wtk_mer_wav_stream_savefile2(tts->wav, "output/test.wav", count);
            // count++;
            // break;
		}
    }
end:
    #ifdef USE_MKL
    // mkl_thread_free_buffers();/* 仅释放当前线程中mkl的内存 */
    mkl_free_buffers(); /* 释放全部mkl内存 */
    #endif
    wtk_world_sound_set_allow(0);
    wtk_heap_delete(heap);
    return tts->wav;
}
void wtk_mer_engtts_stop(wtk_mer_engtts_t *eng)
{
    eng->is_stop = 0;
}
void wtk_mer_engtts_delete(wtk_mer_engtts_t *eng)
{
    wtk_mer_tts_delete(eng->tts);
    wtk_free(eng);
}
