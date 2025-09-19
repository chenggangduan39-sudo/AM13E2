package com.qdreamer.qvoice.presenter

import android.content.Context
import android.os.Handler
import android.os.Looper
import android.os.Message
import com.qdreamer.qvoice.QEngine
import com.qdreamer.qvoice.QSession
import com.qdreamer.qvoice.constants.QVoiceCode
import com.qdreamer.qvoice.listener.OnAudioRecordListener
import com.qdreamer.qvoice.listener.OnQVoiceCallback
import com.qdreamer.qvoice.record.IRecordOption
import com.qdreamer.qvoice.record.IRecordPresenter
import com.qdreamer.qvoice.record.impl.AudioRecordOption
import com.qdreamer.qvoice.record.impl.AudioRecordPresenter
import com.qdreamer.qvoice.record.impl.QAudioRecordOption
import com.qdreamer.qvoice.record.impl.QAudioRecordPresenter
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.GlobalScope
import kotlinx.coroutines.Job
import kotlinx.coroutines.launch

/**
 * @Author: JinYx
 * @Create: 2023-12-18 09:26:51
 * @Signature: 不属于这个时代的愚者；灰雾之上的神秘主宰；执掌好运的黄黑之王。
 */
class QVoicePresenter private constructor(
    private val sessionValue: Long?, private val sessionConfig: String,
    private val engineConfig: String, private val recordOption: IRecordOption? = null
) : IQVoicePresenter, OnQVoiceCallback, OnAudioRecordListener {

    private companion object {
        init {
            // 一般而言都是打的 libQvoice.so；如果是其他名称的库文件，1、改库文件名称 或 2、改此处 loadLibrary 名称
            System.loadLibrary("Qvoice");
        }
    }

    private var qSession: QSession? = null
    private var qEngine: QEngine? = null
    private var recordPresenter: IRecordPresenter? = null

    private var qVoiceCallback: OnQVoiceCallback? = null
    private var callbackJob: Job? = null
    private var callbackHandler: CallbackHandler? = null

    /**
     * [appId]    联系奇梦者申请的应用 Id
     * [appKey]   对应引用 Id 的密钥
     * [engineConfig] 需要初始化的语音功能对应参数，由奇梦者提供
     * [sessionAttachmentParam] appId 初始化时额外的信息，一般默认空字符串即可
     * [recordOption] 初始化录音机参数，默认 null 表示不创建内置录音机，该接口有两个实现类，分别用于创建系统录音机和 tinyalsa 声卡录音机
     */
    constructor(
        context: Context, appId: String, appKey: String, engineConfig: String,
        sessionAttachmentParam: String = "", recordOption: IRecordOption? = null
    ) : this(String.format(
        "appid=%s;secretkey=%s;cache_path=%s;$sessionAttachmentParam", appId, appKey, context.getExternalFilesDir(null)!!.absolutePath
    ), engineConfig, recordOption)

    /**
     * [sessionConfig] 初始化 QSession 参数
     * [engineConfig] 初始化 QEngine 参数
     * [recordOption] 初始化录音机参数，默认 null 表示不创建内置录音机，该接口有两个实现类，分别用于创建系统录音机和 tinyalsa 声卡录音机
     */
    constructor(sessionConfig: String, engineConfig: String, recordOption: IRecordOption? = null
    ): this(null, sessionConfig, engineConfig, recordOption)

    /**
     * [sessionValue] 其他 IQVoicePresenter 创建后，调用 getSessionValue 获取的 session 值，可以共用同一个 session，此处不在继续创建
     * [engineConfig] 初始化 QEngine 参数
     * [recordOption] 初始化录音机参数，默认 null 表示不创建内置录音机，该接口有两个实现类，分别用于创建系统录音机和 tinyalsa 声卡录音机
     */
    constructor(sessionValue: Long, engineConfig: String, recordOption: IRecordOption? = null
    ): this(sessionValue, "", engineConfig, recordOption)

    override fun setOnQVoiceCallback(listener: OnQVoiceCallback?) {
        qVoiceCallback = listener
    }

    init {
        callbackJob = GlobalScope.launch(Dispatchers.IO) {
            Looper.prepare()
            callbackHandler = CallbackHandler(Looper.myLooper()!!)
            Looper.loop()
        }
    }

    override fun initQEngine() {
        while (callbackHandler == null) {
            Thread.sleep(1)
        }
        if (sessionValue == null || sessionValue == 0L) {
            if (sessionConfig.trim().isEmpty()) {
                callbackHandler?.obtainMessage(QVoiceCode.QTK_JNI_INIT_FAILED.toInt(), "init session param error {sessionValue: $sessionValue, sessionConfig: $sessionConfig}".toByteArray())?.sendToTarget()
                return
            }
            qSession = QSession(sessionConfig, this)
        }
        if (getSessionValue() == 0L) {
            callbackHandler?.obtainMessage(QVoiceCode.QTK_JNI_INIT_FAILED.toInt(), "init session failed {$sessionConfig}".toByteArray())?.sendToTarget()
            qSession?.releaseSession()?.also { qSession = null }
        } else {
            qEngine = QEngine(getSessionValue(), engineConfig, this)
            if (qEngine!!.engineValue == 0L) {
                callbackHandler?.obtainMessage(QVoiceCode.QTK_JNI_INIT_FAILED.toInt(), "init engine failed {$engineConfig}".toByteArray())?.sendToTarget()
                qEngine?.releaseEngine()?.also { qEngine = null }
                qSession?.releaseSession()?.also { qSession = null }
            } else {
                try {
                    when (recordOption) {
                        is AudioRecordOption -> {
                            recordPresenter = AudioRecordPresenter()
                            recordPresenter!!.initRecorder(recordOption)
                        }
                        is QAudioRecordOption -> {
                            recordPresenter = QAudioRecordPresenter()
                            recordPresenter!!.initRecorder(QAudioRecordOption(recordOption.audioCfg, getSessionValue(), ""))
                        }
                    }
                    callbackHandler?.obtainMessage(QVoiceCode.QTK_JNI_INIT_SUCCESS.toInt(), byteArrayOf())?.sendToTarget()
                } catch (e: Exception) {
                    callbackHandler?.obtainMessage(QVoiceCode.QTK_JNI_INIT_FAILED.toInt(), "init record failed {${recordOption?.toString()}}".toByteArray())?.sendToTarget()
                    recordPresenter?.releaseRecorder()?.also { recordPresenter = null }
                    qEngine?.releaseEngine()?.also { qEngine = null }
                    qSession?.releaseSession()?.also { qSession = null }
                }
            }
        }
    }

    override fun startQEngine(): Boolean {
        return qEngine?.startEngine()?.also { isSuccess ->
            if (isSuccess && recordPresenter != null) {
                recordPresenter?.startRecorder(this)
            }
        } ?: false
    }

    override fun feedQEngine(audio: ByteArray) {
        qEngine?.feedEngine(audio)
    }

    override fun stopQEngine(): Boolean {
        recordPresenter?.stopRecorder()
        return qEngine?.stopEngine() ?: false
    }

    override fun releaseQEngine() {
        qVoiceCallback = null
        callbackJob?.cancel()
        callbackHandler?.removeCallbacksAndMessages(null)?.also { callbackHandler = null }
        recordPresenter?.releaseRecorder()?.also { recordPresenter = null }
        qEngine?.releaseEngine()?.also { qEngine = null }
        qSession?.releaseSession()?.also { qSession = null }
    }

    override fun getSessionValue(): Long {
        return if (sessionValue == null || sessionValue == 0L) {
            qSession?.session ?: 0L
        } else {
            sessionValue
        }
    }

    override fun onRecordAudio(audio: ByteArray) {
        callbackHandler?.obtainMessage(QVoiceCode.RECORD_AUDIO_DATA.toInt(), audio)?.sendToTarget()
        qEngine?.feedEngine(audio)
    }

    override fun onQVoiceCallback(type: Byte, data: ByteArray?) {
        callbackHandler?.obtainMessage(type.toInt(), data)?.sendToTarget()
    }

    private inner class CallbackHandler(looper: Looper) : Handler(looper) {
        override fun handleMessage(msg: Message) {
            super.handleMessage(msg)
            qVoiceCallback?.onQVoiceCallback(msg.what.toByte(), msg.obj as ByteArray?)
        }
    }
}