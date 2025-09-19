#include "wtk_mer_wav.h"
#include "tts-mer/syn/wtk_mer_thread.h"
#include "tts-mer/sptk/wtk_sptk_tool.h"
#include "tts-mer/world/synth.h"

void wtk_mer_wav_post_filter( double *sintabl, wtk_matf_t *mf, float pf_coef, float fw_coef, float co_coef, int fl_coef)
{
    int row = mf->row
      , col = mf->col
      , dim = col
      , i
      , len = col*row
      , fre_M = 511
      , fre_len = (fre_M+1)*row
      , c2a_m = fre_M;
    wtk_heap_t *heap = wtk_heap_new(4096);
    float *weight = wtk_heap_malloc(heap, dim*sizeof(float))
        , *p = mf->p;
    char buf[100];
    
    weight[0] = weight[1] = 1.0;
    for (i=2; i<dim; i++) {
        weight[i] = pf_coef;
    }

    float *fre_p
        , *vop_p;
    /* 
    SPTK-3.9/freqt -m 59 -a 0.58 -M 511 -A 0 < arctic_a0056.mgc | SPTK-3.9/c2acr -m 511 -M 0 -l 1024 > arctic_a0056.mgc_r0
     */
    int r0_len = row;
    float *r0 = wtk_heap_malloc( heap, r0_len*sizeof(float));
    fre_p = wtk_heap_malloc( heap, fre_len*sizeof(float));
    sprintf(buf, "-m 59 -a 0.58 -M %d -A 0", fre_M);
    wtk_sptk_freqt(buf, p, len, fre_p);

    sprintf(buf, "-m %d -M 0 -l 1024", c2a_m);
    wtk_sptk_c2acr( sintabl, buf, fre_p, fre_len, r0);

    /* 
    SPTK-3.9/vopr -m -n 59 < arctic_a0056.mgc weight | SPTK-3.9/freqt -m 59 -a 0.58 -M 511 -A 0 | SPTK-3.9/c2acr -m 511 -M 0 -l 1024 > arctic_a0056.mgc_p_r0
     */
    int p_r0_len = row;
    float *p_r0 = wtk_heap_malloc( heap, p_r0_len*sizeof(float));
    vop_p = wtk_heap_malloc( heap, len*sizeof(float));
    fre_p = wtk_heap_malloc( heap, fre_len*sizeof(float));
    wtk_sptk_vopr("-m -n 59", p, len, weight, dim, vop_p);
    wtk_sptk_freqt("-m 59 -a 0.58 -M 511 -A 0", vop_p, len, fre_p);
    wtk_sptk_c2acr( sintabl, "-m 511 -M 0 -l 1024", fre_p, fre_len, p_r0);

    /* 
    SPTK-3.9/vopr -m -n 59 < arctic_a0056.mgc weight | SPTK-3.9/mc2b -m 59 -a 0.58 | SPTK-3.9/bcp -n 59 -s 0 -e 0 > arctic_a0056.mgc_b0
     */
    int b0_len = row;
    float *b0 = wtk_heap_malloc( heap, b0_len*sizeof(float))
        , *mc_p = wtk_heap_malloc( heap, len*sizeof(float));
    wtk_sptk_vopr("-m -n 59", p, len, weight, dim, vop_p);
    wtk_sptk_mc2b("-m 59 -a 0.58", vop_p, len, mc_p);
    wtk_sptk_bcp("-n 59 -s 0 -e 0", mc_p, sizeof(float)*len, b0);


    /* 
    SPTK-3.9/vopr -d < arctic_a0056.mgc_r0 arctic_a0056.mgc_p_r0 | SPTK-3.9/sopr -LN -d 2 | SPTK-3.9/vopr -a arctic_a0056.mgc_b0 > arctic_a0056.mgc_p_b0    
     */
    int p_b0_len = row;
    float *p_b0 = wtk_heap_malloc( heap, row*sizeof(float))
        , *r0_p_r0 = wtk_heap_malloc( heap, row*sizeof(float));
    wtk_sptk_vopr("-d", r0, r0_len, p_r0, p_r0_len, r0_p_r0);
    wtk_sptk_sopr("-LN -d 2", r0_p_r0, p_b0_len);
    wtk_sptk_vopr("-a",  b0, b0_len, r0_p_r0, p_b0_len, p_b0);

    /* 
    SPTK-3.9/vopr -m -n 59 < arctic_a0056.mgc weight | SPTK-3.9/mc2b -m 59 -a 0.58 |
    SPTK-3.9/bcp -n 59 -s 1 -e 59 | 
    SPTK-3.9/merge -n 58 -s 0 -N 0 arctic_a0056.mgc_p_b0 | SPTK-3.9/b2mc -m 59 -a 0.58 > arctic_a0056.mgc_p_mgc
     */
    int bcp5_len = row*59;
    float *bcp5 = wtk_heap_malloc( heap, bcp5_len*sizeof(float))
        , *merge5 = wtk_heap_malloc( heap, len*sizeof(float));
    wtk_sptk_vopr("-m -n 59", p, len, weight, dim, vop_p);
    wtk_sptk_mc2b("-m 59 -a 0.58", vop_p, len, mc_p);
    wtk_sptk_bcp("-n 59 -s 1 -e 59", mc_p, len*sizeof(float), bcp5);
    wtk_sptk_merge("-n 58 -s 0 -N 0", bcp5, bcp5_len*sizeof(float), p_b0, p_b0_len*sizeof(float), merge5);
    wtk_sptk_b2mc("-m 59 -a 0.58", merge5, len, p);

    wtk_heap_delete(heap);
}

