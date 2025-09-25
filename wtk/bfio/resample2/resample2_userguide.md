SR = 48000;     //初始采样率                                                                              
new_SR = 16000;//目标采样率，实际工程中尽量保证new_SR和SR有整数倍的关系；非整数倍情况下，可能存在bug。
downsamp_rate = 3;//downsample_rate=int(SR/new_SR)，upsample_rate=int(new_SR/SR)
frm_size = 240;//每次feed的帧长
window_sz = 512;//为什么需要窗？因为计算中采用fft来加速时域卷积。
taps = 270;//滤波器抽头数，必须为偶数。另外taps和frm_size要保证一定的关系，否则会出现频谱混叠。我的经验是taps>=frm_size，同时taps+frm_size+1<=window_sz,保证taps+frm_size尽可能的大。
weight_len = 271;//taps+1
beta = 9.0;//kaiser窗的参数，控制窗的形状，一般默认就行
cutoff_ratio = 0.15;//滤波器截止频率，一般默认就行
