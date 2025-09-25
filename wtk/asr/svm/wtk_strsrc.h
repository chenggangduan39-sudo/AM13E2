#ifndef WTK_STRSRC_H_
#define WTK_STRSRC_H_

#include "wtk/core/wtk_str.h"
#include "wtk/core/cfg/wtk_source.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    const wtk_string_t *str;
    int pos;
} wtk_strsrc_data_t;

int wtk_strsrc_init(wtk_source_t *, wtk_strsrc_data_t *data, const wtk_string_t *str);

#ifdef __cplusplus
}
#endif
#endif
