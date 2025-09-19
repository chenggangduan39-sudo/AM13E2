package com.qdreamer.qvoice.record.impl;

import com.qdreamer.qvoice.QAudio;
import com.qdreamer.qvoice.QSession;
import com.qdreamer.qvoice.listener.OnAudioRecordListener;
import com.qdreamer.qvoice.listener.OnQVoiceCallback;
import com.qdreamer.qvoice.record.IRecordOption;
import com.qdreamer.qvoice.record.IRecordPresenter;

import java.nio.ByteBuffer;
import java.util.Arrays;

public class QAudioRecordPresenter implements IRecordPresenter, OnQVoiceCallback {

    static {
        System.loadLibrary("Qvoice");
    }

    private QSession qSession = null;
    private QAudio qAudio = null;
    private long audioValue = 0L;
    private boolean isRecording = false;

    private OnAudioRecordListener readListener = null;
    private Thread readThread = null;

    @Override
    public void initRecorder(IRecordOption option) {
        if (!(option instanceof QAudioRecordOption)) {
            throw new RuntimeException("录音机配置参数异常");
        }
        if (qAudio != null) {
            releaseRecorder();
        }
        qAudio = new QAudio();
        Long sessionValue = ((QAudioRecordOption) option).getQSession();
        if (null == sessionValue || sessionValue == 0L) {
            qSession = new QSession(((QAudioRecordOption) option).getSessionConfig(), this);
            sessionValue = qSession.getSession();
        }
        audioValue = qAudio.init(sessionValue, ((QAudioRecordOption) option).getAudioCfg());
    }

    @Override
    public boolean startRecorder(OnAudioRecordListener listener) {
        if (qAudio == null) {
            return false;
        }
        readListener = listener;
        if (!isRecording) {
            isRecording = true;
            qAudio.recorderStart(audioValue);
            readAudioData();
        }
        return true;
    }

    @Override
    public void stopRecorder() {
        readListener = null;
        stopReadAudio();
        if (qAudio != null && isRecording) {
            isRecording = false;
            qAudio.recorderStop(audioValue);
        }
    }

    @Override
    public void releaseRecorder() {
        stopRecorder();
        if (qAudio != null) {
            qAudio.exit(audioValue);
            qAudio = null;
        }
        if (qSession != null) {
            qSession.releaseSession();
            qSession = null;
        }
    }

    private void stopReadAudio() {
        if (readThread != null && readThread.isAlive() && !readThread.isInterrupted()) {
            readThread.interrupt();
            readThread = null;
        }
    }

    private void readAudioData() {
        stopReadAudio();
        readThread = new Thread(this::readAudioOld);
        readThread.start();
    }

    private void readAudioOld() {
        try {
            while (qAudio != null && isRecording) {
                byte[] buffer = qAudio.recorderRead(audioValue);
                if (readListener != null && buffer != null) {
                    readListener.onRecordAudio(buffer);
                }
                Thread.sleep(1);
            }
        } catch (Exception ignore) {
        }
    }

    private void readAudioNew() {
        try {
            // JNI 需要 DirectByteBuffer 而不是 HeapByteBuffer（ByteBuffer.allocate()），一般一通道一帧 1024 字节，最多 8 通道
            ByteBuffer buffer = ByteBuffer.allocateDirect(1024 * 8);
            while (qAudio != null && isRecording) {
                int len = qAudio.recorderRead(audioValue, buffer);
                // DirectByteBuffer 前面会多出 4 个字节，后面也会多出 3 个字节；即创建的字节长度为 4096；但是 array 获取到的数据是 4103 长度
                if (readListener != null && len > 7) {
                    readListener.onRecordAudio(Arrays.copyOfRange(buffer.array(), 4, 4 + len));
                }
                buffer.clear();
                Thread.sleep(1);
            }
        } catch (Exception ignore) {}
    }

    @Override
    public void onQVoiceCallback(byte type, byte[] data) {

    }
}
