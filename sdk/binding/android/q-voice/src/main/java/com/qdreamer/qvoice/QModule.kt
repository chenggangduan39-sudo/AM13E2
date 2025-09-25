package com.qdreamer.qvoice

import com.qdreamer.qvoice.listener.OnQVoiceCallback
import kotlinx.coroutines.*

class QModule(
    sessionValue: Long, param: String, resPath: String, private val callback: OnQVoiceCallback?
) {

    private companion object {
        private const val MODULE_VALUE = 0L
    }

    // private native long moduleNew(long sessionValue, String param, String resPath);
    private external fun moduleNew(sessionValue: Long, param: String, resPath: String): Long
    private external fun moduleDel(moduleValue: Long)
    private external fun moduleStart(moduleValue: Long): Int
    private external fun moduleSet(moduleValue: Long, params: String): Int
    private external fun moduleStop(moduleValue: Long): Int
    private external fun moduleRead(moduleValue: Long): ByteArray?
    private external fun moduleFeed(moduleValue: Long, data: ByteArray, len: Int = data.size): Int

    private var moduleValue: Long = MODULE_VALUE
    private var moduleJob: Job? = null

    private var isModuleStarted = false

    init {
        moduleValue = moduleNew(sessionValue, param, resPath)
        moduleJob?.cancel()
        moduleJob = GlobalScope.launch(Dispatchers.IO) {
            while (isActive && moduleValue != MODULE_VALUE) {
                val bytes = moduleRead(moduleValue)
                if (isActive && bytes != null && bytes.isNotEmpty()) {
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

    fun startModule(): Boolean {
        synchronized(isModuleStarted) {
            if (moduleValue != MODULE_VALUE && !isModuleStarted) {
                moduleStart(moduleValue)
                isModuleStarted = true
            }
            return isModuleStarted
        }
    }

    fun feedModule(audio: ByteArray) {
        if (moduleValue != MODULE_VALUE && isModuleStarted) {
            moduleFeed(moduleValue, audio, audio.size)
        }
    }

    fun stopModule(): Boolean {
        synchronized(isModuleStarted) {
            if (moduleValue != MODULE_VALUE && isModuleStarted) {
                moduleStop(moduleValue)
                isModuleStarted = false
            }
            return !isModuleStarted
        }
    }

    fun releaseModule() {
        stopModule()
        moduleJob?.cancel()
        val temp = moduleValue
        moduleValue = MODULE_VALUE
        moduleDel(temp)
    }

}