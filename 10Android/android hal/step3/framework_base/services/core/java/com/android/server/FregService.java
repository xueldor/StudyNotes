package com.android.server;

import android.content.Context;
import android.freg.IFregService;
import android.util.Slog;

public class FregService extends IFregService.Stub {
    private static final String TAG = "FregService";

    private long mPtr = 0;

    FregService(){
        mPtr = init_native();

        if(mPtr == 0) {
            Slog.e(TAG, "Failed to initialize freg service");
        }
    }

    public void setVal(int val) {
        if(mPtr == 0) {
            Slog.e(TAG, "Freg service is not initialized");
            return;
        }
        setVal_native(mPtr, val);
    }
    public int getVal() {
        if(mPtr == 0) {
            Slog.e(TAG, "Freg service is not initialized");
            return 0;
        }
        return getVal_native(mPtr);
    }

    private static native long init_native();
    private static native void setVal_native(long ptr, int val);
    private static native int getVal_native(long ptr);
}