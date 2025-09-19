#include "tts-mer/wtk_mer_common.h"
#include "tts-mer/syn/wtk_mer_tts.h"
#include "tts-mer/syn/wtk_mer_thread.h"
#include "tts-mer/interface/wtk_mer_engtts_api.h"

static void wtk_mer_test()
{
    float f1 = 10.2, f2 = 15, sum;
    // sum = f1+f2;
    // f2 = f1;

    // __asm__ __volatile__(
    //     "movss %2, %%xmm0"
	//    "addss  %%xmm0, %1\n"
	//    :"=r"(sum)
	//    :"r"(f1),"r"(f2)
	// );
    // printf("sum=%f, f1=%f, f2=%f\n", sum, f1, f2);
    wtk_exit(0);
}

static int read_file_lines_call(wtk_larray_t *lines, wtk_source_t *src)
{
    wtk_strbuf_t *buf = wtk_strbuf_new(256, 1);
    wtk_string_t *v;
    int ret;
    while(1)
    {
        ret = wtk_mer_source_read_line(src, buf);
        if (ret!=0){ ret=0;goto end;}
        if (buf->pos==0) { continue;}
        v = wtk_string_dup_data( buf->data, buf->pos);
        wtk_larray_push2(lines, &v);
        // printf("[%.*s]\n", buf->pos, buf->data);
    }
    end:
        wtk_strbuf_delete(buf);
        return ret;
}

static void print_usage()
{
	printf("parser usgae:\n");
	printf("\t-c configure file\n");
	printf("\t-i input file\n");
	printf("\nExample:\n");
	printf("\t ./tool/tts -c cfg -i test.txt \n\n"
			"test.txt: no. text\n");
    exit(1);
}

static void test_tts_file(wtk_mer_tts_t *tts, char *s, int index, int cnt)
{
    wtk_heap_t *heap = wtk_heap_new(4096);
    wtk_tts_parser_t *parser = tts->parser;
    wtk_tts_lab_t *lab;
	wtk_strbuf_t *buf;
    int pos, fn_len=256;
    char wav_fn[fn_len];

    wtk_mer_tts_reset(tts);
    wtk_debug("[-%d-]\n", index);
	buf=wtk_strbuf_new(256,1);
	pos=wtk_str_str(s, strlen(s), " ", 1);

    wtk_debug("\"*/%.*s.lab\"\n", (int)strlen(s), s);
    wtk_strbuf_push(buf, s+pos+1, strlen(s)-pos-1);
	wtk_strbuf_strip(buf);
    wtk_tts_parser_process(parser, buf->data, buf->pos); /* 生成lab */
	lab=parser->lab;
    {
        float rho=0.86, diff=0;
		int i,j
          , count=0
          , lab_len=500
          , lab_i
          , phn_len
          , k, idx[2]
          , dur, durs, dure;
        char line[512];
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
            // wtk_string_print(lines[i]);
        }

		snt=(wtk_tts_snt_t**)lab->snts->slot;
        for(i=0;i<lab->snts->nslot;++i)
		{
			s=snt[i];
			if(!s->phns){continue;}
			//wtk_debug("===================> snt=%d\n ",s->type);么重要，那我们就是需要把它提取出来！我们要提取的不仅仅是共振峰的位置，还得提取它们转变的过程。所以我们提取的是频谱的包络（Spectral Envelope）。这包络就是一条连接这些共
			//wtk_debug("snt_type=%d nslot=%d\n",s->type,s->phns->nslot);
            phns=(wtk_tts_xphn_t**)s->phns->slot;
            lab_i=0;
            dur=durs=dure=0;
            phn_len=s->phns->nslot;
			for(j=0;j<phn_len;++j)
			{
				phn=phns[j];
                // if(j==0 && i==0){continue;}
                k = 0;
                lc=wtk_syn_hmm_lc_new(syn->cfg->hmm, syn->heap,phn->lab->data,phn->lab->len);
                lc->phn=phn;
                wtk_syn_dtree_search(syn->cfg->tree,WTK_SYN_DTREE_TREE_DUR,2,phn->lab->data,phn->lab->len, idx);
		        wtk_syn_hmm_lc_find_durpdf(lc, rho, idx, &diff);
                // printf("dur: %.*s\n", phn->phn->len, phn->phn->data);
                // printf("dur: %.*s %d %d\n", phn->phn->len,phn->phn->data, dur, lc->totaldur);
                // wtk_exit(1);
                while (k<=4)
                {
                    // memset(line, '\0', 256);
                    str_arr = lab_arrs[lab_i];
                    // str_arr[1]->pos = (j*5+k)*10;
                    // str_arr[2]->pos = (j*5+k+1)*10;
                    // str_arr[1]->pos = 0;
                    // str_arr[2]->pos = lc->dur[k];
                    durs = dure;
                    dure = durs + lc->dur[k];
                    str_arr[1]->pos = durs;
                    str_arr[2]->pos = dure;
                    sprintf(str_arr[3]->data, "%.*s[%d]", phn->lab->len, phn->lab->data, k+2);
                    str_arr[3]->pos = phn->lab->len+3;
                    // printf("%s\n",line);
                    // printf("%.*s\n", phn->lab->len, phn->lab->data);
                    lab_i++;
                    k++;
                    if (lab_i==lab_len)
                    { wtk_debug("句子可能过长, 初始化lab_arrs[%d]大一点\n", lab_len);wtk_exit(1);}
                }
                dur+=lc->totaldur;
                // if (j>0 && (j%20==0 || j==(phn_len-1)))
                // {
                //     sprintf(wav_fn, "output/test.%d.%d.wav", index, count);
                //     wtk_mer_process_thread(wav_fn, lab_arrs, lab_i, tts);
                //     // wtk_mer_process(wav_fn, lab_arrs, lab_i, tts);
                //     lab_i=0;
                //     count++;
                // }
			}
            // wtk_source_load_file( lab_lines, (wtk_source_load_handler_t) read_file_lines_call, "yinhang.dm.txt");
            // 您可以委托他人或公司服务人员办理,需要的应备资料可以致电我司客服热线95500咨询
            // 银行保险是由银行、邮政、基金组织以及其它金融机构与保险公司合作，通过共同的销售渠道向客户提供产品和服务。
            if (i==0) sprintf(wav_fn, "output/test.%d.wav", index);
            else sprintf(wav_fn, "output/test.%d.%d.wav", index, i);
            wtk_mer_process_thread(wav_fn, lab_arrs, lab_i, tts);
            // wtk_mer_process(wav_fn, lab_arrs, lab_i, tts);
            // printf("dur: %d \n", dur);
            count=0;
		}
    }
    #ifdef USE_MKL
    // mkl_thread_free_buffers();/* 仅释放当前线程中mkl的内存 */
    mkl_free_buffers(); /* 释放全部mkl内存 */
    #endif
	wtk_strbuf_delete(buf);
    wtk_heap_delete(heap);
}

