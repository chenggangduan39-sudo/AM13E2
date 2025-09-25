package com.qdreamer.avspeech;

public class Separator {
    public native long create(String cfg, String custom_str);
    public native void destory(long mod);
    public native void feedImage(long mod, byte[] data, int len, int H, int W);
    public native void feedImageNV21(long mod, byte[] data, int len);
    public native void feedAudio(long mod, byte[] data, int len);
    public native byte[] read(long mod);
    public native void process(String cfn, String dfn, int n);
    public native int reset(long mod);
    public native int[] getroi(int boundary_angle, int camera_angle, int W);
}
