package com.qdreamer.qvoice.record.impl

import com.qdreamer.qvoice.QAudio
import com.qdreamer.qvoice.QSession
import com.qdreamer.qvoice.listener.OnAudioRecordListener
import com.qdreamer.qvoice.listener.OnQVoiceCallback
import com.qdreamer.qvoice.record.IRecordOption
import com.qdreamer.qvoice.record.IRecordPresenter
import kotlinx.coroutines.*
import java.nio.ByteBuffer

class QAudioRecordPresenter : IRecordPresenter, OnQVoiceCallback {

    private companion object {
        init {
            System.loadLibrary("Qvoice");
        }
    }

    private var qSession: QSession? = null
    private var qAudio: QAudio? = null
    private var audioValue: Long = 0L
    private var isRecording: Boolean = false

    private var readListener: OnAudioRecordListener? = null
    private var readJob: Job? = null

    override fun initRecorder(option: IRecordOption) {
        if (option !is QAudioRecordOption) {
            throw RuntimeException("录音机配置参数异常")
        }
        if (qAudio != null) {
            releaseRecorder()
        }
        qAudio = QAudio()
        var sessionValue = option.qSession
        if (sessionValue == null || sessionValue == 0L) {
            qSession = QSession(option.sessionConfig, this)
            sessionValue = qSession!!.session
        }
        audioValue = qAudio!!.init(sessionValue, option.audioCfg)
    }

    override fun startRecorder(listener: OnAudioRecordListener?): Boolean {
        if (qAudio == null) {
            return false
        }
        readListener = listener
        if (!isRecording) {
            isRecording = true
            qAudio!!.recorderStart(audioValue)
            readAudioData()
        }
        return true
    }

    override fun stopRecorder() {
        readListener = null
        readJob?.cancel()
        if (qAudio != null && isRecording) {
            isRecording = false
            qAudio!!.recorderStop(audioValue)
        }
    }

    override fun releaseRecorder() {
        stopRecorder()
        qAudio?.exit(audioValue)
        qAudio = null
        qSession?.releaseSession()
        qSession = null
    }

    private fun readAudioData() {
        readJob?.cancel()
        readJob = GlobalScope.launch(Dispatchers.IO) {
            readAudioOld(this)
        }
    }

    private fun readAudioOld(scope: CoroutineScope) {
        while (qAudio != null && isRecording && scope.isActive) {
            val buffer: ByteArray = qAudio!!.recorderRead(audioValue)
            readListener?.onRecordAudio(buffer)
            Thread.sleep(1)
        }
    }

    private fun readAudioNew(scope: CoroutineScope) {
        // JNI 需要 DirectByteBuffer 而不是 HeapByteBuffer（ByteBuffer.allocate()），一般一通道一帧 1024 字节，最多 8 通道
        val buffer = ByteBuffer.allocateDirect(1024 * 8)
        while (qAudio != null && isRecording && scope.isActive) {
            val len = qAudio!!.recorderRead(audioValue, buffer)
            // DirectByteBuffer 前面会多出 4 个字节，后面也会多出 3 个字节；即创建的字节长度为 4096；但是 array 获取到的数据是 4103 长度
            if (len > 7) {
                readListener?.onRecordAudio(buffer.array().copyOfRange(4, 4 + len))
            }
            buffer.clear()
            Thread.sleep(1)
        }
    }

    override fun onQVoiceCallback(type: Byte, data: ByteArray?) {
    }

}