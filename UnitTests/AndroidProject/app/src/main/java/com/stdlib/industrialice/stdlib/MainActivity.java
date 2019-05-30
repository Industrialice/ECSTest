package com.stdlib.industrialice.stdlib;

import android.app.Activity;
import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.support.annotation.NonNull;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.widget.ScrollView;
import android.widget.TextView;

public class MainActivity extends Activity {

    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("native-lib");
    }

    static private TextView tv;
    static private ScrollView scrollview;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        // Example of a call to a native method
        tv = (TextView) findViewById(R.id.sample_text);
        scrollview = (ScrollView) findViewById(R.id.scrollView);
        callIntoJNI(getApplicationInfo().dataDir);
    }

    private static Handler handler = new Handler(Looper.getMainLooper());

    public void OnLogMessage(final String message)
    {
        handler.post(new Runnable()
        {
            @Override
            public void run()
            {
                tv.append(message);
                scrollview.fullScroll(ScrollView.FOCUS_DOWN);
            }
        });
    }

    public void PopLastMessage(final int length)
    {
        handler.post(new Runnable()
        {
            @Override
            public void run()
            {
                String text = tv.getText().toString();
                tv.setText(text.substring(0, text.length() - length));
            }
        });
    }

    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
    public native void callIntoJNI(String appPath);
}
