package com.qdreamer.qvoice.record

import com.qdreamer.qvoice.listener.OnAudioRecordListener

interface IRecordPresenter {

    /**
     * 初始化录音机
     */
    @Throws(Exception::class)
    fun initRecorder(option: IRecordOption)

    /**
     * 开启录音机
     * [listener] 录音数据回调
     */
    fun startRecorder(listener: OnAudioRecordListener? = null): Boolean

    /**
     * 停止录音机
     */
    fun stopRecorder()

    /**
     * 销毁录音机
     */
    fun releaseRecorder()

}