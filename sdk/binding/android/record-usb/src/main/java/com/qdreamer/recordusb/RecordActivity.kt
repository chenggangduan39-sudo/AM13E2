package com.qdreamer.recordusb

import android.Manifest
import android.os.Bundle
import android.view.View
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity
import com.qdreamer.qvoice.listener.OnAudioRecordListener
import com.qdreamer.qvoice.record.IRecordPresenter
import com.qdreamer.qvoice.record.impl.*
import kotlinx.android.synthetic.main.activity_record.*
import permissions.dispatcher.NeedsPermission
import permissions.dispatcher.OnPermissionDenied
import permissions.dispatcher.RuntimePermissions
import java.io.File

/**
 * @Author: JinYx
 * @Create: 2024-03-01 19:00:26
 * @Signature: 不属于这个时代的愚者；灰雾之上的神秘主宰；执掌好运的黄黑之王。
 */
@RuntimePermissions
class RecordActivity : AppCompatActivity(), View.OnClickListener, OnAudioRecordListener {

    private companion object {
        private const val SESSION_CONFIG = "appid=0;secretkey=0;cache_path=%s;"
        private const val AUDIO_CFG = "audio.cfg"

        // 单通道、16k采样率，每毫秒32字节音频数据
        private const val AUDIO_PER_MILL = 32

        private const val USE_USB_RECORD = false
    }

    private var recordPresenter: IRecordPresenter? = null
    private var isRecording = false
    private var audioFile: File? = null

    private var audioCfg: File? = null

    private var totalAudioLen: Long = 0
    private var lastAudioSec = 0L

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_record)

        applyPermissionWithPermissionCheck()
    }

    @NeedsPermission(Manifest.permission.RECORD_AUDIO, Manifest.permission.WRITE_EXTERNAL_STORAGE)
    fun applyPermission() {
        txtRecord.isEnabled = true
        txtRecord.setOnClickListener(this)

        audioFile = File(getExternalFilesDir(null)!!, "audio.pcm")
        audioFile!!.writeBytes(byteArrayOf())

        if (USE_USB_RECORD) {
            audioCfg = File(getExternalFilesDir(null)!!, "audio.cfg")
            if (!audioCfg!!.exists() || !audioCfg!!.isFile) {
                assets.open(AUDIO_CFG).use { input ->
                    audioCfg!!.writeBytes(input.readBytes())
                }
            }
        }
    }

    override fun onClick(v: View?) {
        if (v?.id == R.id.txtRecord) {
            if (isRecording) {
                recordPresenter?.stopRecorder()
                isRecording = false
                txtRecord.text = "开始录音"
            } else {
                if (recordPresenter == null) {
                    if (USE_USB_RECORD) {
                        // USB 录音机; 因考虑内网关系，不做联网验证；appid和secretKey 随便传个字符串就行
                        val sessionConfig = String.format(SESSION_CONFIG, filesDir.absolutePath)
                        recordPresenter = QAudioRecordPresenter()
                        recordPresenter!!.initRecorder(QAudioRecordOption(audioCfg!!.absolutePath, sessionConfig))
                    } else {    // 系统录音机
                        recordPresenter = AudioRecordPresenter()
                        recordPresenter!!.initRecorder(AudioRecordOption())
                    }
                }
                recordPresenter?.startRecorder(this)
                isRecording = true
                txtRecord.text = "暂停录音"
            }
        }
    }

    override fun onRecordAudio(audio: ByteArray) {
        totalAudioLen += audio.size
        val sec = totalAudioLen / AUDIO_PER_MILL / 1000
        if (sec != lastAudioSec) {
            lastAudioSec = sec
            runOnUiThread {
                txtTime.text = formatTime(sec)
            }
        }
        audioFile?.appendBytes(audio)
    }

    private fun formatTime(sec: Long): String {
        val min = sec / 60
        val sec2 = sec % 60
        return if (min > 60) {
            val hour = min / 60
            String.format("%02d:%02d:%02d", hour, min % 60, sec2)
        } else {
            String.format("%02d:%02d", min, sec2)
        }
    }

    @OnPermissionDenied(Manifest.permission.RECORD_AUDIO, Manifest.permission.WRITE_EXTERNAL_STORAGE)
    fun onPermissionDenied() {
        Toast.makeText(this, "需要录音和写入外部存储权限", Toast.LENGTH_LONG).show()
    }

    override fun onRequestPermissionsResult(
        requestCode: Int, permissions: Array<out String>, grantResults: IntArray
    ) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        onRequestPermissionsResult(requestCode, grantResults)
    }

    override fun onDestroy() {
        recordPresenter?.releaseRecorder()?.also { recordPresenter = null }
        super.onDestroy()
    }

}