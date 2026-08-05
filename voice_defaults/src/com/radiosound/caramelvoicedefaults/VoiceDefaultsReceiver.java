/*
 * Copyright 2026 Radio Sound, Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
package com.radiosound.caramelvoicedefaults;

import android.content.BroadcastReceiver;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.app.AppOpsManager;
import android.os.UserHandle;
import android.provider.Settings;
import android.util.Log;

/** Selects the product's offline TTS engine when the active user has no choice yet. */
public final class VoiceDefaultsReceiver extends BroadcastReceiver {
    private static final String TAG = "CaramelVoiceDefaults";
    private static final String TTS_DEFAULT_SYNTH = "tts_default_synth";
    private static final String ESPEAK_ENGINE = "com.reecedunn.espeak";
    private static final String CONTROL_AUDIO = "android:control_audio";
    private static final String CONTROL_AUDIO_PARTIAL = "android:control_audio_partial";

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
        if (current != null && !current.isEmpty() && !ESPEAK_ENGINE.equals(current)) {
            return;
        }
        if (current == null || current.isEmpty()) {
            if (!Settings.Secure.putString(resolver, TTS_DEFAULT_SYNTH, ESPEAK_ENGINE)) {
                Log.e(TAG, "Unable to select the offline eSpeak TTS engine");
                return;
            }
        }
        allowEspeakPlaybackForActiveUser(context);
        Log.i(TAG, "Selected offline eSpeak TTS engine and playback policy for the active user");
    }

    /**
     * Audio hardening defaults CONTROL_AUDIO app-ops to foreground-only. The bundled TTS engine
     * is a background bound service while it synthesizes, so allow only this product-owned engine
     * to play speech. This is intentionally package-scoped and does not disable hardening globally.
     */
    private static void allowEspeakPlaybackForActiveUser(Context context) {
        final int userId = UserHandle.myUserId();
        final int uid;
        try {
            uid = context.getPackageManager().getPackageUid(ESPEAK_ENGINE, 0);
        } catch (PackageManager.NameNotFoundException exception) {
            Log.w(TAG, "Offline TTS engine is not installed for user " + userId, exception);
            return;
        }

        final AppOpsManager appOps = context.getSystemService(AppOpsManager.class);
        appOps.setMode(CONTROL_AUDIO, uid, ESPEAK_ENGINE, AppOpsManager.MODE_ALLOWED);
        appOps.setMode(CONTROL_AUDIO_PARTIAL, uid, ESPEAK_ENGINE, AppOpsManager.MODE_ALLOWED);
        Log.i(TAG, "Allowed offline TTS playback hardening app-ops for " + ESPEAK_ENGINE
                + " uid " + uid + " user " + userId);
    }
}
