/*
 * Copyright 2026 Radio Sound, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
package com.radiosound.carameldisplaydefaults;

import android.content.BroadcastReceiver;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.os.SystemProperties;
import android.provider.Settings;
import android.util.Log;

/** Applies the immutable hardware default for the Waveshare panel's refresh rate. */
public final class DisplayDefaultsReceiver extends BroadcastReceiver {
    private static final String TAG = "CaramelDisplayDefaults";
    private static final String REFRESH_RATE_PROPERTY =
            "ro.vendor.rpi5.display.refresh_rate";
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
        final String refreshRate = getConfiguredRefreshRate();
        if (refreshRate == null) {
            return;
        }
        putIfNeeded(resolver, PEAK_REFRESH_RATE, refreshRate);
        putIfNeeded(resolver, MIN_REFRESH_RATE, refreshRate);
    }

    private static String getConfiguredRefreshRate() {
        final String configured = SystemProperties.get(REFRESH_RATE_PROPERTY, "");
        if (configured.isEmpty()) {
            Log.e(TAG, "Missing " + REFRESH_RATE_PROPERTY);
            return null;
        }
        try {
            final float refreshRate = Float.parseFloat(configured);
            if (!Float.isFinite(refreshRate) || refreshRate <= 0.0f) {
                throw new NumberFormatException("non-positive or non-finite rate");
            }
        } catch (NumberFormatException e) {
            Log.e(TAG, "Invalid " + REFRESH_RATE_PROPERTY + "=" + configured, e);
            return null;
        }
        return configured;
    }

    private static void putIfNeeded(ContentResolver resolver, String key, String refreshRate) {
        final String current = Settings.System.getString(resolver, key);
        if (refreshRate.equals(current)) {
            return;
        }
        if (!Settings.System.putString(resolver, key, refreshRate)) {
            Log.e(TAG, "Unable to set " + key + " for the active user");
            return;
        }
        Log.i(TAG, "Set " + key + "=" + refreshRate + " for the active user");
    }
}
