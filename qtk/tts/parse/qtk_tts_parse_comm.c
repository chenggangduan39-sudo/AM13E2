#include "qtk_tts_parse_comm.h"

int qtk_tts_parse_isBEMS(int i,int len)
{
    int ret = 0;
    if(len == 1 && i == 1){
        ret = 4;    //S
    }else if(len != 1 && i == 1){
        ret = 1;    //B
    }else if(len != 1 && i == len){
        ret = 2; //E
    }else{
        ret = 3; //M
    }
    return ret;
}
