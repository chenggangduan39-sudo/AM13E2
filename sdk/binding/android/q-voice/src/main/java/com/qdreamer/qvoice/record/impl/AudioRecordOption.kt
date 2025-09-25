package com.qdreamer.qvoice.record.impl

import android.media.AudioFormat
import android.media.MediaRecorder
import com.qdreamer.qvoice.record.IRecordOption

data class AudioRecordOption(
    val source: Int = MediaRecorder.AudioSource.MIC,
    val sample: Int = 16000,
    val channel: Int = AudioFormat.CHANNEL_IN_MONO,
    val format: Int = AudioFormat.ENCODING_PCM_16BIT
) : IRecordOption {
    override fun toString(): String {
        return "AudioRecordOption(source=$source, sample=$sample, channel=$channel, format=$format)"
    }
}