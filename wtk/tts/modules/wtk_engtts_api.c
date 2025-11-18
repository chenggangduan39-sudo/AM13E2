/*
 * wtk_tts_api.c
 *
 *  Created on: Oct 18, 2015
 *      Author: dm
 */

#include "../wtk_tts_cfg.h"
#include "../wtk_tts.h"
#include "../include/wtk_engtts_api.h"

typedef enum engtts{TTS_STATE_INIT, TTS_STATE_PROC, TTS_STATE_PAUSE, TTS_STATE_END} engtts_state;
struct wtk_engtts{
	struct wtk_tts* tts;
	struct wtk_tts_cfg* cfg;
	volatile engtts_state state;
};

wtk_engtts_t *wtk_engtts_new(char *fn)
{
	return wtk_engtts_new2(fn, NULL, NULL, NULL);
}

wtk_engtts_t *wtk_engtts_new2(char *fn, wtk_pitch_callback_f start, wtk_pitch_callback_f end, void*ths)
{
	wtk_engtts_t *engine;
	int ret=0;

	engine = (wtk_engtts_t*) wtk_calloc(1, sizeof(*engine));
	engine->cfg=wtk_tts_cfg_new_bin(fn);
	if (!engine->cfg){
		printf("failed engtts res load %s\n", fn);
		ret=-1;
		goto end;
	}

	engine->tts=wtk_tts_new2((wtk_tts_cfg_t*)engine->cfg, start, end, ths);
	if (!engine->tts){
		printf("failed engtts created\n");
		ret=-1;
		goto end;
	}
	engine->state = TTS_STATE_INIT;
end:
	if (ret!=0){
		wtk_engtts_delete(engine);
		engine=0;
	}

	return engine;
}

void wtk_engtts_delete(wtk_engtts_t *engine)
{
	if (engine->tts){
		wtk_tts_delete(engine->tts);
	}
	if (engine->cfg){
		wtk_tts_cfg_delete_bin(engine->cfg);
	}

	wtk_free(engine);
}

int wtk_engtts_reset(wtk_engtts_t *engine)
{

	return wtk_tts_reset(engine->tts);
}

void wtk_engtts_set_notify(wtk_engtts_t *engine,void *ths,wtk_engtts_notify_f notify)
{
	wtk_tts_set_notify(engine->tts, ths, (wtk_tts_notify_f)notify);
}

int wtk_engtts_feed(wtk_engtts_t *engine,char *data,int len)
{
	int ret;
	engine->state = TTS_STATE_PROC;
	ret=wtk_tts_process(engine->tts, data, len);
	engine->state = TTS_STATE_END;
	return ret;
}

void wtk_engtts_set_speed(wtk_engtts_t *engine,float speed)
{
	wtk_tts_set_speed(engine->tts, speed);
}

void wtk_engtts_set_pitch(wtk_engtts_t *engine,float pitch)
{
	wtk_tts_set_pitch(engine->tts, pitch);
}

int wtk_engtts_pause(wtk_engtts_t* engine)
{
	int ret=0;
	if (engine->state == TTS_STATE_PROC){
		engine->state = TTS_STATE_PAUSE;
		wtk_tts_pause(engine->tts);
	}else{
		ret=-1;
	}
	return ret;
}

int wtk_engtts_resume(wtk_engtts_t* engine)
{
	int ret=0;
	if (engine->state == TTS_STATE_PAUSE){
		engine->state = TTS_STATE_PROC;
		wtk_tts_resume(engine->tts);
	}else{
		ret = -1;
	}
	return ret;
}

int wtk_engtts_stop(wtk_engtts_t* engine)
{
	int ret=0;
	if (engine->state == TTS_STATE_PROC || engine->state == TTS_STATE_PAUSE){
		wtk_tts_set_stop_hint(engine->tts);
	}else{
		ret = -1;
	}

	return ret;
}

void wtk_engtts_set_volume(wtk_engtts_t* engine, float volume)
{
	wtk_tts_set_volume_scale(engine->tts, volume);
}
