#ifndef QTK_CORE_QTK_AUDIO_CONVERSION_H
#define QTK_CORE_QTK_AUDIO_CONVERSION_H

#ifdef __cplusplus
extern "C"{
#endif

void qtk_data_change_vol(char *data, int bytes, float shift);
void qtk_data_change_vol2(char *data, int bytes, float mshift, float sshift, int mic, int spk);
#ifdef __cplusplus
}
#endif
#endif