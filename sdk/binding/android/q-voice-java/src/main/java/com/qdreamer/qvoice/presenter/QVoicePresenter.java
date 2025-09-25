package com.qdreamer.qvoice.presenter;

import android.content.Context;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.text.TextUtils;

import com.qdreamer.qvoice.QEngine;
import com.qdreamer.qvoice.QSession;
import com.qdreamer.qvoice.constants.QVoiceCode;
import com.qdreamer.qvoice.listener.OnAudioRecordListener;
import com.qdreamer.qvoice.listener.OnQVoiceCallback;
import com.qdreamer.qvoice.record.IRecordOption;
import com.qdreamer.qvoice.record.IRecordPresenter;
import com.qdreamer.qvoice.record.impl.AudioRecordOption;
import com.qdreamer.qvoice.record.impl.AudioRecordPresenter;
import com.qdreamer.qvoice.record.impl.QAudioRecordOption;
import com.qdreamer.qvoice.record.impl.QAudioRecordPresenter;

import java.nio.charset.StandardCharsets;

/**
 * @Author: JinYx
 * @Create: 2023-12-18 10:39:39
 * @Signature: 不属于这个时代的愚者；灰雾之上的神秘主宰；执掌好运的黄黑之王。
 */
public class QVoicePresenter implements IQVoicePresenter, OnQVoiceCallback, OnAudioRecordListener {

    static {
        // 一般而言都是打的 libQvoice.so；如果是其他名称的库文件，1、改库文件名称 或 2、改此处 loadLibrary 名称
        System.loadLibrary("Qvoice");
    }

    private String sessionConfig;
    private Long sessionValue = null;
    private final String engineConfig;
    private final IRecordOption recordOption;

    private QSession qSession;
    private QEngine qEngine;
    private IRecordPresenter recordPresenter;

    private OnQVoiceCallback qVoiceCallback;
    private Thread callbackThread;
    private CallbackHandler callbackHandler;

    /**
     * @param sessionConfig 初始化 QSession 参数
     * @param engineConfig 初始化 QEngine 参数
     */
    public QVoicePresenter(String sessionConfig, String engineConfig) {
        this(sessionConfig, engineConfig, null);
    }

    /**
     * @param appId    联系奇梦者申请的应用 Id
     * @param appKey   对应引用 Id 的密钥
     * @param engineConfig 需要初始化的语音功能对应参数，由奇梦者提供
     */
    public QVoicePresenter(Context context, String appId, String appKey, String engineConfig) {
        this(context, appId, appKey, engineConfig, "", null);
    }

    /**
     * @param appId    联系奇梦者申请的应用 Id
     * @param appKey   对应引用 Id 的密钥
     * @param engineConfig 需要初始化的语音功能对应参数，由奇梦者提供
     * @param sessionAttachmentParam appId 初始化时额外的信息，一般默认空字符串即可
     */
    public QVoicePresenter(Context context, String appId, String appKey, String engineConfig, String sessionAttachmentParam) {
        this(context, appId, appKey, engineConfig, sessionAttachmentParam, null);
    }

    /**
     * @param appId    联系奇梦者申请的应用 Id
     * @param appKey   对应引用 Id 的密钥
     * @param engineConfig 需要初始化的语音功能对应参数，由奇梦者提供
     * @param recordOption 初始化录音机参数，默认 null 表示不创建内置录音机，该接口有两个实现类，分别用于创建系统录音机和 tinyalsa 声卡录音机
     */
    public QVoicePresenter(Context context, String appId, String appKey, String engineConfig, IRecordOption recordOption) {
        this(context, appId, appKey, engineConfig, "", recordOption);
    }

    /**
     * @param appId    联系奇梦者申请的应用 Id
     * @param appKey   对应引用 Id 的密钥
     * @param engineConfig 需要初始化的语音功能对应参数，由奇梦者提供
     * @param sessionAttachmentParam appId 初始化时额外的信息，一般默认空字符串即可
     * @param recordOption 初始化录音机参数，默认 null 表示不创建内置录音机，该接口有两个实现类，分别用于创建系统录音机和 tinyalsa 声卡录音机
     */
    public QVoicePresenter(Context context, String appId, String appKey, String engineConfig, String sessionAttachmentParam, IRecordOption recordOption) {
        this(String.format(
            "appid=%s;secretkey=%s;cache_path=%s;" + sessionAttachmentParam, appId, appKey, context.getExternalFilesDir(null).getAbsolutePath()
        ), engineConfig, recordOption);
    }

    /**
     * @param sessionConfig 初始化 QSession 参数
     * @param engineConfig 初始化 QEngine 参数
     * @param recordOption 初始化录音机参数，默认 null 表示不创建内置录音机，该接口有两个实现类，分别用于创建系统录音机和 tinyalsa 声卡录音机
     */
    public QVoicePresenter(String sessionConfig, String engineConfig, IRecordOption recordOption) {
        this.sessionConfig = sessionConfig;
        this.sessionValue = null;
        this.engineConfig = engineConfig;
        this.recordOption = recordOption;
        createCallbackHandler();
    }

    public QVoicePresenter(long sessionValue, String engineConfig) {
        this(sessionValue, engineConfig, null);
    }

    public QVoicePresenter(long sessionValue, String engineConfig, IRecordOption recordOption) {
        this.sessionValue = sessionValue;
        this.engineConfig = engineConfig;
        this.recordOption = recordOption;
        createCallbackHandler();
    }

