package com.qdreamer.qvoice

import android.os.Build
import android.util.Log
import com.qdreamer.qvoice.listener.OnQVoiceCallback
import kotlinx.coroutines.*
import java.security.MessageDigest
import java.security.NoSuchAlgorithmException

class QSession constructor(config: String, private val callback: OnQVoiceCallback?) {

    companion object {
        private const val SESSION_VALUE = 0L

        private var qSessionUserId: String = getMD5(Build.BRAND + Build.DEVICE + Build.ID)

        @JvmStatic
        fun setUserId(userId: String) {
            qSessionUserId = userId
        }

        @JvmStatic
        private fun UserIDGET() = qSessionUserId

        private fun getMD5(content: String?): String {
            if (content.isNullOrEmpty()) return ""
            val md5: MessageDigest
            try {
                md5 = MessageDigest.getInstance("MD5")
                val bytes = md5.digest(content.toByteArray())
                val result = StringBuilder()
                for (b in bytes) {
                    var temp = Integer.toHexString(b.toInt().and(0xff))
                    if (temp.length == 1) {
                        temp = "0$temp"
                    }
                    result.append(temp)
                }
                return result.toString()
            } catch (e: NoSuchAlgorithmException) {
                e.printStackTrace()
            }
            return BuildConfig.LIBRARY_PACKAGE_NAME
        }
    }

    private external fun init(config: String): Long
    private external fun sessionRead(session: Long): ByteArray?
    private external fun exit(session: Long)

    var session: Long = SESSION_VALUE
        private set

    private var sessionJob: Job? = null

    init {
        Log.d("hw", Build.BRAND + " _ " + Build.DEVICE + " _ " + Build.ID);
        session = init(config)
        sessionJob?.cancel()
        sessionJob = GlobalScope.launch(Dispatchers.IO) {
            while (isActive && session != SESSION_VALUE) {
                val bytes = sessionRead(session)
                if (isActive && session != SESSION_VALUE && bytes != null && bytes.isNotEmpty()) {
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

    fun releaseSession() {
        sessionJob?.cancel()
        if (session != SESSION_VALUE) {
            val temp = session
            session = SESSION_VALUE
            exit(temp)
        }
    }

}