package com.qdreamer.vad;

public class Vad {
    public native long create(String cfg);
    public native void destroy(long mod);
    public native void start(long mod);
    public native void reset(long mod);
    public native byte[] feed(long mod, byte[] data, int len);
    public native byte[] feed_end(long mod);
}
