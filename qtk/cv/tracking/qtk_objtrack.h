#ifndef __QBL_OBJECT_TRACKING_QBL_OBJTRACK_H__
#define __QBL_OBJECT_TRACKING_QBL_OBJTRACK_H__
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    TRACKING_GRAY = (1 << 0),
    TRACKING_CN = (1 << 1),
    TRACKING_CUSTOM = (1 << 2)
} qtk_objtrack_mode_t;

#ifdef __cplusplus
};
#endif
#endif
