package com.qdreamer.qvoice.record.impl

import android.annotation.SuppressLint
import android.media.AudioRecord
import com.qdreamer.qvoice.listener.OnAudioRecordListener
import com.qdreamer.qvoice.record.IRecordOption
import com.qdreamer.qvoice.record.IRecordPresenter
import kotlinx.coroutines.*

class AudioRecordPresenter : IRecordPresenter {

    private var audioRecord: AudioRecord? = null
    private var bufferSize: Int = 0

    private var readListener: OnAudioRecordListener? = null
    private var readJob: Job? = null

    @SuppressLint("MissingPermission")
    override fun initRecorder(option: IRecordOption) {
        if (option !is AudioRecordOption) {
            throw RuntimeException("录音机配置参数异常")
        }
        if (audioRecord != null) {
            releaseRecorder()
        }
        bufferSize = AudioRecord.getMinBufferSize(option.sample, option.channel, option.format)
        audioRecord = AudioRecord(option.source, option.sample, option.channel, option.format, bufferSize)
    }

    override fun startRecorder(listener: OnAudioRecordListener?): Boolean {
        return if (audioRecord != null && audioRecord!!.state == AudioRecord.STATE_INITIALIZED) {
            if (audioRecord!!.recordingState != AudioRecord.RECORDSTATE_RECORDING) {
                audioRecord!!.startRecording()
            }
            readListener = listener
            readAudioData()
            true
        } else {
            false
        }
    }

    override fun stopRecorder() {
        readListener = null
        readJob?.cancel()
        if (audioRecord != null && audioRecord!!.state == AudioRecord.STATE_INITIALIZED
            && audioRecord!!.recordingState == AudioRecord.RECORDSTATE_RECORDING) {
            audioRecord!!.stop()
        }
    }

    override fun releaseRecorder() {
        stopRecorder()
        audioRecord?.release()
    }

    private fun readAudioData() {
        readJob?.cancel()
        readJob = GlobalScope.launch(Dispatchers.IO) {
            while (audioRecord != null && audioRecord!!.state == AudioRecord.STATE_INITIALIZED
                && audioRecord!!.recordingState == AudioRecord.RECORDSTATE_RECORDING && isActive) {
                val buffer = ByteArray(bufferSize)
                val len = audioRecord!!.read(buffer, 0, bufferSize)
                if (len == bufferSize) {
                    readListener?.onRecordAudio(buffer)
                } else if (len > 0) {
                    readListener?.onRecordAudio(buffer.copyOfRange(0, len))
//                } else if (len < 0) {
                    // 录音失败
                }
                Thread.sleep(1)
            }
        }
    }

}