/*
 * qtk_soundtouch_glue.h
 *
 *  Created on: Mar 30, 2023
 *      Author: dm
 */

#ifndef QTK_SOUNDTOUCH_QTK_SOUNDTOUCH_API_H_
#define QTK_SOUNDTOUCH_QTK_SOUNDTOUCH_API_H_

// Sound stretch Processing chunk size (size chosen to be divisible by 2, 4, 6, 8, 10, 12, 14, 16 channels ...)
#ifndef SOUNDTOUCH_BUFF_SIZE
#define SOUNDTOUCH_BUFF_SIZE           6720
#endif

#ifdef DLL_EXPORTS
#define SOUNDTOUCHDLL_API __declspec(dllexport)
#else
#define SOUNDTOUCHDLL_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif
SOUNDTOUCHDLL_API void* qtk_soundtouch_api_new(void* c);
SOUNDTOUCHDLL_API void qtk_soundtouch_api_feed(void* api, short* data, int len);
SOUNDTOUCHDLL_API int qtk_soundtouch_api_recv(void* api, short* recvbuf, int size);
SOUNDTOUCHDLL_API void qtk_soundtouch_api_delete(void* api);
SOUNDTOUCHDLL_API void qtk_soundtouch_api_flush(void*api);

SOUNDTOUCHDLL_API void qtk_soundtouch_api_settemp(void* api, float tempo);
SOUNDTOUCHDLL_API void qtk_soundtouch_api_setpitch(void* api, float pitch);
SOUNDTOUCHDLL_API void qtk_soundtouch_api_setrate(void* api, float rate);
SOUNDTOUCHDLL_API void qtk_soundtouch_api_setTempoChange(void* api, float tempoDelta);
SOUNDTOUCHDLL_API void qtk_soundtouch_api_setPitchSemiTones(void* api, float pitchDelta);
SOUNDTOUCHDLL_API void qtk_soundtouch_api_setRateChange(void* api, float rateDelta);

////////////////raw test/////////////
//int test_soundtouch(const int nParams, const char * const paramStr[]);
////////////////api test////////////
//int test_soundtouch_api(const int nParams, const char * const paramStr[]);
#ifdef __cplusplus
}
#endif
#endif /* QTK_SOUNDTOUCH_QTK_SOUNDTOUCH_API_H_ */
