#ifndef WTK_MER_SOUND_H
#define WTK_MER_SOUND_H
#ifdef __cplusplus
extern "C" {
#endif

double wtk_world_sound_get(double f);
void wtk_world_sound_reset();
void wtk_world_sound_set_allow(int i);
void wtk_world_sound_set(double f);

#ifdef __cplusplus
}
#endif
#endif