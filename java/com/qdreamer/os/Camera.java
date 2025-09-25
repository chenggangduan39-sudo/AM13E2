package com.qdreamer.os;

public class Camera {
    public native long create(String device, boolean blocking, int num_buffers);
    public native void destroy(long mod);
    public native int setResolution(long mod, int W, int H);
    public native int setFPS(long mod, int fps);
    public native int setFmt(long mod, String fmt);
    public native byte[] capFrame(long mod);
    public native int start(long mod);
    public native int stop(long mod);
}
