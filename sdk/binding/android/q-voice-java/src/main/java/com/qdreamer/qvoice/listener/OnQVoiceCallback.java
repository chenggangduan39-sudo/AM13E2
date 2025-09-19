package com.qdreamer.qvoice.listener;

public interface OnQVoiceCallback {

    /**
     * SDK 结果回调
     * @param type 消息类型 {@link com.qdreamer.qvoice.constants.QVoiceCode}
     * @param data 消息数据，data 可能为 null
     */
    void onQVoiceCallback(byte type, byte[] data);

}
