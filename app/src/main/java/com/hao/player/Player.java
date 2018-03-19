package com.hao.player;

import android.view.Surface;

public class Player {
    static {
        System.loadLibrary("avutil");
        System.loadLibrary("avcodec");
        System.loadLibrary("avformat");
        System.loadLibrary("swresample");
        System.loadLibrary("swscale");
        System.loadLibrary("haoplayer");
        nativeInit();
    }
    private native static void nativeInit();
    public native static void setSurface(Surface surface);
    public native static void setDataSource(String source);
    public native static void play();
    public native static void pause();
    public native static void stop();
    public native static void seek(int position);
    public native static int getDuration();
    public native static int getPosition();
}

