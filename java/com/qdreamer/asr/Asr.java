package com.qdreamer.asr;

public class Asr {
    public native long create(String cfg);
    public native void destroy(long mod);
    public native void start(long mod);
    public native void reset(long mod);
    public native String get_result(long mod);
    public native String get_hint_result(long mod);
    public native void feed(long mod, byte[] data, int len);
    public native void feed_end(long mod);
}