wtk_matf_t* wtk_mer_process_lf0(wtk_matf_t *mf)
{/*
sopr -magic -1.0E+10 -EXP -MAGIC 0.0 arctic_a0056.lf0 | x2x +fd > arctic_a0056.f0
  */
    int len = mf->row*mf->col;
    float *p = mf->p;
    wtk_matf_t *f0= wtk_matf_new(mf->row, mf->col);

    wtk_sptk_sopr("-magic -1.0E+10 -EXP -MAGIC 0.0", p, len);
    memcpy(f0->p, p, sizeof(*p)*len);
    return f0;
}

wtk_matf_t* wtk_mer_process_bap(wtk_matf_t *mf)
{/* 
sopr -c 0 arctic_a0056.bap | x2x +fd > arctic_a0056.ap
 */
    int len = mf->row*mf->col;
    float *p = mf->p;
    wtk_matf_t *ap= wtk_matf_new(mf->row, mf->col);

    wtk_sptk_sopr("-c 0", p, len);
    memcpy(ap->p, p, sizeof(*p)*len);
    return ap;
}

wtk_matf_t* wtk_mer_process_mgc( double *sintbl, wtk_matf_t *mf)
{/* 
mgc2sp -a 0.58 -g 0 -m 59 -l 1024 -o 2 arctic_a0056.mgc_p_mgc | sopr -d 32768.0 -P | x2x +fd > arctic_a0056.sp 
*/
    int len = mf->row*mf->col 
      , l = 1024
      , sp_len;
    float *p = mf->p, *sp;
    wtk_matf_t *sp_mf = wtk_matf_new(mf->row, l/2+1);
    sp = sp_mf->p;
    sp_len = sp_mf->row*sp_mf->col;

    wtk_sptk_mgc2sp( sintbl, "-a 0.58 -g 0 -m 59 -l 1024 -o 2", p, len, sp);
    wtk_sptk_sopr("-d 32768.0 -P", sp, sp_len);
    return sp_mf;
}

