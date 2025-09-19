#include "wtk_mer_sound.h"
static double max_x=0;
static int is_allow=0;

double wtk_world_sound_get(double f)
{
    return f>max_x? f: max_x;
}
void wtk_world_sound_set_allow(int i)
{
    is_allow=i;
}
void wtk_world_sound_set(double f)
{
    if (is_allow && f>max_x) {max_x=f;}
}
void wtk_world_sound_reset()
{
    max_x=0;
}