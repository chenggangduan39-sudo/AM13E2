package com.qdreamer.qvoice.listener

interface OnQVoiceCallback {

    /**
     * [type] 回调消息类型
     * [data] 对应消息类型的真实数据
     */
    fun onQVoiceCallback(type: Byte, data: ByteArray?)

}