package com.qdreamer.qvoice.record;

import com.qdreamer.qvoice.listener.OnAudioRecordListener;

public interface IRecordPresenter {

    /**
     * 初始化录音机
     */
    void initRecorder(IRecordOption option) throws Exception;

    /**
     * 开启录音机
     * [listener] 录音数据回调
     */
    boolean startRecorder(OnAudioRecordListener listener);

    /**
     * 停止录音机
     */
    void stopRecorder();

    /**
     * 销毁录音机
     */
    void releaseRecorder();

}
