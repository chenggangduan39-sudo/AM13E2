#include <stdio.h>  
#include <stdlib.h> 
#include "sdk/codec/oggenc/qtk_oggenc.h"
#include "sdk/codec/oggenc/qtk_oggenc_cfg.h"
#include "sdk/codec/oggdec/qtk_oggdec.h"
#include "wtk/core/wtk_wavfile.h"

void test_enc_on_data(FILE *ff, char *data, int len)
{
	if(len > 0)
	{
		fwrite(data, 1, len, ff);
		fflush(ff);
	}
}

void test_dec_on_data(FILE *ff, char *data, int len)
{
	if(len > 0)
	{
		fwrite(data, 1, len, ff);
		fflush(ff);
	}
}

void test_ogg_enc(char *ifn, char *ofn)
{
	FILE *encf=NULL;
	char *data=NULL;
	int len=0;
	qtk_oggenc_cfg_t cfg;
	qtk_oggenc_t *enc=NULL;

	qtk_oggenc_cfg_init(&cfg);
	qtk_oggenc_cfg_update(&cfg);

	encf = fopen(ofn, "wb");


	enc = qtk_oggenc_new(&cfg);
	qtk_oggenc_set_write(enc, encf, (qtk_oggenc_write_f)test_enc_on_data);

	qtk_oggenc_start(enc, 16000, 1, 16);

	data = file_read_buf(ifn, &len);
	char *ss=data+44;
	// char *ee=data+len-44;
	// int step=1024, nx=0;
	// while(ss<ee)
	// {
	// 	nx = min(ee-ss, step);
	// 	qtk_oggenc_encode(enc, ss, nx, 0);
	// 	ss+=nx;
	// }
	// qtk_oggenc_encode(enc, NULL, 0, 1);

	qtk_oggenc_encode(enc, ss, len-44, 1);

	qtk_oggenc_reset(enc);
	qtk_oggenc_delete(enc);
	qtk_oggenc_cfg_clean(&cfg);
	if(encf)
	{
		fclose(encf);
	}
	if(data)
	{
		wtk_free(data);
	}
}

void test_ogg_dec(char *ifn, char *ofn)
{
	FILE *decf=NULL;
	char *data=NULL;
	int len=0;
	qtk_oggdec_t *dec=NULL;

	decf = fopen(ofn, "wb");

	dec = qtk_oggdec_new();
	qtk_oggdec_start(dec, (qtk_oggdec_write_f)test_dec_on_data, decf);

	data = file_read_buf(ifn, &len);
	// char *ss=data;
	// char *ee=data+len;
	// int step=1024, nx=0;

	// while(ss<ee)
	// {
	// 	nx = min(ee-ss, step);
	// 	wtk_debug("========>>>>>>>>>>>>nx=%d\n",nx);
	// 	qtk_oggdec_feed(dec, ss, nx);
	// 	ss+=nx;
	// }
	qtk_oggdec_feed(dec, data, len);

	qtk_oggdec_stop(dec);
	qtk_oggdec_delete(dec);
	if(decf)
	{
		fclose(decf);
	}
	if(data)
	{
		wtk_free(data);
	}
}

int main(int argc, char* argv[])  
{
	int ret=atoi(argv[3]);
	if(ret == 0)
	{
		test_ogg_enc(argv[1], argv[2]);
	}else if(ret == 1){
		test_ogg_dec(argv[1], argv[2]);
	}
	return 0;  
}
