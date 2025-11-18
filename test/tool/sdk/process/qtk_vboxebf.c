#include "qtk/api/vboxebf/qtk_vboxebf.h"
#include "wtk/core/wtk_arg.h"
#include "wtk/core/cfg/wtk_main_cfg.h"
#include "wtk/core/wtk_riff.h"

int co=0;
void test_on_data(wtk_wavfile_t *wav, char *data, int len)
{
	if(len > 0)
	{
		wtk_wavfile_write(wav, data, len);
		fflush(wav->file);
	}
}

void test_file(qtk_vboxebf_t *vb, char *data_fn)
{
	char *data;
	int datalen;
	int ret;
	wtk_riff_t *riff;
	
	riff = wtk_riff_new();
	wtk_riff_open(riff, data_fn);

	int step=20*32*10;
	data=(char *)wtk_malloc(step);

	// wtk_riff_read(riff, data,56*5);

	while(1)
	{
		ret = wtk_riff_read(riff, data, step);
		if(ret <= 0)
		{
			wtk_debug("break ret=%d\n",ret);
			break;
		}
		if(ret < step)
		{
			break;
		}
		qtk_vboxebf_feed(vb, data, ret, 0);

		datalen+=ret;
	}
	qtk_vboxebf_feed(vb, NULL, 0, 1);
	qtk_vboxebf_reset(vb);
	wtk_riff_delete(riff);
	wtk_free(data);
}

int vboxebf_run(int argc, char **argv)
{
	wtk_arg_t *arg = NULL;
	qtk_vboxebf_cfg_t *cfg = NULL;
	qtk_vboxebf_t *vb = NULL;
	char *cfg_fn=NULL;
	char *data_fn=NULL;
	char *out_fn=NULL;
	int ret;
	//float tm;
	wtk_wavfile_t *wav;


	arg = wtk_arg_new(argc, argv);
	ret = wtk_arg_get_str_s(arg, "c", &cfg_fn);
	if(ret != 0){
		wtk_debug("not cfg_fn\n");
		goto end;
	}

	ret = wtk_arg_get_str_s(arg, "i", &data_fn);
	if(ret != 0){
		wtk_debug("not data_fn\n");
		goto end;
	}

	ret = wtk_arg_get_str_s(arg, "o", &out_fn);
	if(ret != 0){
		wtk_debug("not data_fn\n");
		goto end;
	}

	wav = wtk_wavfile_new(48000);
	wav->max_pend = 0;
	wtk_wavfile_open(wav, out_fn);
	wtk_wavfile_set_channel(wav, 1);

	cfg = qtk_vboxebf_cfg_new(cfg_fn);
	vb = qtk_vboxebf_new(cfg);
	if(!vb){
		wtk_debug("vboxebf new failed.\n")
		goto end;
	}
	qtk_vboxebf_set_notify(vb, wav, (qtk_vboxebf_notify_f)test_on_data);
	qtk_vboxebf_start(vb);

	test_file(vb, data_fn);

end:
	if(arg){
		wtk_arg_delete(arg);
	}
	if(wav)
	{
		wtk_wavfile_close(wav);
		wtk_wavfile_delete(wav);
	}
	if(vb)
	{
		qtk_vboxebf_delete(vb);
	}
	if(cfg)
	{
		qtk_vboxebf_cfg_delete(cfg);
	}

	return 0;
}

int main(int argc, char **argv)
{
	vboxebf_run(argc, argv);
	return 0;
}
