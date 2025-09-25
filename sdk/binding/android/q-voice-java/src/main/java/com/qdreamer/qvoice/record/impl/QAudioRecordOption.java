package com.qdreamer.qvoice.record.impl;

import com.qdreamer.qvoice.record.IRecordOption;

public class QAudioRecordOption implements IRecordOption {

    /**
     * audio.cfg 配置文件相关的 QAudio 初始化参数
     */
    private String audioCfg;

    /**
     * QSession.init 返回值，不传则由录音机内部初始化 QSession
     */
    private Long qSession = null;

    /**
     * 由录音机内部初始化 QSession 则由外层 QSession 初始化参数
     */
    private String sessionConfig = "";

    public QAudioRecordOption(String audioCfg, long qSession) {
        this(audioCfg, qSession, "");
    }

    public QAudioRecordOption(String audioCfg, String sessionConfig) {
        this(audioCfg, null, sessionConfig);
    }

    private QAudioRecordOption(String audioCfg, Long qSession, String sessionConfig) {
        this.audioCfg = audioCfg;
        this.qSession = qSession;
        this.sessionConfig = sessionConfig;
    }

    public String getAudioCfg() {
        return audioCfg;
    }

    public Long getQSession() {
        return qSession;
    }

    public void setQSession(Long qSession) {
        this.qSession = qSession;
    }

    public String getSessionConfig() {
        return sessionConfig;
    }

    public void setSessionConfig(String sessionConfig) {
        this.sessionConfig = sessionConfig;
    }

    @Override
    public String toString() {
        return "QAudioRecordOption{" +
                "audioCfg='" + audioCfg + '\'' +
                ", qSession=" + qSession +
                ", sessionConfig='" + sessionConfig + '\'' +
                '}';
    }
}
