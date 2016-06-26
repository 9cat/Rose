package org.libsdl.app;


import android.app.IntentService;
import android.content.Intent;
import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.os.Binder;
import android.os.IBinder;
import android.util.Log;

public class PlaybackAudioService extends IntentService {
    private static final String TAG = "SDL";
    // Audio
    protected AudioTrack mAudioTrack;
    protected boolean mXmiting;
    protected int bufferSizeInBytes;
    protected int length;
    protected byte[] byteBuffer;
    protected short[] shortBuffer;

    public static boolean mExitThread = true;
    public static boolean mDestroyed;
    public static boolean mRequireCreate = false;
    private static int mSampleRate = 0;
    private static boolean mIs16Bit;
    private static boolean mIsStereo;
    private static int mDesiredFrames;

    private final IBinder mBinder = new LocalBinder();
    /**
     * A constructor is required, and must call the super IntentService(String)
     * constructor with a name for the worker thread.
     */
    public PlaybackAudioService() {
        super("PlaybackAudioService");
        Log.i("SDL", "PlaybackAudioService, PlaybackAudioService(), " + System.currentTimeMillis());
        mXmiting = false;
        bufferSizeInBytes = 0;
    }

    public static native boolean nativeFillByteAudio(byte[] buffer);
    public static native boolean nativeFillShortAudio(short[] buffer);

    /**
     * The IntentService calls this method from the default worker thread with
     * the intent that started the service. When this method returns, IntentService
     * stops the service, as appropriate.
     */
    @Override
    protected void onHandleIntent(Intent intent) {
        Log.i("SDL", "PlaybackAudioService, 1, onHandleIntent(), " + System.currentTimeMillis());

        while (!mExitThread) {
            if (mRequireCreate) {
                mRequireCreate = false;
                if (mAudioTrack != null) {
                    Log.i("SDL", "PlaybackAudioService, onHandleIntent, stop audio track!");
                    mAudioTrack.stop();
                    mAudioTrack = null;
                }
                if (audioInit(mSampleRate, mIs16Bit, mIsStereo, mDesiredFrames) != 0) {
                    Log.i("SDL", "PlaybackAudioService, onHandleIntent, audioInit failed!");
                    break;
                }
            }

            // app will use this thread for execute background task, so call app function every time.
            if (mIs16Bit) {
                mXmiting = nativeFillShortAudio(shortBuffer);
            } else {
                mXmiting = nativeFillByteAudio(byteBuffer);
            }
            if (!mXmiting) {
                try {
                    Thread.sleep(10);
                } catch(InterruptedException e) {
                    // Nom nom
                }
                continue;
            }
            for (int i = 0; i < length; ) {
                int result = mIs16Bit? mAudioTrack.write(shortBuffer, i, length - i): mAudioTrack.write(byteBuffer, i, length - i);
                if (result > 0) {
                    i += result;
                } else if (result == 0) {
                    try {
                        Thread.sleep(1);
                    } catch(InterruptedException e) {
                        // Nom nom
                    }
                } else {
                    Log.w(TAG, "SDL audio: error return from write(byte)");
                    mXmiting = false;
                    break;
                }
            }
        }

        Log.i("SDL", "PlaybackAudioService, 2, onHandleIntent(), " + System.currentTimeMillis());
        if (mAudioTrack != null) {
            mAudioTrack.stop();
            mAudioTrack = null;
        }

        Log.i("SDL", "PlaybackAudioService, 3, onHandleIntent(), " + System.currentTimeMillis());
    }

    @Override
    public IBinder onBind(Intent intent) {
        // mBound = true;
        Log.i("SDL", "PlaybackAudioService, onBind(), " + System.currentTimeMillis());
        return mBinder;
    }

    @Override
    public boolean onUnbind(Intent intent) {
        // mBound = false;
        // close();
        Log.i("SDL", "PlaybackAudioService, onUnbind(), " + System.currentTimeMillis());
        return super.onUnbind(intent);
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        mDestroyed = true;
        Log.i("SDL", "PlaybackAudioService, onDestroy(), " + System.currentTimeMillis());
    }

