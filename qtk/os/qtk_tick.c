#include "qtk_tick.h"

#include <time.h>

uint64_t qtk_get_tick_ms(void) {
    uint64_t t;
    #ifdef WIN32
    LARGE_INTEGER lt, ft;
    QueryPerformanceFrequency(&ft);
    QueryPerformanceCounter(&lt);
    t = lt.QuadPart * 1000.0 / ft.QuadPart;
    #else
    struct timespec ti;
    clock_gettime(CLOCK_MONOTONIC, &ti);
    t = (uint64_t)ti.tv_sec * 1000;
    t += ti.tv_nsec / 1000000;
    #endif

    return t;
}

double qtk_get_tick_ms_d(void) {
    double t;
    #ifdef WIN32
    LARGE_INTEGER lt, ft;
    QueryPerformanceFrequency(&ft);
    QueryPerformanceCounter(&lt);
    t = lt.QuadPart * 1000.0 / ft.QuadPart;
    #else
    struct timespec ti;
    clock_gettime(CLOCK_MONOTONIC, &ti);
    t = (uint64_t)ti.tv_sec * 1000;
    t += ti.tv_nsec / 1000000.0;
    #endif

    return t;
}
