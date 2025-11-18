/**
 * @file wtk_engtts_api.h
 * @brief tts engine api
 * @author david <daming.dai@qdreamer.com>
 * @version 1.0
 * @date 2015-10-18
 *
 * Copyright (C) 2015 suzhou Qdreamer Co., Ltd.
 *
 */

#ifndef WTK_QDREAMER_WTK_ENGTTS_API_H_
#define WTK_QDREAMER_WTK_ENGTTS_API_H_
#include "wtk/tts/wtk_tts.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef void (*wtk_engtts_notify_f)(void* ths, char* data, int bytes);
typedef struct wtk_engtts wtk_engtts_t;
/**
 * @brief create tts engine
 *
 * @param fn configure file name
 *
 * @return return tts engine, 0 on failed.
 *
 */
wtk_engtts_t *wtk_engtts_new(char* fn);
wtk_engtts_t *wtk_engtts_new2(char *fn, wtk_pitch_callback_f start, wtk_pitch_callback_f end, void*ths);
/**
 * @brief destroy tts engine
 *
 */
void wtk_engtts_delete(wtk_engtts_t *engine);

/**
 * @brief reset tts engine
 *
 * @return return 0 on success, else other.
 */
int wtk_engtts_reset(wtk_engtts_t* engine);

/**
 * @brief set feedback function and user's definition structure,  feedback when a part of whole audio finish(so maybe multi time)
 *
 * @param ths  user definition structure
 *
 * @param notify feedback function
 *
 */
void wtk_engtts_set_notify(wtk_engtts_t* engine, void* ths,wtk_engtts_notify_f notify);

/**
 * @brief engine process, engine generate audio raw data with reference text.
 *
 * @return return 0 on success, else -1.
 *
 */
int wtk_engtts_feed(wtk_engtts_t* engine, char *data, int len);

/**
 * @brief set engine level of audio speed, default 1.0
 */
void wtk_engtts_set_speed(wtk_engtts_t* engine, float speed);

/**
 * @brief set tts level of audio speech change, default 0
 */
void wtk_engtts_set_pitch(wtk_engtts_t* engine, float pitch);

int wtk_engtts_pause(wtk_engtts_t* engine);

int wtk_engtts_resume(wtk_engtts_t* engine);

int wtk_engtts_stop(wtk_engtts_t* engine);

void wtk_engtts_set_volume(wtk_engtts_t* engine, float volume);

void wtk_engtts_set_pitch_callback(wtk_engtts_t* engine, wtk_pitch_callback_f start, wtk_pitch_callback_f end, void*ths);

#ifdef __cplusplus
};
#endif
#endif /* WTK_QDREAMER_WTK_ENGTTS_API_H_ */
