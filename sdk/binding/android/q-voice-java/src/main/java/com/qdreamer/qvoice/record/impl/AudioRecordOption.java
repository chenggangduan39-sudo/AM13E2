package com.qdreamer.qvoice.record.impl;

import android.media.AudioFormat;
import android.media.MediaRecorder;

import com.qdreamer.qvoice.record.IRecordOption;

public class AudioRecordOption implements IRecordOption {

    private static final int DEFAULT_SOURCE = MediaRecorder.AudioSource.MIC;
    private static final int DEFAULT_SAMPLE = 16000;
    private static final int DEFAULT_CHANNEL = AudioFormat.CHANNEL_IN_MONO;
    private static final int DEFAULT_FORMAT = AudioFormat.ENCODING_PCM_16BIT;

    private final int source;
    private final int sample;
    private final int channel;
    private final int format;

    public AudioRecordOption() {
        this(DEFAULT_SOURCE, DEFAULT_SAMPLE, DEFAULT_CHANNEL, DEFAULT_FORMAT);
    }

    public AudioRecordOption(int source, int sample, int channel, int format) {
        this.source = source;
        this.sample = sample;
        this.channel = channel;
        this.format = format;
    }

    public int getSource() {
        return source;
    }

    public int getSample() {
        return sample;
    }

    public int getChannel() {
        return channel;
    }

    public int getFormat() {
        return format;
    }

    @Override
    public String toString() {
        return "AudioRecordOption{" +
                "source=" + source +
                ", sample=" + sample +
                ", channel=" + channel +
                ", format=" + format +
                '}';
    }
}