    private void createCallbackHandler() {
        callbackThread = new Thread(() -> {
            Looper.prepare();
            callbackHandler = new CallbackHandler(Looper.myLooper());
            Looper.loop();
        });
        callbackThread.start();
    }

    private void sendMessageToHandler(int what, byte[] obj) {
        if (callbackHandler != null) {
            callbackHandler.obtainMessage(what, obj).sendToTarget();
        }
    }

    @Override
    public void setOnQVoiceCallback(OnQVoiceCallback listener) {
        qVoiceCallback = listener;
    }

    @Override
    public void initQEngine() {
        // 等待 Handler 创建完成
        while (callbackHandler == null) {
            try { Thread.sleep(1); } catch (InterruptedException ignore) {}
        }
        if (sessionValue == null || sessionValue == 0L) {
            if (TextUtils.isEmpty(sessionConfig)) {
                sendMessageToHandler(QVoiceCode.QTK_JNI_INIT_FAILED, ("init session param error {sessionValue: " + sessionValue + ", sessionConfig: " + sessionConfig + "}").getBytes(StandardCharsets.UTF_8));
            }
            qSession = new QSession(sessionConfig, this);
        }
        if (getSessionValue() == 0L) {
            sendMessageToHandler(QVoiceCode.QTK_JNI_INIT_FAILED, ("init session failed {" + sessionConfig + "}").getBytes(StandardCharsets.UTF_8));
            qSession.releaseSession();
            qSession = null;
        } else {
            qEngine = new QEngine(getSessionValue(), engineConfig, this);
            if (qEngine.getEngineValue() == 0L){
                sendMessageToHandler(QVoiceCode.QTK_JNI_INIT_FAILED, ("init engine failed {" + engineConfig + "}").getBytes(StandardCharsets.UTF_8));
                qEngine.releaseEngine();
                qEngine = null;
                qSession.releaseSession();
                qSession = null;
            } else {
                try {
                    if (recordOption != null && recordOption instanceof AudioRecordOption) {
                        recordPresenter = new AudioRecordPresenter();
                        recordPresenter.initRecorder(recordOption);
                    } else if (recordOption != null && recordOption instanceof QAudioRecordOption) {
                        recordPresenter = new QAudioRecordPresenter();
                        ((QAudioRecordOption) recordOption).setQSession(getSessionValue());
                        ((QAudioRecordOption) recordOption).setSessionConfig("");
                        recordPresenter.initRecorder(recordOption);
                    }
                    sendMessageToHandler(QVoiceCode.QTK_JNI_INIT_SUCCESS, new byte[0]);
                } catch (Exception e) {
                    sendMessageToHandler(QVoiceCode.QTK_JNI_INIT_FAILED, ("init record failed {" + (recordOption == null ? "null" : recordOption.toString()) + "}").getBytes(StandardCharsets.UTF_8));
                    if (recordPresenter != null) {
                        recordPresenter.releaseRecorder();
                        recordPresenter = null;
                    }
                    qEngine.releaseEngine();
                    qEngine = null;
                    qSession.releaseSession();
                    qSession = null;
                }
            }
        }
    }

    @Override
    public boolean startQEngine() {
        boolean started = qEngine != null && qEngine.startEngine();
        if (started && recordPresenter != null) {
            recordPresenter.startRecorder(this);
        }
        return started;
    }

    @Override
    public void feedQEngine(byte[] audio) {
        if (qEngine != null) {
            qEngine.feedEngine(audio);
        }
    }

    @Override
    public boolean stopQEngine() {
        if (recordPresenter != null) {
            recordPresenter.stopRecorder();
        }
        return qEngine != null && qEngine.stopEngine();
    }

    @Override
    public void releaseQEngine() {
        qVoiceCallback = null;
        if (callbackThread != null && callbackThread.isAlive() && !callbackThread.isInterrupted()) {
            callbackThread.interrupt();
            callbackThread = null;
        }
        if (callbackHandler != null) {
            callbackHandler.removeCallbacksAndMessages(null);
            callbackHandler = null;
        }
        if (recordPresenter != null) {
            recordPresenter.releaseRecorder();
            recordPresenter = null;
        }
        if (qEngine != null) {
            qEngine.releaseEngine();
            qEngine = null;
        }
        if (qSession != null) {
            qSession.releaseSession();
            qSession = null;
        }
    }

    @Override
    public long getSessionValue() {
        return (sessionValue != null && sessionValue != 0L) ? sessionValue : (qSession == null ? 0L : qSession.getSession());
    }

    @Override
    public void onRecordAudio(byte[] audio) {
        if (callbackHandler != null) {
            callbackHandler.obtainMessage(QVoiceCode.RECORD_AUDIO_DATA, audio).sendToTarget();
        }
        if (qEngine != null) {
            qEngine.feedEngine(audio);
        }
    }

    @Override
    public void onQVoiceCallback(byte type, byte[] data) {
        sendMessageToHandler(type, data);
    }

    private class CallbackHandler extends Handler {
        CallbackHandler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg) {
            super.handleMessage(msg);
            if (qVoiceCallback != null) {
                qVoiceCallback.onQVoiceCallback((byte) msg.what, (byte[]) msg.obj);
            }
        }
    }

}
