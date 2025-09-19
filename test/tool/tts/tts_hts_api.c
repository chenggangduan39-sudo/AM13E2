#include "wtk/tts/include/wtk_engtts_api.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>


static void test_engtts_save(FILE *fp,char *data,int bytes)
{
	fwrite(data, 1, bytes, fp);
}
FILE *fp;
void test_tts_api()
{
	wtk_engtts_t *engine;
	char *s;
//	int i;
//	double t;
	char *cfg_fn;
	char *fn;


	fn="./test.wav";
	cfg_fn="./api/tts/res/girl.0.0.1.bin";
	engine=wtk_engtts_new(cfg_fn);
	if (!engine){
		return;
	}

	fp = fopen(fn, "w");
	//optional,default 1
	wtk_engtts_set_speed(engine, 1.1);
	//optional,default 0
	wtk_engtts_set_pitch(engine, 0.8);
	s="您好，我是苏州奇梦者网络科技有限公司，专注智能人机交互";

	wtk_engtts_set_notify(engine,fp,(wtk_engtts_notify_f)test_engtts_save);
	wtk_engtts_feed(engine,s,strlen(s));
	wtk_engtts_delete(engine);
	fclose(fp);
}

//for mips
int main(int argc,char **argv)
{
	test_tts_api();
	return 0;
}

