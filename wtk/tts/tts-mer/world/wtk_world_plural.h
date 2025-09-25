#ifndef WTK_WORLD_PLURAL_H_
#define WTK_WORLD_PLURAL_H_
#include "common.h"

typedef struct
{
    double *real;/* 实部 */
    double *imag;/* 虚部 */
    int size;
} wtk_world_plural_t;

typedef struct
{
    float *real;/* 实部 */
    float *imag;/* 虚部 */
    int size;
} wtk_world_fplural_t;

/* duuble */
wtk_world_plural_t* wtk_world_plural_new(int size);
void wtk_world_plural_zero(wtk_world_plural_t *p);
void wtk_world_plural_delete(wtk_world_plural_t *p);
void wtk_world_plural_print(wtk_world_plural_t *p, int is_save);
void wtk_world_plural_mul(wtk_world_plural_t *a, wtk_world_plural_t *b, wtk_world_plural_t *dst);

/* float */
wtk_world_fplural_t* wtk_world_fplural_new(int size);
void wtk_world_fplural_zero(wtk_world_fplural_t *p);
void wtk_world_fplural_delete(wtk_world_fplural_t *p);
void wtk_world_fplural_print(wtk_world_fplural_t *p, int is_save);
void wtk_world_fplural_mul(wtk_world_fplural_t *a, wtk_world_fplural_t *b, wtk_world_fplural_t *dst);
#endif