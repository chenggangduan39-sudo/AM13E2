#ifndef FA80B6D6_FDE7_4002_46EA_822DF458658E
#define FA80B6D6_FDE7_4002_46EA_822DF458658E

#include "qtk/core/qtk_io.h"
#include "wtk/core/wtk_type.h"

int qtk_serde_msgpack_pack_map(uint32_t n, void *upval, qtk_io_writer w);
int qtk_serde_msgpack_pack_str(const char *data, uint32_t len, void *upval,
                               qtk_io_writer w);
int qtk_serde_msgpack_pack_array(size_t n, void *upval, qtk_io_writer w);
int qtk_serde_msgpack_pack_float(float f, void *upval, qtk_io_writer w);
int qtk_serde_msgpack_pack_bin(const void *d, uint32_t len, void *upval,
                               qtk_io_writer w);
int qtk_serde_msgpack_pack_int32(int32_t d, void *upval, qtk_io_writer w);

#define qtk_serde_msgpack_pack_s(s, upval, w)                                  \
    qtk_serde_msgpack_pack_str(s, sizeof(s) - 1, upval, w)

#endif /* FA80B6D6_FDE7_4002_46EA_822DF458658E */
