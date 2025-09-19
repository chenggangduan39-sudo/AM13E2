#include "qtk/os/qtk_file.h"

int qtk_file_read(FILE *f, char *d, int l) {
    size_t ret;
    ret = fread(d, 1, l, f);
    if (ret > 0) {
        return ret;
    }
    return -1;
}

int qtk_file_write(FILE *f, char *d, int l) {
    int ret;
    int err = 0;
    ret = fwrite(d, 1, l, f);
    if (ret < l) {
        err = ferror(f);
    }
    return err ? -1 : ret;
}
