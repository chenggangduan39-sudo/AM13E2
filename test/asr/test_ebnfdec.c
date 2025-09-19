#include "wtk/asr/wfst/ebnfdec/wtk_ebnfdec.h"

void test_ebnfdec()
{
	wtk_ebnfdec_cfg_t cfg;
	char *fn;
	char *data;
	int len;
	wtk_ebnfdec_t *dec;
	int i;
	wtk_string_t v;

	fn="./data/input1362075524661.wav";
	fn="./x.wav";
	fn="./recx/xiaohe.wav";
	//fn="./recx/xiaohe1.wav";
	//fn="./p.0.wav";
	data=file_read_buf(fn,&len);
	wtk_ebnfdec_cfg_init(&(cfg));
	cfg.compile_fn="./recx/grammar/gc.far.0.0.5.bin";
	cfg.dec_fn="./recx/grammar/gr.far.0.0.5.bin";
	//cfg.vad_fn="./res/robotc/vad.0.6.bin";
	cfg.vad_fn=NULL;
	wtk_ebnfdec_cfg_update(&(cfg));

	dec=wtk_ebnfdec_new(&(cfg));
	wtk_ebnfdec_set_word_s(dec,"小盒小盒");

	for(i=0;i<1;++i)
	{
		wtk_ebnfdec_start(dec);
		wtk_ebnfdec_feed(dec,data+44,len-44,1);
		v=wtk_ebnfdec_get_result(dec);
		wtk_debug("[%.*s]\n",v.len,v.data);
		wtk_ebnfdec_reset(dec);
	}
	wtk_ebnfdec_delete(dec);

	wtk_ebnfdec_cfg_clean(&(cfg));
	wtk_free(data);
}
