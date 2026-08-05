/*
 * Copyright 2026 Radio Sound, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
package com.radiosound.caramelvoicedefaults;

import android.content.BroadcastReceiver;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.provider.Settings;
import android.util.Log;

/** Selects the product's offline TTS engine when the active user has no choice yet. */
public final class VoiceDefaultsReceiver extends BroadcastReceiver {
    private static final String TAG = "CaramelVoiceDefaults";
    private static final String TTS_DEFAULT_SYNTH = "tts_default_synth";
    private static final String ESPEAK_ENGINE = "com.reecedunn.espeak";

    @Override
    public void onReceive(Context context, Intent intent) {
        final String action = intent.getAction();
        if (!Intent.ACTION_LOCKED_BOOT_COMPLETED.equals(action)
                && !Intent.ACTION_BOOT_COMPLETED.equals(action)
                && !Intent.ACTION_USER_UNLOCKED.equals(action)) {
            return;
        }

        final ContentResolver resolver = context.getContentResolver();
        final String current = Settings.Secure.getString(resolver, TTS_DEFAULT_SYNTH);
        if (current != null && !current.isEmpty()) {
            return;
        }
        if (!Settings.Secure.putString(resolver, TTS_DEFAULT_SYNTH, ESPEAK_ENGINE)) {
            Log.e(TAG, "Unable to select the offline eSpeak TTS engine");
            return;
        }
        Log.i(TAG, "Selected offline eSpeak TTS engine for the active user");
    }
}
