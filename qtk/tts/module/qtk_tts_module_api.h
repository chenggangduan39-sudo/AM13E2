/*
 * qtk_tts_module_api.h
 *
 *  Created on: Apr 25, 2023
 *      Author: dm
 */

#ifndef QTK_TTS_MODULE_QTK_TTS_MODULE_API_H_
#define QTK_TTS_MODULE_QTK_TTS_MODULE_API_H_
#ifdef DLL_EXPORTS
#define TTS_DLL_API __declspec(dllexport)
#else
#define TTS_DLL_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef int(*qtk_tts_notify_f)(void* ths, short* data, int len, int is_end);

TTS_DLL_API void* qtk_tts_module_api_new(char *fn, int use_bin);
TTS_DLL_API int qtk_tts_module_api_feed(void* m, char* data, int len);
TTS_DLL_API int qtk_tts_module_api_reset(void* m);
TTS_DLL_API void qtk_tts_module_api_delete(void *api);
TTS_DLL_API void qtk_tts_module_api_setNotify(void* m, qtk_tts_notify_f notify, void* user_data);
TTS_DLL_API void qtk_tts_module_api_setVol(void* m, float vol);
TTS_DLL_API void qtk_tts_module_api_setPitch(void* m, float pitch);
TTS_DLL_API void qtk_tts_module_api_setTempo(void* m, float tempo);
TTS_DLL_API void qtk_tts_module_api_setVolChanged(void* m, float volDelta);
TTS_DLL_API void qtk_tts_module_api_setPitchChanged(void* m, float pitchDelta);
TTS_DLL_API void qtk_tts_module_api_setTempoChanged(void* m, float tempoDelta);
TTS_DLL_API void qtk_tts_module_api_setRateChanged(void* m, float rateDelta);
#ifdef __cplusplus
};
#endif
#endif /* QTK_TTS_MODULE_QTK_TTS_MODULE_API_H_ */
