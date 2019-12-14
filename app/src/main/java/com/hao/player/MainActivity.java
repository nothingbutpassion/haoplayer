package com.hao.player;

import android.Manifest;
import android.app.Activity;
import android.content.pm.PackageManager;
import android.graphics.Color;
import android.graphics.PixelFormat;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.support.v4.app.ActivityCompat;
import android.util.Log;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import android.view.ViewGroup;
import android.view.Window;
import android.view.WindowManager;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.SeekBar;
import android.widget.TextView;
import android.widget.Toast;

import java.io.File;

public class MainActivity extends Activity {

    private static final String TAG = "haoplayer";
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

        // Request permissions
        requestPermissions();

        // Request full screen
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        getWindow().setFlags(WindowManager.LayoutParams.FLAG_FULLSCREEN,
                WindowManager.LayoutParams.FLAG_FULLSCREEN);


        // For xiaomi phone
        String url = "/storage/6464-3563/MyFiles/videos/music.avi";
        if (new File(url).canRead()) {
            Player.setDataSource(url);
        }

        SurfaceView playbackSurface = new SurfaceView(this);
        SurfaceHolder holder = playbackSurface.getHolder();
        holder.setFormat(PixelFormat.RGBA_8888);

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
        linearLayout.setPadding(8, 8, 8, 8);

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
        
        playbackSurface.setOnTouchListener(new View.OnTouchListener() {
            private long downTime;
            @Override
            public boolean onTouch(View v, MotionEvent event) {
                switch (event.getAction()) {
                    case MotionEvent.ACTION_DOWN:
                        downTime = System.currentTimeMillis();
                        break;
                    case MotionEvent.ACTION_UP:
                        if (System.currentTimeMillis() - downTime < 500) {
                            if (surfaceCreated) {
                                if (isPlaying) {
                                    Player.pause();
                                } else {
                                    Player.play();
                                }
                                isPlaying = !isPlaying;
                            }
                        }
                        break;
                    default:
                        break;
                }
                return true;
            }
        });

        holder.addCallback(new SurfaceHolder.Callback() {
            @Override
            public void surfaceCreated(SurfaceHolder holder) {
                Log.i(TAG, "surfaceCreated");
                surfaceCreated = true;
                Player.setSurface(holder.getSurface());
                stopThread = false;
                new Thread(new Runnable() {
                    @Override
                    public void run() {
                        while (!stopThread) {
                            int pos = Player.getPosition();
                            int du = Player.getDuration();
                            Message msg = Message.obtain();
                            msg.arg1 = pos;
                            msg.arg2 = du;
                            msg.what = 11;
                            handler.sendMessage(msg);
                            try {
                                Thread.sleep(1000);
                            } catch (Exception e) {
                                e.printStackTrace();
                            }
                        }
                    }
                }).start();
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
                stopThread = false;
                surfaceCreated = false;
            }
        });

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
                    if (isPlaying)
                        Player.play();
                    else
                        Player.pause();
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
        Log.d(TAG, "onResume");
        super.onResume();
    }

    @Override
    protected void onPause() {
        Log.d(TAG, "onPause");
        super.onPause();
        if (isPlaying) {
            Player.pause();
            isPlaying = false;
        }
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, String[] permissions, int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        for (int result: grantResults) {
            if (result != PackageManager.PERMISSION_GRANTED) {
                Toast.makeText(this,  permissions + "is not granted!", Toast.LENGTH_LONG).show();
                finish();
                return;
            }
        }

        // Reload the activity with permission granted
        Toast.makeText(this, "Permission is granted.", Toast.LENGTH_SHORT).show();
        finish();
        startActivity(getIntent());
    }

    private  boolean requestPermissions() {
        boolean granted = true;
        String[] permissisons = new String[]{
                Manifest.permission.READ_EXTERNAL_STORAGE,
                Manifest.permission.WRITE_EXTERNAL_STORAGE
        };
        // Check permissions
        for (String permission: permissisons) {
            if (ActivityCompat.checkSelfPermission(this, permission) != PackageManager.PERMISSION_GRANTED) {
                granted = false;
            }
        }
        // Request permissions
        if (!granted) {
            ActivityCompat.requestPermissions(this, permissisons, 0);
            return false;
        }
        return true;
    }


}
