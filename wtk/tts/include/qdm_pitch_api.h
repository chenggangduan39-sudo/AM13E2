/*
 * qdm_pitch_api.h
 *
 *  Created on: Dec 15, 2016
 *      Author: dm
 */

#ifndef QDM_PITCH_API_H_
#define QDM_PITCH_API_H_
#ifdef __cplusplus
extern "C"{
#endif
typedef void (*qdm_pitch_notify_f)(void* ths, char* data, int bytes);
typedef struct qdm_pitch qdm_pitch_t;

/**
 * @brief create pitch engine
 *
 * @param fn configure file name
 *
 * @return return pitch engine, 0 on failed.
 *
 */
qdm_pitch_t *qdm_pitch_new(char* fn);

/**
 * @brief destroy pitch engine
 *
 */
void qdm_pitch_delete(qdm_pitch_t *engine);

/**
 * @brief reset pitch engine
 *
 * @return return 0 on success, else other.
 */
int qdm_pitch_reset(qdm_pitch_t* engine);

/**
 * @brief set feedback function and user's definition structure,  feedback when a part of whole audio finish(so maybe multi time)
 *
 * @param ths  user definition structure
 *
 * @param notify feedback function
 *
 */
void qdm_pitch_set_notify(qdm_pitch_t* engine, void* ths,qdm_pitch_notify_f notify);

/**
 * @brief engine process, engine generate audio raw data with audio file.
 *
 * @return return 0 on success, else -1.
 *
 */
int qdm_pitch_feed_file(qdm_pitch_t* engine, char *in, char *out);

int qdm_pitch_feed(qdm_pitch_t* engine, char *data, int len);


/**
 * @brief set pitch level of audio speech change, default 0
 */
void qdm_pitch_set_shift(qdm_pitch_t* engine, float shift);
#ifdef __cplusplus
}
#endif
#endif /* WTK_PITCH_API_H_ */
