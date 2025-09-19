package com.qdreamer.qvoice;

import com.qdreamer.qvoice.listener.OnQVoiceCallback;

import java.util.Arrays;

public class QModule {

    private static final long MODULE_VALUE = 0L;

    private OnQVoiceCallback qVoiceCallback;
    private long moduleValue = MODULE_VALUE;
    private boolean isModuleStarted = false;
    private Thread moduleThread = null;

    private native long moduleNew(long sessionValue, String param, String resPath);
    private native void moduleDel(long moduleValue);
    private native int moduleStart(long moduleValue);
    private native int moduleSet(long moduleValue, String param);
    private native int moduleStop(long moduleValue);
    private native byte[] moduleRead(long moduleValue);
    private native int moduleFeed(long moduleValue, byte[] data, int len);

    public QModule(long sessionValue, String param, String resPath, OnQVoiceCallback callback) {
        this.qVoiceCallback = callback;

        moduleValue = moduleNew(sessionValue, param, resPath);
        moduleThread = new Thread(() -> {
            try {
                while (moduleValue != MODULE_VALUE) {
                    byte[] bytes = moduleRead(moduleValue);
                    if (qVoiceCallback != null && moduleValue != MODULE_VALUE && bytes != null && bytes.length > 0) {
                        if (bytes.length > 1) {
                            qVoiceCallback.onQVoiceCallback(bytes[0], Arrays.copyOfRange(bytes, 1, bytes.length));
                        } else {
                            qVoiceCallback.onQVoiceCallback(bytes[0], null);
                        }
                    }
                    Thread.sleep(1);
                }
            } catch (Exception ignore) {
            }
        });
        moduleThread.start();
    }

    public boolean startModule() {
        synchronized (this) {
            if (moduleValue != MODULE_VALUE && !isModuleStarted) {
                moduleStart(moduleValue);
                isModuleStarted = true;
            }
            return isModuleStarted;
        }
    }

    public void feedModule(byte[] audio) {
        if (moduleValue != MODULE_VALUE && isModuleStarted) {
            if (audio == null) {
                moduleFeed(moduleValue, new byte[0], 0);
            } else {
                moduleFeed(moduleValue, audio, audio.length);
            }
        }
    }

    public boolean stopModule() {
        synchronized (this) {
            if (moduleValue != MODULE_VALUE && isModuleStarted) {
                moduleStop(moduleValue);
                isModuleStarted = false;
            }
            return !isModuleStarted;
        }
    }

    public void releaseModule() {
        stopModule();
        qVoiceCallback = null;
        long temp = moduleValue;
        moduleValue = MODULE_VALUE;
        if (moduleThread != null && moduleThread.isAlive() && !moduleThread.isInterrupted()) {
            moduleThread.interrupt();
            moduleThread = null;
        }
        moduleDel(temp);
    }

}
