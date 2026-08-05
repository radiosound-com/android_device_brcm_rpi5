/*
 * Copyright 2026 Radio Sound, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
package com.radiosound.carameldisplaydefaults;

import android.content.BroadcastReceiver;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.provider.Settings;
import android.util.Log;

/** Applies the immutable hardware default for the Waveshare panel's refresh rate. */
public final class DisplayDefaultsReceiver extends BroadcastReceiver {
    private static final String TAG = "CaramelDisplayDefaults";
    private static final String REFRESH_RATE = "60.0";
    // These Settings.System fields are hidden from the public Android SDK in AOSP 16.
    private static final String PEAK_REFRESH_RATE = "peak_refresh_rate";
    private static final String MIN_REFRESH_RATE = "min_refresh_rate";

    @Override
    public void onReceive(Context context, Intent intent) {
        final String action = intent.getAction();
        if (!Intent.ACTION_LOCKED_BOOT_COMPLETED.equals(action)
                && !Intent.ACTION_BOOT_COMPLETED.equals(action)
                && !Intent.ACTION_USER_UNLOCKED.equals(action)) {
            return;
        }

        final ContentResolver resolver = context.getContentResolver();
        putIfNeeded(resolver, PEAK_REFRESH_RATE);
        putIfNeeded(resolver, MIN_REFRESH_RATE);
    }

    private static void putIfNeeded(ContentResolver resolver, String key) {
        final String current = Settings.System.getString(resolver, key);
        if (REFRESH_RATE.equals(current)) {
            return;
        }
        if (!Settings.System.putString(resolver, key, REFRESH_RATE)) {
            Log.e(TAG, "Unable to set " + key + " for the active user");
            return;
        }
        Log.i(TAG, "Set " + key + "=" + REFRESH_RATE + " for the active user");
    }
}
