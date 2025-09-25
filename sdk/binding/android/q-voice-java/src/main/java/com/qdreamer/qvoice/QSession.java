package com.qdreamer.qvoice;

import android.os.Build;
import android.text.TextUtils;
import android.util.Log;

import com.qdreamer.qvoice.listener.OnQVoiceCallback;

import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.Arrays;

public class QSession {

    private static final long SESSION_VALUE = 0L;
    private static final String DEFAULT_USER_ID = getMD5(Build.BRAND + Build.DEVICE + Build.ID);

    private static String qSessionUserId = DEFAULT_USER_ID;

    private OnQVoiceCallback qVoiceCallback;
    private long session = SESSION_VALUE;
    private Thread sessionThread;

    private native long init(String config);
    private native byte[] sessionRead(long session);
    private native void exit(long session);

    public QSession(String config, OnQVoiceCallback callback) {
        qVoiceCallback = callback;
        Log.d("hw", Build.BRAND + " _ " + Build.DEVICE + " _ " + Build.ID);
        session = init(config);
        sessionThread = new Thread(() -> {
            try {
                while (session != SESSION_VALUE) {
                    byte[] bytes = sessionRead(session);
                    if (qVoiceCallback != null && session != SESSION_VALUE && bytes != null && bytes.length > 0) {
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
        sessionThread.start();
    }

    public long getSession() {
        return session;
    }

    public void releaseSession() {
        qVoiceCallback = null;
        long temp = session;
        session = SESSION_VALUE;
        if (sessionThread != null && sessionThread.isAlive() && !sessionThread.isInterrupted()) {
            sessionThread.interrupt();
            sessionThread = null;
        }
        if (temp != SESSION_VALUE) {
            exit(temp);
        }
    }

    public static void setUserId(String userId) {
        qSessionUserId = userId;
    }

    private static String UserIDGET() {
        return qSessionUserId;
    }

    private static String getMD5(String content) {
        if (TextUtils.isEmpty(content)) {
            return "";
        }
        MessageDigest md5;
        try {
            md5 = MessageDigest.getInstance("MD5");
            byte[] bytes = md5.digest(content.getBytes());
            StringBuilder result = new StringBuilder();
            for (byte b : bytes) {
                String temp = Integer.toHexString(b & 0xff);
                if (temp.length() == 1) {
                    temp = "0" + temp;
                }
                result.append(temp);
            }
            return result.toString().toLowerCase();
        } catch (NoSuchAlgorithmException e) {
            return BuildConfig.LIBRARY_PACKAGE_NAME;
        }
    }

}