    /**
     * Local binder class
     */
    public class LocalBinder extends Binder {
        public PlaybackAudioService getService() {
            return PlaybackAudioService.this;
        }
    }

    public static void audioSetParam(int sampleRate, boolean is16Bit, boolean isStereo, int desiredFrames) {
        mSampleRate = sampleRate;
        mIs16Bit = is16Bit;
        mIsStereo = isStereo;
        mDesiredFrames = desiredFrames;

        mRequireCreate = true;
    }

    private int audioInit(int sampleRate, boolean is16Bit, boolean isStereo, int desiredFrames) {
        if (mAudioTrack != null) {
            Log.i(TAG, "mAudioTrack must be null before initialization of Audio Track");
            return -1;
        }
        int channelConfig = isStereo ? AudioFormat.CHANNEL_CONFIGURATION_STEREO : AudioFormat.CHANNEL_CONFIGURATION_MONO;
        int audioFormat = is16Bit ? AudioFormat.ENCODING_PCM_16BIT : AudioFormat.ENCODING_PCM_8BIT;
        int frameSize = (isStereo ? 2 : 1) * (is16Bit ? 2 : 1);

        Log.i(TAG, "SDL audio: wanted " + (isStereo ? "stereo" : "mono") + " " + (is16Bit ? "16-bit" : "8-bit") + " " + (sampleRate / 1000f) + "kHz, " + desiredFrames + " frames buffer");

        // Let the user pick a larger buffer if they really want -- but ye
        // gods they probably shouldn't, the minimums are horrifyingly high
        // latency already
        desiredFrames = Math.max(desiredFrames, (AudioTrack.getMinBufferSize(sampleRate, channelConfig, audioFormat) + frameSize - 1) / frameSize);

        mAudioTrack = new AudioTrack(AudioManager.STREAM_MUSIC, sampleRate,
                    channelConfig, audioFormat, desiredFrames * frameSize, AudioTrack.MODE_STREAM);

        // Instantiating AudioTrack can "succeed" without an exception and the track may still be invalid
        // Ref: https://android.googlesource.com/platform/frameworks/base/+/refs/heads/master/media/java/android/media/AudioTrack.java
        // Ref: http://developer.android.com/reference/android/media/AudioTrack.html#getState()

        if (mAudioTrack.getState() != AudioTrack.STATE_INITIALIZED) {
            Log.e(TAG, "Failed during initialization of Audio Track");
            mAudioTrack = null;
            return -1;
        }

        mAudioTrack.play();

        bufferSizeInBytes = desiredFrames * frameSize;
        length = mIs16Bit? bufferSizeInBytes / 2: bufferSizeInBytes;
        byteBuffer = mIs16Bit? null: new byte[length];
        shortBuffer = mIs16Bit? new short[length]: null;

        Log.i(TAG, "SDL audio: got " + ((mAudioTrack.getChannelCount() >= 2) ? "stereo" : "mono") + " " + ((mAudioTrack.getAudioFormat() == AudioFormat.ENCODING_PCM_16BIT) ? "16-bit" : "8-bit") + " " + (mAudioTrack.getSampleRate() / 1000f) + "kHz, " + desiredFrames + " frames buffer");

        return 0;
    }

    /**
     * This method is called by SDL using JNI.
     */
    public void audioQuit() {
        if (mAudioTrack == null) {
            Log.i(TAG, "PlaybackAudioService.audioQuit, mAudioTrack == null, do nothing");
            return;
        }
        Log.i(TAG, "PlaybackAudioService.audioQuit, set mExitThread = true.");
        mExitThread = true;

        while (mAudioTrack != null) {
            try {
                Thread.sleep(1);
            } catch (InterruptedException e) {
                // Nom nom
            }
        }
        Log.i(TAG, "PlaybackAudioService.audioQuit, exited");
    }
}