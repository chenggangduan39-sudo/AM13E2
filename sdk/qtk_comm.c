#include "qtk_comm.h"

static char number1[]={
    0x99 ,0x23 ,0x80 ,0x01 ,0x7F ,0xAA
};
static char number2[]={
    0x99 ,0x23 ,0x01 ,0x01 ,0xFE ,0xAA
};
static char number3[]={
    0x99 ,0x23 ,0x11 ,0x01 ,0xEE ,0xAA
};
static char number4[]={
    0x99 ,0x23 ,0x00 ,0x01 ,0xFF ,0xAA
};
static char number5[]={
    0x99 ,0x23 ,0x0E ,0x01 ,0xF1 ,0xAA
};
static char number6[]={
    0x99 ,0x23 ,0x0F ,0x01 ,0xF0 ,0xAA
};
static char number7[]={
    0x99 ,0x23 ,0x10 ,0x01 ,0xEF ,0xAA
};
static char number8[]={
    0x99 ,0x23 ,0x0B ,0x01 ,0xF4 ,0xAA
};
static char number9[]={
    0x99 ,0x23 ,0x17 ,0x01 ,0xE8 ,0xAA
};
static char number10[]={
    0x99 ,0x23 ,0x18 ,0x01 ,0xE7 ,0xAA
};
static char number11[]={
    0x70 ,0x00 ,0x00 ,0x00 ,0x01 ,0xAA
};
static char number12[]={
    0x70 ,0x00 ,0x00 ,0x00 ,0x02 ,0xAA
};
static char number13[]={
    0x70 ,0x00 ,0x00 ,0x00 ,0x03 ,0xAA
};
static char number14[]={
    0x70 ,0x00 ,0x00 ,0x00 ,0x04 ,0xAA
};
static char number15[]={
    0x70 ,0x00 ,0x00 ,0x00 ,0x05 ,0xAA
};
static char number16[]={
    0x70 ,0x00 ,0x00 ,0x00 ,0x06 ,0xAA
};

qtk_comm_t qtk_comm_array[]={
    {"开机",number1},
    {"关机",number2},
    {"切换OPS通道",number3},
    {"回到主页",number4},
    {"切换HDMI1通道",number5},
    {"切换HDMI2通道",number6},
    {"切换HDMI3通道",number7},
    {"切换VGA通道",number8},
    {"音量加大",number9},
    {"音量降低",number10},
    {"启动白板",number11},
    {"启动批注",number12},
    {"启动录音",number13},
    {"启动相机",number14},
    {"启动光感",number15},
    {"启动音乐模式",number16},
};

int qtk_comm_get_number(char *input,int ilen,char *output, int *olen)
{
    int ret=-1;
    int i=0,j;
    // wtk_debug("===============>>%d\n",sizeof(qtk_comm_array));
    while(i < 16)
    {
        if(strncmp(input,qtk_comm_array[i].comm,ilen) == 0)
        {
            for(j=0;j<6;++j)
            {
                output[j] = qtk_comm_array[i].num[j];
                printf("%x ",output[j]);
            }
            printf("\n");
            *olen = 6;
            // wtk_debug("-=----------------->>i=%d olen=%d %x\n",i,*olen,qtk_comm_array[i].num[0]);
            ret = 0;
            break;
        }
        i++;
    }

    return ret;
}
