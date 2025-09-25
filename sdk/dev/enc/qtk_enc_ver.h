#ifndef __QTK_ENC_VER_H__
#define __QTK_ENC_VER_H__

#include "EnDe.h"
#include <stdio.h>
#include <linux/types.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DEV_NUMBER "/dev/i2c-0"

int qtk_enc_ver(char *i2cdev_path);

#ifdef __cplusplus
};
#endif
#endif