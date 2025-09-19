/***************************************************************
LEGAL DISCLAIMER
(Header of MediaTek Software/Firmware Release or Documentation)
Mediatek Inc (C) 2017~2018. All right reserved.

File name: user_interface_gpio.c

Author: Yuyun.liu(just for coding style)

Description:

others:
************************************************************** */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>

#include "u_gpio.h"

 /****************************************************************
 * Constants
 ****************************************************************/

#define SYSFS_GPIO_DIR "/sys/class/gpio"
#define POLL_TIMEOUT (3 * 1000) /* 3 seconds */
#define MAX_BUF 64

/****************************************************************
 * uiGpioExport
 ****************************************************************/
int uiGpioExport(unsigned int gpio)
{
    int  fd, len, ret;
    char buf[MAX_BUF];

    fd = open(SYSFS_GPIO_DIR "/export", O_WRONLY);
    if (fd < 0)
    {
        perror("gpio/export");
        return fd;
    }
    len = snprintf(buf, sizeof(buf), "%d", gpio);
    ret = write(fd, buf, len);
    if(ret < 0)
    {
        return ret;
    }
    close(fd);
    return 0;
}

/****************************************************************
 * uiGpioUnexport
 ****************************************************************/
int uiGpioUnexport(unsigned int gpio)
{
    int  fd, len, ret;
    char buf[MAX_BUF];

    fd = open(SYSFS_GPIO_DIR "/unexport", O_WRONLY);
    if (fd < 0)
    {
        perror("gpio/export");
        return fd;
    }
    len = snprintf(buf, sizeof(buf), "%d", gpio);
    ret = write(fd, buf, len);
    if(ret < 0)
    {
        return ret;
    }
    close(fd);
    return 0;
}

/****************************************************************
 * uiGpioSetDir
    set gpio direction  input or output
 ****************************************************************/
int uiGpioSetDir(unsigned int gpio, unsigned int pOutFlag)
{
    int  fd, ret;
    char buf[MAX_BUF];

    snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR  "/gpio%d/direction", gpio);
    fd = open(buf, O_WRONLY);
    if (fd < 0)
    {
        perror("gpio/direction");
        return fd;
    }
    if (pOutFlag)
    {
        ret = write(fd, "out", 4);
        if(ret < 0)
        {
            return ret;
        }
    }
    else
    {
        ret = write(fd, "in", 3);
        if(ret < 0)
        {
            return ret;
        }
    }
    close(fd);
    return 0;
}

/****************************************************************
 * uiGpioSetValue
 ****************************************************************/
int uiGpioSetValue(unsigned int gpio, unsigned int value)
{
    int  fd, ret;
    char buf[MAX_BUF];

    snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR "/gpio%d/value", gpio);
    fd = open(buf, O_WRONLY);
    if (fd < 0)
    {
        perror("gpio/set-value");
        return fd;
    }
    if (value)
    {
        ret = write(fd, "1", 2);
        if(ret < 0)
        {
            return ret;
        }
    }
    else
    {
        ret = write(fd, "0", 2);
        if(ret < 0)
        {
            return ret;
        }
    }
    close(fd);
    return 0;
}

/*----------------------------------------------------------------------------
 * Function: uiGpioGetValue
 * Description:
 *      The GPIO input reading functions. It will check the gpio and read
 *      related gpio device file.
 * Inputs:
 *      gpio: the gpio number to read.
 * Outputs:
 * Returns:
 *      GPIO input value.
 *---------------------------------------------------------------------------*/
int uiGpioGetValue(unsigned int gpio)
{
    int  fd, val, ret;
    char buf[MAX_BUF];
    char ch;

    snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR "/gpio%d/value", gpio);
    fd = open(buf, O_RDONLY);
    if (fd < 0)
    {
        perror("gpio/get-value");
        return fd;
    }
    ret = read(fd, &ch, 1);
    if(ret < 0)
    {
        return ret;
    }
    if (ch != '0')
    {
        val = 1;
    }
    else
    {
        val = 0;
    }
    close(fd);
    return val;
}

/****************************************************************
 * uiGpioSetEdge
 ****************************************************************/
int uiGpioSetEdge(unsigned int gpio, char *edge)
{
    int  fd, ret;
    char buf[MAX_BUF];

    snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR "/gpio%d/edge", gpio);
    fd = open(buf, O_WRONLY);
    if (fd < 0)
    {
        perror("gpio/set-edge");
        return fd;
    }
    ret = write(fd, edge, strlen(edge) + 1);
    if(ret < 0)
    {
        return ret;
    }
    close(fd);
    return 0;
}

/****************************************************************
 * uiGpioFdOpen
 ****************************************************************/
int uiGpioFdOpen(unsigned int gpio)
{
    int  fd;
    char buf[MAX_BUF];

    snprintf(buf, sizeof(buf), SYSFS_GPIO_DIR "/gpio%d/value", gpio);
    fd = open(buf, O_RDONLY | O_NONBLOCK );
    if (fd < 0)
    {
        perror("gpio/fd_open");
    }
    return fd;
}

/****************************************************************
 * uiGpioFdClose
 ****************************************************************/
int uiGpioFdClose(int fd)
{
    return close(fd);
}

/*----------------------------------------------------------------------------
 * Function: uiGpioConfig
 * Description:
 *   Configure GPIO pin. It sets pin as gpio�Boutput or input mode and
 *   set output value if output mode.
 * Inputs:
 *      i4GpioNum: the gpio number to be set.
 *      i4Output:  If the integer is 0, this function will set the mode of the
 *                 gpio number as input mode, otherwise set as output mode.
 *      i4High: In output mode,if the integer is 0, this function will set the
 *              bit of the gpio number as 0, otherwise set as 1.
                If input mode, ignore it.
 * Outputs:
 * Returns:
 *---------------------------------------------------------------------------*/
void uiGpioConfig(unsigned int i4GpioNum,
                    unsigned int i4OutIn,
                    unsigned int i4High)
{
    i4GpioNum = GPIO_BASE_VALUE + i4GpioNum;
    uiGpioExport(i4GpioNum);
    uiGpioSetDir(i4GpioNum, i4OutIn);
    uiGpioSetValue(i4GpioNum, i4High);
}
