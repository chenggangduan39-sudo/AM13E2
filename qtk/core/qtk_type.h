#ifndef QBL_CORE_QBL_TYPE_H_
#define QBL_CORE_QBL_TYPE_H_
#include "wtk/core/wtk_type.h"
#ifdef __cplusplus
extern "C" {
#endif

typedef void *(*qtk_new_handler_t)(void *user_data);
typedef int (*qtk_delete_handler_t)(void *data);
typedef int (*qtk_delete_handler2_t)(void *user_data, void *data);
typedef int (*qtk_cmp_handler_t)(void *d1, void *d2);
typedef int (*qtk_walk_handler_t)(void *user_data, void *data);
typedef void (*qtk_print_handler_t)(void *data);
typedef void (*qtk_write_f)(void *inst, const char *data, int bytes);

#define unuse(var) (void)(var)
#ifdef NO_EXIT
#define exit(code) unuse(code)
#endif

#ifndef QTK_INLINE
#define QTK_INLINE __inline
#endif

#ifdef NO_DEBUG
static QBL_INLINE void __nop(int dummpy, ...) {}
#define qtk_debug(...) __nop(0, __VA_ARGS__)
#else // NO_DEBUG
#define qtk_debug(...)                                                         \
    do {                                                                       \
        printf("%s:%d:", __FUNCTION__, __LINE__);                              \
        printf(__VA_ARGS__);                                                   \
        fflush(stdout);                                                        \
    } while (0)
#endif // NO_DEBUG

#include <assert.h>

#define qtk_assert(expr) assert(expr)
#define qtk_num2ptr(ptr_type, num) cast(ptr_type, cast(uintptr_t, num))
#define qtk_ptr2num(num_type, ptr) cast(num_type, cast(uintptr_t, ptr))

#define COUNT_OF(x)                                                            \
    ((sizeof(x) / sizeof(0 [x])) / ((size_t)(!(sizeof(x) % sizeof(0 [x])))))

#if defined(__GNUC__) || defined(__clang__)
#define qtk_unreachable() __builtin_unreachable()
#else
#define qtk_unreachable()
#endif

#define qtk_align_size(sz, n) (((sz) + (n) - 1) & -n)

#ifdef __cplusplus
};
#endif
#endif
