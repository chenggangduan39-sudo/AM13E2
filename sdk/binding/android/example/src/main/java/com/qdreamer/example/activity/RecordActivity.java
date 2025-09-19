package com.qdreamer.example.activity;

import android.Manifest;
import android.media.AudioFormat;
import android.media.MediaRecorder;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;

import com.qdreamer.example.R;
import com.qdreamer.example.utils.FileUtils;
import com.qdreamer.qvoice.constants.QVoiceCode;
import com.qdreamer.qvoice.listener.OnQVoiceCallback;
import com.qdreamer.qvoice.presenter.IQVoicePresenter;
import com.qdreamer.qvoice.presenter.QVoicePresenter;
import com.qdreamer.qvoice.record.IRecordOption;
import com.qdreamer.qvoice.record.impl.AudioRecordOption;
import com.qdreamer.qvoice.utils.CopyEngineResUtils;

import java.io.File;

import permissions.dispatcher.NeedsPermission;
import permissions.dispatcher.OnPermissionDenied;
import permissions.dispatcher.RuntimePermissions;

@RuntimePermissions
public class RecordActivity extends AppCompatActivity implements View.OnClickListener, OnQVoiceCallback {

    /**
     * 问号替换成对应项目申请的 appid 和 secretkey
     * cache_path 是用于存储 sdk 输出 qvoice.log 的文件绝对路径，例如 /sdcard/
     */
    private static final String SESSION_CONFIG = "appid=?;secretkey=?;cache_path=%s;";

    /**
     * role 替换成对应 sdk 的语音角色，例如识别的 asr
     * cfg 为对应 assets 中配置文件拷贝到本地的绝对路径，例如 /sdcard/asr.cfg 或 /sdcard/asr.bin
     * 除开这两个参数，是否有其他根据 SDK 给到的绝对
     */
    private static final String ENGINE_CONFIG = "role=?;cfg=%s;";

    private static final String CONFIG_FILE_NAME = "asr.cfg";

    private Button btnRecord;
    private TextView txtQVoiceResult;

    private IQVoicePresenter qVoicePresenter;
    private Thread initThread;

    private File audioFile;

