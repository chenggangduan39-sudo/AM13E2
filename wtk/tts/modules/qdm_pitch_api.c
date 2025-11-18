/*
 * qdm_pitch_api.c
 *
 *  Created on: Dec 15, 2016
 *      Author: dm
 */

#include "../include/qdm_pitch_api.h"
#include "wtk/core/pitch/wtk_pitch.h"
struct qdm_pitch{
	wtk_pitch_t *pitch;
	wtk_pitch_cfg_t cfg;
	float shift;
};

qdm_pitch_t *qdm_pitch_new(char* fn)
{
	qdm_pitch_t *engine;
	int ret=0;

	engine = (qdm_pitch_t*) wtk_calloc(1, sizeof(*engine));
	wtk_pitch_cfg_init(&(engine->cfg));
	engine->shift=1;
	engine->pitch=wtk_pitch_new(&(engine->cfg));
	if (!engine->pitch){
		printf("failed pitch created\n");
		ret=-1;
		goto end;
	}
end:
	if (ret!=0){
		qdm_pitch_delete(engine);
		engine=0;
	}

	return engine;
}

void qdm_pitch_delete(qdm_pitch_t *engine)
{
	if (engine->pitch){
		wtk_pitch_delete(engine->pitch);
	}
	wtk_free(engine);
}

int qdm_pitch_reset(qdm_pitch_t* engine)
{
	wtk_pitch_reset(engine->pitch);
	return 0;
}

void qdm_pitch_set_notify(qdm_pitch_t* engine, void* ths,qdm_pitch_notify_f notify)
{
	wtk_pitch_set(engine->pitch, ths, (wtk_pitch_noityf_f)notify);
}

int qdm_pitch_feed_file(qdm_pitch_t* engine, char *in, char *out)
{
	wtk_pitch_convert(engine->pitch, engine->shift, in, out);
	return 0;
}

int qdm_pitch_feed(qdm_pitch_t* engine, char *data, int len)
{
	wtk_pitch_process(engine->pitch,engine->shift,data,len);

	return 0;
}


/**
 * @brief set pitch level of audio speech change, default 0
 */
void qdm_pitch_set_shift(qdm_pitch_t* engine, float shift)
{
	if (shift > 0)
		engine->shift = shift;
}


