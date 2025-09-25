package com.qdreamer.qvoice

import android.util.Log
import com.qdreamer.qvoice.listener.OnQVoiceCallback
import kotlinx.coroutines.*

class QEngine(
    session: Long, config: String, private val callback: OnQVoiceCallback?
) {

    private companion object {
        private const val ENGINE_VALUE = 0L

        private const val FEED_DATA = 0
        private const val FEED_END = 1
    }

    /**
     * java 代码定义为 native
     * private native long newEngine(long session, String config)
     */
    private external fun newEngine(session: Long, config: String): Long
    private external fun startEngine(engineValue: Long): Int
    private external fun feedEngine(engineValue: Long, audioData: ByteArray, len: Int, end: Int): Int
    private external fun cancelEngine(engineValue: Long): Int
    private external fun setEngine(engineValue: Long, param: String): Int
    private external fun resetEngine(engineValue: Long): Int
    private external fun readEngine(engineValue: Long): ByteArray?
    private external fun deleteEngine(engineValue: Long): Int

    var engineValue: Long = ENGINE_VALUE
        private set
    private var engineJob: Job? = null

    private var isEngineStarted = false

    init {
        engineValue = newEngine(session, config)
        Log.e("DEBUG", "------> $engineValue ========> $config")
        engineJob?.cancel()
        engineJob = GlobalScope.launch(Dispatchers.IO) {
            while (isActive && engineValue != ENGINE_VALUE) {
                val bytes = readEngine(engineValue)
                if (isActive && engineValue != ENGINE_VALUE && bytes != null && bytes.isNotEmpty()) {
                    if (bytes.size > 1) {
                        callback?.onQVoiceCallback(bytes[0], bytes.copyOfRange(1, bytes.size))
                    } else {
                        callback?.onQVoiceCallback(bytes[0], null)
                    }
                }
                Thread.sleep(1)
            }
        }
    }

    fun startEngine(): Boolean {
        synchronized(isEngineStarted) {
            if (engineValue != ENGINE_VALUE && !isEngineStarted) {
                startEngine(engineValue)
                isEngineStarted = true
            }
            return isEngineStarted
        }
    }

    fun feedEngine(audio: ByteArray) {
        if (engineValue != ENGINE_VALUE && isEngineStarted) {
            feedEngine(engineValue, audio, audio.size, FEED_DATA)
        }
    }

    fun stopEngine(): Boolean {
        synchronized(isEngineStarted) {
            if (engineValue != ENGINE_VALUE && isEngineStarted) {
                feedEngine(engineValue, byteArrayOf(), 0, FEED_END)
                resetEngine(engineValue)
                isEngineStarted = false
            }
            return !isEngineStarted
        }
    }

    fun setQEngine(param: String) {
        setEngine(engineValue, param)
    }

    fun releaseEngine() {
        stopEngine()
        engineJob?.cancel()
        val temp = engineValue
        engineValue = ENGINE_VALUE
        deleteEngine(temp)
    }

}