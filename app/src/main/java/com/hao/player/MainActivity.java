package com.hao.player;

import android.app.Activity;
import android.graphics.Color;
import android.graphics.PixelFormat;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.view.Gravity;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.ViewGroup;
import android.view.Window;
import android.view.WindowManager;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.SeekBar;
import android.widget.TextView;

import java.io.File;

public class MainActivity extends Activity {

    private static final String TAG = MainActivity.class.getSimpleName();
    private boolean isPlaying = false;
    private boolean surfaceCreated = false;
    private boolean stopThread = false;
    private Handler handler = null;
    private SeekBar seek = null;
    private TextView position = null;
    private TextView duration = null;


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        // Request full screen
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,
                WindowManager.LayoutParams.FLAG_FULLSCREEN);


        // For xiaomi phone
        //String url = "/storage/6464-3563/MyFiles/videos/music.avi";
        String url = "/sdcard/videos/music.avi";
        if (new File(url).canRead()) {
            Player.setDataSource(url);
        }

        SurfaceView playbackSurface = new SurfaceView(this);
        SurfaceHolder holder = playbackSurface.getHolder();
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

        position = new TextView(this);
        position.setText(" 00:00 ");
        position.setTextColor(Color.WHITE);
        position.setTextAlignment(TextView.TEXT_ALIGNMENT_CENTER);
        seek = new SeekBar(this);
        duration = new TextView(this);
        duration.setText(" 00:00 ");
        duration.setTextColor(Color.WHITE);
        duration.setTextAlignment(TextView.TEXT_ALIGNMENT_CENTER);
        LinearLayout linearLayout = new LinearLayout(this);
        linearLayout.setOrientation(LinearLayout.HORIZONTAL);
        linearLayout.addView(position,  new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.WRAP_CONTENT,
                ViewGroup.LayoutParams.WRAP_CONTENT
        ));

        linearLayout.addView(seek,  new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.WRAP_CONTENT,
                ViewGroup.LayoutParams.WRAP_CONTENT,
                0.8f
        ));
        linearLayout.addView(duration,  new LinearLayout.LayoutParams(
                ViewGroup.LayoutParams.WRAP_CONTENT,
                ViewGroup.LayoutParams.WRAP_CONTENT
        ));
        linearLayout.setGravity(Gravity.CENTER);

        FrameLayout frameLayout = new FrameLayout(this);
        frameLayout.addView(playbackSurface, new FrameLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.MATCH_PARENT));
        frameLayout.addView(linearLayout, new FrameLayout.LayoutParams(
                ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.WRAP_CONTENT,
                Gravity.BOTTOM
        ));
        setContentView(frameLayout);

        seek.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {

            private int mProgress = 0;

            @Override
            public void onProgressChanged(SeekBar seekBar, int progress, boolean fromUser) {
                if (fromUser) {
                    mProgress = progress;
                    position.setText(String.format(" %02d:%02d", mProgress/60, mProgress%60));
                }
            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {
            }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {
                if (surfaceCreated) {
                    Player.seek(1000*mProgress);
                    isPlaying = false;
                    Player.play();
                    isPlaying = true;
                }
            }
        });

        handler = new Handler() {
            @Override
            public void handleMessage(Message msg) {
                if (msg.what == 11) {
                    int pos = msg.arg1/1000;
                    int du = msg.arg2/1000;
                    position.setText(String.format(" %02d:%02d", pos/60, pos%60));
                    duration.setText(String.format("%02d:%02d ", du/60, du%60));
                    if (du > 0) {
                        seek.setMax(du);
                        seek.setProgress(pos);
                    }
                }
                super.handleMessage(msg);
            }
        };

    }

    @Override
    protected void onResume() {
        super.onResume();
        if (surfaceCreated && !isPlaying) {
            Player.play();
            isPlaying = true;
        }

        new Thread(new Runnable() {
            @Override
            public void run() {
                stopThread = false;
                while (!stopThread) {
                    if (surfaceCreated && isPlaying) {
                        int pos = Player.getPosition();
                        int du = Player.getDuration();
                        Message msg = Message.obtain();
                        msg.arg1 = pos;
                        msg.arg2 = du;
                        msg.what = 11;
                        handler.sendMessage(msg);
                        try {
                            Thread.sleep(1000);
                        }
                        catch (Exception e) {

                        }

                    }
                }
            }
        }).start();
    }

    @Override
    protected void onPause() {
        super.onPause();
        if (surfaceCreated && isPlaying) {
            Player.pause();
            isPlaying = false;
        }
        stopThread = true;
    }


}