size_t wtk_mer_generate_wav(wtk_mer_wav_param_t *wparam, wtk_mer_wav_t *hash, wtk_mer_wav_stream_t *wav)
{/* 
WORLD/synth 1024 16000 arctic_a0056.f0 arctic_a0056.sp arctic_a0056.ap arctic_a0056.wav
 */
    wtk_matf_t 
        *lf0_mf,
        *bap_mf,
        *mgc_mf,
        *f0_mf,
        *ap_mf,
        *sp_mf;
    int f0_len;
    double *sintbl = wparam->sintbl;
    wtk_rfft_t *rf = wparam->rf;
    float *win = wparam->win;
    
    lf0_mf = hash->lf0;
    bap_mf = hash->bap;
    mgc_mf = hash->mgc;

    wtk_mer_wav_post_filter( sintbl, mgc_mf, 1.4, 0.58, 511, 1024);
    //wtk_mer_matf_write_file(lf0_mf, "output/gen.lf0", 0);
    //wtk_mer_matf_write_file(bap_mf, "output/gen.bap", 0);
    //wtk_mer_matf_write_file(mgc_mf, "output/gen.mgc", 0);
//    wtk_matf_t *lf0= wtk_matf_new(1107, 1);
//    wtk_matf_t *bap= wtk_matf_new(1107, 5);
//    wtk_matf_t *mgc= wtk_matf_new(1107, 25);
//    wtk_mer_matf_read_file2(lf0, "/home/dm/work/project/mywork/tts-dmd-20170411/f.lf02", 0);
//    wtk_mer_matf_read_file2(bap, "/home/dm/work/project/mywork/tts-dmd-20170411/f.bap2", 0);
//    wtk_mer_matf_read_file2(mgc, "/home/dm/work/project/mywork/tts-dmd-20170411/f.mgc2", 0);
    //exit(0);
    double tx1=0, tx2=0,tx3,tx4;
    tx1=time_get_ms();
//    f0_mf = wtk_mer_process_lf0(lf0);
//    ap_mf = wtk_mer_process_bap(bap);
//    sp_mf = wtk_mer_process_mgc(sintbl, mgc);
    f0_mf = wtk_mer_process_lf0(lf0_mf);
    ap_mf = wtk_mer_process_bap(bap_mf);
    sp_mf = wtk_mer_process_mgc(sintbl, mgc_mf);
    tx3=time_get_ms();
    wtk_debug("process tx3-tx1=%f\n",tx3-tx1);
    if (wtk_mer_thread_wav_process(NULL, NULL, &f0_mf, &ap_mf, &sp_mf)!=0) {return 0;}
    // wtk_mer_matf_write_file(f0_mf, "output/f0.bin", 1);
    // wtk_mer_matf_write_file(ap_mf, "output/ap.bin", 1);
    // wtk_mer_matf_write_file(sp_mf, "output/sp.bin", 1);
    
    f0_len = f0_mf->row*f0_mf->col;
    wtk_mer_wav_stream_setlen(wav, f0_len);
    tx4=time_get_ms();
    wtk_debug("sp_mf row=%d col=%d\n", sp_mf->row, sp_mf->col);
    wtk_world_synth( rf, win, 1024, wav->fs, wav->frame_period, 
        f0_mf->p, f0_len,
        sp_mf->p, sp_mf->row*sp_mf->col,
        ap_mf->p, ap_mf->row*ap_mf->col,
        NULL, wav->data);
    tx2=time_get_ms();
    wtk_debug("process tx2-tx1 =%f\n",tx2-tx1);
    wtk_debug("process tx2-tx4 =%f\n",tx2-tx4);
    wtk_matf_delete(f0_mf);
    wtk_matf_delete(ap_mf);
    wtk_matf_delete(sp_mf);
    return wav->len;
}

wtk_mer_wav_param_t* wtk_mer_wav_param_new(int fft_size, int fs)
{
    int win_len = ((int)(fs*0.005)) * 2 -1;
    wtk_mer_wav_param_t *p = malloc(sizeof(*p));
    p->sintbl = wtk_sptk_getsintbl(fft_size);
    p->rf = wtk_rfft_new(fft_size/2);
    p->win = wtk_nn_hanning(win_len, 0);
    p->win_len = win_len;
    return p;
}
void wtk_mer_wav_param_delete(wtk_mer_wav_param_t *p)
{
    free(p->sintbl);
    free(p->win);
    wtk_rfft_delete(p->rf);
    free(p);
}

wtk_mer_wav_t* wtk_mer_wav_new(wtk_heap_t *heap)
{
    wtk_mer_wav_t *wav = wtk_heap_malloc(heap, sizeof(wtk_mer_wav_t));
    wav->heap=heap;
    return wav;
}
void wtk_mer_wav_delete(wtk_mer_wav_t *v)
{
    wtk_heap_delete(v->heap);
}
void wtk_mer_wav_add(wtk_mer_wav_t *wav, char *fea, wtk_matf_t *v)
{
    if(strncmp(fea, "lf0", 3)==0) {
        wav->lf0=v; 
    } else if (strncmp(fea, "mgc", 3)==0) {
        wav->mgc=v;
    } else if (strncmp(fea, "bap", 3)==0) {
        wav->bap=v;
    } else {
        wtk_debug("找不到匹配值\n");
        exit(1);
    }
}
