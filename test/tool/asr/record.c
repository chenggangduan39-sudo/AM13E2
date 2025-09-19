#include "wtk/audio/wtk_auin.h"
#include "wtk/core/wtk_strbuf.h"
#include "wtk/core/wavehdr.h"

int main(int argc,char **argv)
{
	wtk_auin_cfg_t cfg;
	wtk_auin_t *auin;
	wtk_strbuf_t *buf;

	wtk_auin_cfg_init(&(cfg));
	wtk_auin_cfg_update(&(cfg));
	buf=wtk_strbuf_new(256,1);
	auin=wtk_auin_new(&(cfg),NULL,buf,(wtk_auin_process_f)wtk_strbuf_push);
	wtk_auin_start(auin);
	wtk_debug("start ============\n");

	getchar();

	wtk_debug("stop ============\n");
	wtk_auin_stop(auin);

	wave_write_file("x.wav",16000,buf->data,buf->pos);
	wtk_auin_delete(auin);
	return 0;
}
