1. bfio module 中 wtk_gainnet_bf_xx 验证结果差异问题
    wtk_equalizer_new存在rand() 使用，在验证结果时，会存在不同程序结果差异问题，需要考虑屏蔽的话，可在使用前，使用固定随机种子srand(1)
    建议引入调试宏定义选项下，开启固定结果，以避免在验证结果上花不必要的时间。
    srand(1); //Note
    for(i = 0; i < 256; ++i){

        eq->dither[i] = (rand()%4)-2;

    }

