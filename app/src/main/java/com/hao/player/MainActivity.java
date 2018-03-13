package com.hao.player;

import android.app.Activity;
import android.graphics.PixelFormat;
import android.os.Bundle;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.Window;
import android.view.WindowManager;

import java.io.File;

public class MainActivity extends Activity {

    private static final String TAG = MainActivity.class.getSimpleName();
    private boolean isPlaying = false;
    private boolean surfaceCreated = false;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        requestWindowFeature(Window.FEATURE_NO_TITLE);
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,
                WindowManager.LayoutParams.FLAG_FULLSCREEN);

        // For xiaomi phone
        //String url = "/storage/6464-3563/MyFiles/videos/music.avi";
        String url = "/sdcard/videos/music.avi";
        if (new File(url).canRead()) {
            Player.setDataSource(url);
        }

        SurfaceView view = new SurfaceView(this);
        SurfaceHolder holder = view.getHolder();
        holder.setFormat(PixelFormat.RGBA_8888);
        holder.addCallback(new SurfaceHolder.Callback() {
            @Override
            public void surfaceCreated(SurfaceHolder holder) {
                Log.i(TAG, "surfaceCreated");
                surfaceCreated = true;
                Player.setSurface(holder.getSurface());
            }
            @Override
            public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
                Log.i(TAG, "surfaceChanged");
                if (!isPlaying) {
                    Player.play();
                    isPlaying = true;
                }
            }

            @Override
            public void surfaceDestroyed(SurfaceHolder holder) {
                Log.i(TAG, "surfaceDestroyed");
                if (isPlaying) {
                    Player.stop();
                }
                surfaceCreated = false;
            }
        });
        setContentView(view);
    }

    @Override
    protected void onResume() {
        super.onResume();
        if (surfaceCreated && !isPlaying) {
            Player.play();
            isPlaying = true;
        }
    }

    @Override
    protected void onPause() {
        super.onPause();
        if (surfaceCreated && isPlaying) {
            Player.pause();
            isPlaying = false;
        }
    }


}