#include <assert.h>
static void test()
{
    printf("神经\n");
}

int main (int argc, char *argv[])
{
    struct  timeval start;
    struct  timeval end;
    double duration;
    wtk_mer_engtts_t *engtts;
    wtk_arg_t *arg;
	char *cfg_fn=NULL;
	char *ifn=NULL;
    int is_bin=0;

	arg=wtk_arg_new(argc,argv);
	if(!arg)
	{
		print_usage();
	}
	wtk_arg_get_str_s(arg, "i", &ifn);
	wtk_arg_get_str_s(arg, "c", &cfg_fn);
    wtk_arg_get_int_s(arg, "b", &is_bin);

	if(!ifn || !cfg_fn)
	{
		print_usage();
	}
    // test();

    engtts = wtk_mer_engtts_new(cfg_fn, is_bin, 0, NULL, NULL);
    // wtk_exit(1);
    
    gettimeofday(&start,NULL);
    wtk_mer_flist_process2( ifn, engtts->tts, (wtk_flist_notify_f2)  test_tts_file);
    // wtk_mer_engtts_start(engtts, "苏州奇梦者网络科技有限公司是一家以人工智能研究为主的科技型企业， 是智能语音技术行业里快速成长的一支新秀，专注于以麦克风阵列算法、语音识别、语义对话及语音合成技术为核心的人工智能技术研究为核心， 并实现其在机器人、儿童玩具等智能硬件领域的应用.");
    // wtk_mer_engtts_start(engtts, "再见");
    // wtk_mer_wav_stream_savefile(engtts->tts->wav, "output/test.wav", duration);
    gettimeofday(&end,NULL);
    duration = (1000000*(end.tv_sec-start.tv_sec) + end.tv_usec-start.tv_usec) /1000000.0;
    printf( "总计: %lf s\n", duration );

    wtk_mer_engtts_delete(engtts);
    if(arg) { wtk_arg_delete(arg);}
    return 0;
}
