package com.qdreamer.qvoice.record.impl

import com.qdreamer.qvoice.record.IRecordOption

data class QAudioRecordOption(
    // audio.cfg 配置文件相关的 QAudio 初始化参数
    val audioCfg: String,
    // QSession.init 返回值，不传则由录音机内部初始化 QSession
    var qSession: Long? = null,
    // 由录音机内部初始化 QSession 则由外层 QSession 初始化参数
    var sessionConfig: String = ""
) : IRecordOption {
    override fun toString(): String {
        return "QAudioRecordOption(audioCfg='$audioCfg', qSession=$qSession, sessionConfig='$sessionConfig')"
    }
}