    @Override
    protected void onCreate(@Nullable Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_record);
        btnRecord = findViewById(R.id.btnRecord);
        txtQVoiceResult = findViewById(R.id.txtQVoiceResult);
        RecordActivityPermissionsDispatcher.applyPermissionWithPermissionCheck(this);
    }

    @NeedsPermission({Manifest.permission.RECORD_AUDIO, Manifest.permission.WRITE_EXTERNAL_STORAGE})
    public void applyPermission() {
        initThread = new Thread(() -> {
            // step 1: 将 assets 目录下的资源文件拷贝到本地磁盘
            File sdkDir = new File(getExternalFilesDir(null), "sdk");
            try {
                // 用于将 assets 下的所有文件拷贝到 /sdcard/Android/data/${applicationId}/files/sdk/ 文件夹下
                CopyEngineResUtils.copyAssets2Local(this, "", sdkDir);
                // 用于将 assets/res 整个 res 文件[夹]拷贝到 /sdcard/Android/data/${applicationId}/files/sdk/ 文件夹下
//                CopyEngineResUtils.copyAssets2Local(this, "res", sdkDir);
            } catch (Exception e) {
                // TODO 资源拷贝失败
                return;
            }

            // step 2: 初始化参数
            String sessionConfig = String.format(SESSION_CONFIG, sdkDir.getAbsolutePath());
            // 此处配置文件的名称，路径根据实际 SDK 定
            File configFile = new File(sdkDir, CONFIG_FILE_NAME);
            if (!configFile.exists() || !configFile.isFile()) {
                // TODO 资源文件不存在，没拷贝成功或者文件名称错误
                return;
            }
            String engineConfig = String.format(ENGINE_CONFIG, configFile.getAbsolutePath());
            IRecordOption recordOption = initRecordOption();

            // step 3: 创建对象实例，初始化
            qVoicePresenter = new QVoicePresenter(sessionConfig, engineConfig, recordOption);
            qVoicePresenter.setOnQVoiceCallback(this);
            qVoicePresenter.initQEngine();
        });
        initThread.start();
    }

    /**
     * 初始化录音参数
     */
    private IRecordOption initRecordOption() {
        // 使用内置系统录音机, 默认无参构造参数如下
        return new AudioRecordOption(MediaRecorder.AudioSource.MIC, 16000, AudioFormat.CHANNEL_IN_MONO, AudioFormat.ENCODING_PCM_16BIT);

        // 使用内置 USB 录音机，需要传入 audio.cfg 的配置文件绝对路径，另外 audio.cfg 中配置的声卡对应权限需要处理
//        String audioConfigFilePath = "/sdcard/Android/data/{applicationId}/files/sdk/audio.cfg";
//        return new QAudioRecordOption(audioConfigFilePath, null);

        // 不使用内置录音机
//        return null;
    }

    @Override
    public void onClick(View view) {
        if (view.getId() == R.id.btnRecord) {
            if (getString(R.string.start_record).contentEquals(btnRecord.getText())) {
                qVoicePresenter.startQEngine();
                btnRecord.setText(R.string.stop_record);
            } else {
                qVoicePresenter.stopQEngine();
                btnRecord.setText(R.string.start_record);
            }
        }
    }

    @Override
    public void onQVoiceCallback(byte b, byte[] bytes) {
        switch (b) {
            case QVoiceCode.QTK_JNI_INIT_SUCCESS: {
                audioFile = new File(getExternalFilesDir(null), "audio.pcm");
                FileUtils.writeBytes(audioFile, new byte[0]);
                runOnUiThread(() -> {
                    btnRecord.setEnabled(true);
                    Toast.makeText(this, "初始化完成", Toast.LENGTH_LONG).show();
                });
            }
            break;
            case QVoiceCode.QTK_JNI_INIT_FAILED: {
                runOnUiThread(() -> {
                    Toast.makeText(this, "初始化失败", Toast.LENGTH_LONG).show();
                    Log.e("TAG", "初始化失败: " + new String(bytes));
                });
            }
            break;
            // 使用内置录音机时，录音数据回调
            case QVoiceCode.RECORD_AUDIO_DATA: {
                // 内置录音机不需要再调用 feed 接口传入音频，如果没使用内置录音机，需要调用 feed 接口传入音频数据
//                if (qVoicePresenter != null) {
//                    qVoicePresenter.feedQEngine(bytes);
//                }
                // 测试中如果有问题可以保存音频日志用于分析
                if (audioFile != null) {
                    FileUtils.appendBytes(audioFile, bytes);
                }
            }
            break;
            // SDK 底层错误日志回调
            case QVoiceCode.QTK_JNI_ERRCODE: {
                Log.e("TAG", "QVoice 错误码信息: " + new String(bytes));
            }
            break;
            // 上面类型是通用的数据，具体的语音功能回调结果通过下面的打印判定
            default:
                String content = new String(bytes != null ? bytes : new byte[0]);
                Log.i("TAG", "语音 SDK 回调类型: " + b + ", 对应类型的数据: " + content);
                break;
        }
    }

    @OnPermissionDenied({Manifest.permission.RECORD_AUDIO, Manifest.permission.WRITE_EXTERNAL_STORAGE})
    public void onPermissionDenied() {
        Toast.makeText(this, "没有权限，无法使用 SDK", Toast.LENGTH_LONG).show();
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        RecordActivityPermissionsDispatcher.onRequestPermissionsResult(this, requestCode, grantResults);
    }

    @Override
    protected void onDestroy() {
        if (initThread != null && initThread.isAlive() && !initThread.isInterrupted()) {
            initThread.interrupt();
            initThread = null;
        }
        btnRecord.setEnabled(false);
        if (qVoicePresenter != null) {
            qVoicePresenter.releaseQEngine();
            qVoicePresenter = null;
        }
        super.onDestroy();
    }
}
