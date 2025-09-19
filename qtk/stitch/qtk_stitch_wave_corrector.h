#ifndef __QTK_STITCH_WAVE_CORRECTOR_H__
#define __QTK_STITCH_WAVE_CORRECTOR_H__


#ifdef __cplusplus
extern "C" {
#endif

typedef struct qtk_stitch_wave_corrector{
    int type;
    int wave_correct_type;
}qtk_stitch_wave_corrector_t;

qtk_stitch_wave_corrector_t* qtk_stitch_wave_corrector_new(int type);
void qtk_stitch_wave_corrector_correct(qtk_stitch_wave_corrector_t* corrector,
                                        void *estimated_cameras);
void qtk_stitch_wave_corrector_delete(qtk_stitch_wave_corrector_t* corrector);

#ifdef __cplusplus
}
#endif

#endif