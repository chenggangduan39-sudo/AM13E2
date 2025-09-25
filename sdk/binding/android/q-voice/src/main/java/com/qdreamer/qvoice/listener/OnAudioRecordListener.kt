package com.qdreamer.qvoice.listener

interface OnAudioRecordListener {

    /**
     * 录音数据回调
     */
    fun onRecordAudio(audio: ByteArray)

}